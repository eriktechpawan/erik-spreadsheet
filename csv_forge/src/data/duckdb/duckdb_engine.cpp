#include "data/duckdb/duckdb_engine.h"
#include "utils/logging.h"
#include "utils/perf_timer.h"
#include "utils/string_utils.h"

#include <QFileInfo>

namespace csvforge {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

DuckDBEngine::DuckDBEngine(QObject* parent)
    : QObject(parent)
{
}

DuckDBEngine::~DuckDBEngine() = default;

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------

bool DuckDBEngine::initialize()
{
    if (m_initialized) {
        return true;
    }

    m_connection = std::make_unique<DuckDBConnection>();
    if (!m_connection->isOpen()) {
        const QString err = m_connection->lastError();
        logMsg(LogCategory::Database, LogLevel::Error,
               QStringLiteral("DuckDBEngine: init failed – %1").arg(err));
        emit errorOccurred(err);
        m_connection.reset();
        return false;
    }

    m_initialized = true;
    logMsg(LogCategory::Database, LogLevel::Info,
           QStringLiteral("DuckDBEngine initialized"));
    return true;
}

bool DuckDBEngine::isInitialized() const
{
    return m_initialized && m_connection && m_connection->isOpen();
}

// ---------------------------------------------------------------------------
// Table management
// ---------------------------------------------------------------------------

QString DuckDBEngine::loadCSVAsTable(const QString& filePath,
                                     const QString& tableName,
                                     const CSVImportOptions& options)
{
    if (!isInitialized()) {
        return QStringLiteral("Engine not initialized");
    }

    PERF_TIMER(QStringLiteral("loadCSVAsTable(%1)").arg(tableName));

    // Ensure the file exists
    if (!QFileInfo::exists(filePath)) {
        const QString err = QStringLiteral("File not found: %1").arg(filePath);
        emit errorOccurred(err);
        return err;
    }

    // Drop existing table if present
    if (tableExists(tableName)) {
        dropTable(tableName);
    }

    // Build read_csv parameters
    QStringList csvArgs;
    csvArgs << QStringLiteral("'%1'").arg(escapeSQL(filePath));

    if (!options.autoDetect) {
        csvArgs << QStringLiteral("auto_detect=false");
        csvArgs << QStringLiteral("delim='%1'").arg(escapeSQL(QString(options.delimiter)));
        csvArgs << QStringLiteral("quote='%1'").arg(escapeSQL(QString(options.quoteChar)));
        csvArgs << QStringLiteral("header=%1").arg(options.hasHeader
                                                    ? QStringLiteral("true")
                                                    : QStringLiteral("false"));
    } else {
        csvArgs << QStringLiteral("auto_detect=true");
    }

    if (options.skipRows > 0) {
        csvArgs << QStringLiteral("skip=%1").arg(options.skipRows);
    }

    if (options.sampleSize > 0) {
        csvArgs << QStringLiteral("sample_size=%1").arg(options.sampleSize);
    }

    if (options.ignoreErrors) {
        csvArgs << QStringLiteral("ignore_errors=true");
    }

    const QString sql = QStringLiteral("CREATE TABLE %1 AS SELECT * FROM read_csv(%2)")
                            .arg(quoteName(tableName), csvArgs.join(QStringLiteral(", ")));

    auto qr = m_connection->execute(sql);
    if (!qr.success) {
        emit errorOccurred(qr.error);
        return qr.error;
    }

    const qint64 rows = getRowCount(tableName);
    logMsg(LogCategory::Import, LogLevel::Info,
           QStringLiteral("Loaded '%1' → table %2 (%3 rows)")
               .arg(filePath, tableName)
               .arg(rows));

    emit tableLoaded(tableName, rows);
    return {};  // empty = success
}

bool DuckDBEngine::dropTable(const QString& tableName)
{
    if (!isInitialized()) return false;

    const QString sql = QStringLiteral("DROP TABLE IF EXISTS %1")
                            .arg(quoteName(tableName));
    auto qr = m_connection->execute(sql);
    if (!qr.success) {
        logMsg(LogCategory::Database, LogLevel::Warning,
               QStringLiteral("dropTable(%1) failed: %2").arg(tableName, qr.error));
    }
    return qr.success;
}

bool DuckDBEngine::tableExists(const QString& tableName) const
{
    if (!isInitialized()) return false;

    const QString sql = QStringLiteral(
        "SELECT COUNT(*) FROM information_schema.tables WHERE table_name = '%1'")
        .arg(escapeSQL(tableName));

    // const_cast: executeScalar is logically const but the C API is not
    auto* self = const_cast<DuckDBEngine*>(this);
    return self->m_connection->executeCount(sql) > 0;
}

QStringList DuckDBEngine::listTables() const
{
    if (!isInitialized()) return {};

    auto* self = const_cast<DuckDBEngine*>(this);
    auto qr = self->m_connection->execute(
        QStringLiteral("SELECT table_name FROM information_schema.tables "
                        "WHERE table_schema = 'main' ORDER BY table_name"));

    QStringList tables;
    if (qr.success) {
        tables.reserve(static_cast<int>(qr.rows.size()));
        for (const auto& row : qr.rows) {
            if (!row.empty()) {
                tables << row[0].toString();
            }
        }
    }
    return tables;
}

// ---------------------------------------------------------------------------
// Schema
// ---------------------------------------------------------------------------

ColumnSchemaList DuckDBEngine::getTableSchema(const QString& tableName) const
{
    ColumnSchemaList schema;
    if (!isInitialized()) return schema;

    auto* self = const_cast<DuckDBEngine*>(this);
    const QString sql = QStringLiteral("PRAGMA table_info('%1')").arg(escapeSQL(tableName));
    auto qr = self->m_connection->execute(sql);
    if (!qr.success) return schema;

    for (int i = 0; i < static_cast<int>(qr.rows.size()); ++i) {
        const auto& row = qr.rows[static_cast<size_t>(i)];
        // PRAGMA table_info columns: cid, name, type, notnull, dflt_value, pk
        if (row.size() < 3) continue;

        ColumnSchema col;
        col.index = row[0].toInt();
        col.name = row[1].toString();
        col.duckdbType = row[2].toString();
        col.type = ColumnSchema::fromDuckDBType(col.duckdbType);
        col.displayOrder = i;
        schema.push_back(std::move(col));
    }

    return schema;
}

qint64 DuckDBEngine::getRowCount(const QString& tableName) const
{
    if (!isInitialized()) return -1;

    auto* self = const_cast<DuckDBEngine*>(this);
    const QString sql = QStringLiteral("SELECT COUNT(*) FROM %1").arg(quoteName(tableName));
    return self->m_connection->executeCount(sql);
}

qint64 DuckDBEngine::getFilteredRowCount(const QString& tableName,
                                          const QString& whereClause) const
{
    if (!isInitialized()) return -1;

    auto* self = const_cast<DuckDBEngine*>(this);
    QString sql = QStringLiteral("SELECT COUNT(*) FROM %1").arg(quoteName(tableName));
    if (!whereClause.isEmpty()) {
        sql += QStringLiteral(" WHERE %1").arg(whereClause);
    }
    return self->m_connection->executeCount(sql);
}

// ---------------------------------------------------------------------------
// Query execution
// ---------------------------------------------------------------------------

DuckDBConnection::QueryResult DuckDBEngine::executeQuery(const QString& sql)
{
    if (!isInitialized()) {
        return { false, QStringLiteral("Engine not initialized"), {}, {}, {}, 0, 0 };
    }
    return m_connection->execute(sql);
}

QVariant DuckDBEngine::executeScalar(const QString& sql)
{
    if (!isInitialized()) return {};
    return m_connection->executeScalar(sql);
}

DuckDBConnection::QueryResult DuckDBEngine::scanCSV(const QString& filePath,
                                                     const QString& sql)
{
    if (!isInitialized()) {
        return { false, QStringLiteral("Engine not initialized"), {}, {}, {}, 0, 0 };
    }

    // Replace a placeholder token with the read_csv_auto call
    QString resolved = sql;
    if (resolved.contains(QStringLiteral("{csv}"))) {
        resolved.replace(QStringLiteral("{csv}"),
                         QStringLiteral("read_csv_auto('%1')").arg(escapeSQL(filePath)));
    } else {
        // If no placeholder, wrap the whole thing
        resolved = QStringLiteral("SELECT * FROM read_csv_auto('%1')").arg(escapeSQL(filePath));
        if (!sql.trimmed().isEmpty() && sql.trimmed().toUpper() != QStringLiteral("SELECT *")) {
            // Caller provided a full SQL statement; use as-is
            resolved = sql;
        }
    }

    return m_connection->execute(resolved);
}

// ---------------------------------------------------------------------------
// Connection access
// ---------------------------------------------------------------------------

DuckDBConnection& DuckDBEngine::connection()
{
    return *m_connection;
}

const DuckDBConnection& DuckDBEngine::connection() const
{
    return *m_connection;
}

} // namespace csvforge
