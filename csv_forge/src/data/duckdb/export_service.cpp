#include "data/duckdb/export_service.h"
#include "utils/logging.h"
#include "utils/perf_timer.h"
#include "utils/string_utils.h"

#include <chrono>

namespace csvforge {

ExportService::ExportService(DuckDBEngine* engine, QObject* parent)
    : QObject(parent)
    , m_engine(engine)
{
}

ExportService::ExportResult ExportService::exportData(const ExportOptions& options)
{
    ExportResult result;
    const auto t0 = std::chrono::steady_clock::now();

    if (!m_engine || !m_engine->isInitialized()) {
        result.error = QStringLiteral("Engine not initialized");
        return result;
    }

    if (options.filePath.isEmpty()) {
        result.error = QStringLiteral("Export file path is empty");
        return result;
    }

    if (options.tableName.isEmpty()) {
        result.error = QStringLiteral("Table name is empty");
        return result;
    }

    emit progressChanged(0, QStringLiteral("Preparing export…"));

    // Build the SELECT portion
    QString selectCols;
    if (options.columns.isEmpty()) {
        selectCols = QStringLiteral("*");
    } else {
        QStringList quoted;
        quoted.reserve(options.columns.size());
        for (const QString& c : options.columns) {
            quoted << quoteName(c);
        }
        selectCols = quoted.join(QStringLiteral(", "));
    }

    // Build the inner SELECT statement
    QString innerSql = QStringLiteral("SELECT %1 FROM %2")
                           .arg(selectCols, quoteName(options.tableName));

    if (!options.whereClause.isEmpty()) {
        innerSql += QStringLiteral(" WHERE %1").arg(options.whereClause);
    }

    // Count rows to export (for progress reporting)
    const QString countSql = QStringLiteral("SELECT COUNT(*) FROM (%1) AS __export_sub__")
                                 .arg(innerSql);
    const qint64 totalRows = m_engine->connection().executeCount(countSql);

    emit progressChanged(10, QStringLiteral("Exporting %1 rows…").arg(formatNumber(totalRows)));

    // Determine format options for COPY
    const QString fmt = options.format.toLower();
    QString copyOptions;

    if (fmt == QLatin1String("parquet")) {
        copyOptions = QStringLiteral("FORMAT PARQUET");
    } else if (fmt == QLatin1String("tsv")) {
        copyOptions = QStringLiteral("FORMAT CSV, DELIMITER '\\t', HEADER %1")
                          .arg(options.includeHeader ? QStringLiteral("true")
                                                    : QStringLiteral("false"));
    } else {
        // Default: CSV
        copyOptions = QStringLiteral("FORMAT CSV, DELIMITER '%1', HEADER %2")
                          .arg(escapeSQL(QString(options.delimiter)))
                          .arg(options.includeHeader ? QStringLiteral("true")
                                                    : QStringLiteral("false"));
    }

    const QString copySql = QStringLiteral("COPY (%1) TO '%2' (%3)")
                                .arg(innerSql, escapeSQL(options.filePath), copyOptions);

    auto qr = m_engine->executeQuery(copySql);

    const auto t1 = std::chrono::steady_clock::now();
    result.elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    if (!qr.success) {
        result.error = qr.error;
        logMsg(LogCategory::Export, LogLevel::Error,
               QStringLiteral("Export failed: %1").arg(qr.error));
        return result;
    }

    result.success = true;
    result.rowsExported = totalRows;

    emit progressChanged(100, QStringLiteral("Export complete – %1 rows in %2 ms")
                                  .arg(formatNumber(result.rowsExported))
                                  .arg(result.elapsedMs));

    logMsg(LogCategory::Export, LogLevel::Info,
           QStringLiteral("Exported %1 rows to '%2' in %3 ms")
               .arg(result.rowsExported)
               .arg(options.filePath)
               .arg(result.elapsedMs));

    return result;
}

} // namespace csvforge
