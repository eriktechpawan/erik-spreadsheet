#include "data/duckdb/csv_import_service.h"
#include "utils/logging.h"
#include "utils/perf_timer.h"
#include "utils/string_utils.h"

#include <QFileInfo>
#include <chrono>

namespace csvforge {

CSVImportService::CSVImportService(DuckDBEngine* engine, QObject* parent)
    : QObject(parent)
    , m_engine(engine)
{
}

// ---------------------------------------------------------------------------
// Import
// ---------------------------------------------------------------------------

CSVImportService::ImportResult CSVImportService::importCSV(
    const QString& filePath,
    const QString& tableName,
    const CSVImportOptions& options)
{
    ImportResult result;
    const auto t0 = std::chrono::steady_clock::now();

    emit progressChanged(0, QStringLiteral("Starting import of %1…").arg(QFileInfo(filePath).fileName()));

    if (!m_engine || !m_engine->isInitialized()) {
        result.error = QStringLiteral("Engine not initialized");
        return result;
    }

    emit progressChanged(10, QStringLiteral("Loading CSV into DuckDB…"));

    const QString err = m_engine->loadCSVAsTable(filePath, tableName, options);
    if (!err.isEmpty()) {
        result.error = err;
        logMsg(LogCategory::Import, LogLevel::Error,
               QStringLiteral("Import failed: %1").arg(err));
        return result;
    }

    emit progressChanged(80, QStringLiteral("Retrieving schema…"));

    result.success = true;
    result.tableName = tableName;
    result.rowCount = m_engine->getRowCount(tableName);
    result.schema = m_engine->getTableSchema(tableName);

    // Estimate error rows when ignore_errors was set (DuckDB does not expose
    // rejected row count directly; we record 0 as a conservative default).
    result.errorRows = 0;

    const auto t1 = std::chrono::steady_clock::now();
    result.elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    emit progressChanged(100, QStringLiteral("Import complete – %1 rows")
                                  .arg(formatNumber(result.rowCount)));

    logMsg(LogCategory::Import, LogLevel::Info,
           QStringLiteral("Imported %1 rows from '%2' in %3 ms")
               .arg(result.rowCount)
               .arg(filePath)
               .arg(result.elapsedMs));

    return result;
}

// ---------------------------------------------------------------------------
// Preview
// ---------------------------------------------------------------------------

DuckDBConnection::QueryResult CSVImportService::previewCSV(
    const QString& filePath,
    int maxRows,
    const CSVImportOptions& options)
{
    if (!m_engine || !m_engine->isInitialized()) {
        return { false, QStringLiteral("Engine not initialized"), {}, {}, {}, 0, 0 };
    }

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

    if (options.ignoreErrors) {
        csvArgs << QStringLiteral("ignore_errors=true");
    }

    const QString sql = QStringLiteral("SELECT * FROM read_csv(%1) LIMIT %2")
                            .arg(csvArgs.join(QStringLiteral(", ")))
                            .arg(maxRows);

    return m_engine->executeQuery(sql);
}

// ---------------------------------------------------------------------------
// Format detection
// ---------------------------------------------------------------------------

CSVFormat CSVImportService::detectFormat(const QString& filePath)
{
    return CSVSniffer::detect(filePath);
}

} // namespace csvforge
