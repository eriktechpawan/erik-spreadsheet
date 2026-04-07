#include "data/duckdb/duckdb_connection.h"
#include "utils/logging.h"

#include <QDate>
#include <QDateTime>
#include <QTime>

namespace csvforge {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

DuckDBConnection::DuckDBConnection()
{
    if (duckdb_open(nullptr, &m_database) != DuckDBSuccess) {
        m_lastError = QStringLiteral("Failed to open in-memory DuckDB database");
        logMsg(LogCategory::Database, LogLevel::Error, m_lastError);
        m_database = nullptr;
        return;
    }

    if (duckdb_connect(m_database, &m_connection) != DuckDBSuccess) {
        m_lastError = QStringLiteral("Failed to create DuckDB connection");
        logMsg(LogCategory::Database, LogLevel::Error, m_lastError);
        duckdb_close(&m_database);
        m_database = nullptr;
        m_connection = nullptr;
        return;
    }

    logMsg(LogCategory::Database, LogLevel::Debug,
           QStringLiteral("DuckDB in-memory database opened"));
}

DuckDBConnection::~DuckDBConnection()
{
    cleanup();
}

// ---------------------------------------------------------------------------
// Move semantics
// ---------------------------------------------------------------------------

DuckDBConnection::DuckDBConnection(DuckDBConnection&& other) noexcept
    : m_database(other.m_database)
    , m_connection(other.m_connection)
    , m_lastError(std::move(other.m_lastError))
{
    other.m_database = nullptr;
    other.m_connection = nullptr;
}

DuckDBConnection& DuckDBConnection::operator=(DuckDBConnection&& other) noexcept
{
    if (this != &other) {
        cleanup();
        m_database = other.m_database;
        m_connection = other.m_connection;
        m_lastError = std::move(other.m_lastError);
        other.m_database = nullptr;
        other.m_connection = nullptr;
    }
    return *this;
}

// ---------------------------------------------------------------------------
// State queries
// ---------------------------------------------------------------------------

bool DuckDBConnection::isOpen() const
{
    return m_database != nullptr && m_connection != nullptr;
}

QString DuckDBConnection::lastError() const
{
    return m_lastError;
}

duckdb_connection DuckDBConnection::rawConnection() const
{
    return m_connection;
}

// ---------------------------------------------------------------------------
// Query execution
// ---------------------------------------------------------------------------

DuckDBConnection::QueryResult DuckDBConnection::execute(const QString& sql)
{
    QueryResult qr;

    if (!isOpen()) {
        qr.error = QStringLiteral("Connection is not open");
        m_lastError = qr.error;
        return qr;
    }

    const QByteArray utf8 = sql.toUtf8();
    duckdb_result result;

    if (duckdb_query(m_connection, utf8.constData(), &result) != DuckDBSuccess) {
        const char* err = duckdb_result_error(&result);
        qr.error = err ? QString::fromUtf8(err) : QStringLiteral("Unknown query error");
        m_lastError = qr.error;
        logMsg(LogCategory::Database, LogLevel::Error,
               QStringLiteral("Query failed: %1").arg(qr.error));
        duckdb_destroy_result(&result);
        return qr;
    }

    const idx_t colCount = duckdb_column_count(&result);
    const idx_t rowCount = duckdb_row_count(&result);

    // Column metadata
    qr.columnNames.reserve(static_cast<size_t>(colCount));
    qr.columnTypes.reserve(static_cast<size_t>(colCount));
    for (idx_t c = 0; c < colCount; ++c) {
        const char* name = duckdb_column_name(&result, c);
        qr.columnNames.emplace_back(name ? QString::fromUtf8(name) : QString());

        duckdb_type t = duckdb_column_type(&result, c);
        switch (t) {
        case DUCKDB_TYPE_BOOLEAN:   qr.columnTypes.emplace_back(QStringLiteral("BOOLEAN")); break;
        case DUCKDB_TYPE_TINYINT:   qr.columnTypes.emplace_back(QStringLiteral("TINYINT")); break;
        case DUCKDB_TYPE_SMALLINT:  qr.columnTypes.emplace_back(QStringLiteral("SMALLINT")); break;
        case DUCKDB_TYPE_INTEGER:   qr.columnTypes.emplace_back(QStringLiteral("INTEGER")); break;
        case DUCKDB_TYPE_BIGINT:    qr.columnTypes.emplace_back(QStringLiteral("BIGINT")); break;
        case DUCKDB_TYPE_FLOAT:     qr.columnTypes.emplace_back(QStringLiteral("FLOAT")); break;
        case DUCKDB_TYPE_DOUBLE:    qr.columnTypes.emplace_back(QStringLiteral("DOUBLE")); break;
        case DUCKDB_TYPE_VARCHAR:   qr.columnTypes.emplace_back(QStringLiteral("VARCHAR")); break;
        case DUCKDB_TYPE_DATE:      qr.columnTypes.emplace_back(QStringLiteral("DATE")); break;
        case DUCKDB_TYPE_TIMESTAMP: qr.columnTypes.emplace_back(QStringLiteral("TIMESTAMP")); break;
        case DUCKDB_TYPE_BLOB:      qr.columnTypes.emplace_back(QStringLiteral("BLOB")); break;
        case DUCKDB_TYPE_DECIMAL:   qr.columnTypes.emplace_back(QStringLiteral("DECIMAL")); break;
        case DUCKDB_TYPE_HUGEINT:   qr.columnTypes.emplace_back(QStringLiteral("HUGEINT")); break;
        default:                    qr.columnTypes.emplace_back(QStringLiteral("UNKNOWN")); break;
        }
    }

    // Row data
    qr.rows.reserve(static_cast<size_t>(rowCount));
    for (idx_t r = 0; r < rowCount; ++r) {
        std::vector<QVariant> row;
        row.reserve(static_cast<size_t>(colCount));
        for (idx_t c = 0; c < colCount; ++c) {
            row.emplace_back(extractValue(&result, r, c));
        }
        qr.rows.emplace_back(std::move(row));
    }

    qr.rowsAffected = duckdb_rows_changed(&result);
    qr.totalRows = static_cast<qint64>(rowCount);
    qr.success = true;
    m_lastError.clear();

    duckdb_destroy_result(&result);
    return qr;
}

DuckDBConnection::QueryResult DuckDBConnection::execute(
    const QString& sql, const std::vector<QVariant>& params)
{
    // DuckDB C API does not have a first-class prepared-statement bind for
    // arbitrary types in a single convenience call.  Build parameterised SQL by
    // safe substitution.  For production hardening the prepared-statement API
    // (duckdb_prepare / duckdb_bind_*) should be used; here we keep the
    // implementation simple and delegate to the non-parameterised overload
    // after replacing positional $N placeholders.

    QString resolved = sql;
    for (size_t i = 0; i < params.size(); ++i) {
        const QString placeholder = QStringLiteral("$%1").arg(i + 1);
        const QVariant& v = params[i];

        QString replacement;
        if (v.isNull()) {
            replacement = QStringLiteral("NULL");
        } else {
            switch (v.typeId()) {
            case QMetaType::Int:
            case QMetaType::LongLong:
                replacement = QString::number(v.toLongLong());
                break;
            case QMetaType::Double:
            case QMetaType::Float:
                replacement = QString::number(v.toDouble(), 'g', 15);
                break;
            case QMetaType::Bool:
                replacement = v.toBool() ? QStringLiteral("TRUE") : QStringLiteral("FALSE");
                break;
            default: {
                QString s = v.toString();
                s.replace(QLatin1Char('\''), QLatin1String("''"));
                replacement = QLatin1Char('\'') + s + QLatin1Char('\'');
                break;
            }
            }
        }
        resolved.replace(placeholder, replacement);
    }

    return execute(resolved);
}

// ---------------------------------------------------------------------------
// Convenience helpers
// ---------------------------------------------------------------------------

QVariant DuckDBConnection::executeScalar(const QString& sql)
{
    QueryResult qr = execute(sql);
    if (qr.success && !qr.rows.empty() && !qr.rows[0].empty()) {
        return qr.rows[0][0];
    }
    return {};
}

qint64 DuckDBConnection::executeCount(const QString& sql)
{
    QVariant v = executeScalar(sql);
    if (v.isValid()) {
        bool ok = false;
        qint64 n = v.toLongLong(&ok);
        if (ok) return n;
    }
    return -1;
}

// ---------------------------------------------------------------------------
// Value extraction
// ---------------------------------------------------------------------------

QVariant DuckDBConnection::extractValue(duckdb_result* result, idx_t row, idx_t col)
{
    if (duckdb_value_is_null(result, col, row)) {
        return {};
    }

    duckdb_type type = duckdb_column_type(result, col);

    switch (type) {
    case DUCKDB_TYPE_BOOLEAN:
        return QVariant(static_cast<bool>(duckdb_value_boolean(result, col, row)));

    case DUCKDB_TYPE_TINYINT:
        return QVariant(static_cast<int>(duckdb_value_int8(result, col, row)));

    case DUCKDB_TYPE_SMALLINT:
        return QVariant(static_cast<int>(duckdb_value_int16(result, col, row)));

    case DUCKDB_TYPE_INTEGER:
        return QVariant(static_cast<int>(duckdb_value_int32(result, col, row)));

    case DUCKDB_TYPE_BIGINT:
        return QVariant(static_cast<qint64>(duckdb_value_int64(result, col, row)));

    case DUCKDB_TYPE_FLOAT:
        return QVariant(static_cast<double>(duckdb_value_float(result, col, row)));

    case DUCKDB_TYPE_DOUBLE:
        return QVariant(duckdb_value_double(result, col, row));

    case DUCKDB_TYPE_DATE: {
        duckdb_date d = duckdb_value_date(result, col, row);
        duckdb_date_struct ds = duckdb_from_date(d);
        return QVariant(QDate(ds.year, ds.month, ds.day));
    }

    case DUCKDB_TYPE_TIMESTAMP: {
        duckdb_timestamp ts = duckdb_value_timestamp(result, col, row);
        duckdb_timestamp_struct tss = duckdb_from_timestamp(ts);
        QDate date(tss.date.year, tss.date.month, tss.date.day);
        QTime time(tss.time.hour, tss.time.min, tss.time.sec,
                   tss.time.micros / 1000);
        return QVariant(QDateTime(date, time));
    }

    case DUCKDB_TYPE_VARCHAR:
    case DUCKDB_TYPE_BLOB:
    case DUCKDB_TYPE_DECIMAL:
    case DUCKDB_TYPE_HUGEINT:
    default: {
        // For all other types fall back to string representation
        char* str = duckdb_value_varchar(result, col, row);
        if (str) {
            QString val = QString::fromUtf8(str);
            duckdb_free(str);
            return QVariant(val);
        }
        return {};
    }
    }
}

// ---------------------------------------------------------------------------
// Cleanup
// ---------------------------------------------------------------------------

void DuckDBConnection::cleanup()
{
    if (m_connection) {
        duckdb_disconnect(&m_connection);
        m_connection = nullptr;
    }
    if (m_database) {
        duckdb_close(&m_database);
        m_database = nullptr;
    }
}

} // namespace csvforge
