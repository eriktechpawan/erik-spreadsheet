#include "data/duckdb/pivot_service.h"
#include "utils/logging.h"
#include "utils/perf_timer.h"
#include "utils/string_utils.h"

#include <chrono>

namespace csvforge {

PivotService::PivotService(DuckDBEngine* engine, QObject* parent)
    : QObject(parent)
    , m_engine(engine)
{
}

// ---------------------------------------------------------------------------
// Build PIVOT query
// ---------------------------------------------------------------------------

QString PivotService::buildPivotQuery(const PivotConfig& config)
{
    if (!config.isValid()) {
        return {};
    }

    // Build aggregate functions (USING clause)
    QStringList aggClauses;
    for (const auto& vf : config.valueFields) {
        aggClauses << vf.toSQL();
    }

    // Build GROUP BY (row fields)
    QStringList groupCols;
    groupCols.reserve(config.rowFields.size());
    for (const QString& rf : config.rowFields) {
        groupCols << quoteName(rf);
    }

    // Build ON (column fields)
    QStringList onCols;
    onCols.reserve(config.columnFields.size());
    for (const QString& cf : config.columnFields) {
        onCols << quoteName(cf);
    }

    // DuckDB PIVOT syntax:
    // PIVOT source_table ON col_field USING agg(val_field) GROUP BY row_fields
    QString sql = QStringLiteral("PIVOT %1 ON %2 USING %3")
                      .arg(quoteName(config.sourceTable),
                           onCols.join(QStringLiteral(", ")),
                           aggClauses.join(QStringLiteral(", ")));

    if (!groupCols.isEmpty()) {
        sql += QStringLiteral(" GROUP BY %1").arg(groupCols.join(QStringLiteral(", ")));
    }

    return sql;
}

// ---------------------------------------------------------------------------
// Execute pivot
// ---------------------------------------------------------------------------

PivotService::PivotResult PivotService::executePivot(const PivotConfig& config)
{
    PivotResult result;
    const auto t0 = std::chrono::steady_clock::now();

    if (!m_engine || !m_engine->isInitialized()) {
        result.error = QStringLiteral("Engine not initialized");
        return result;
    }

    const QString pivotSQL = buildPivotQuery(config);
    if (pivotSQL.isEmpty()) {
        result.error = QStringLiteral("Invalid pivot configuration");
        return result;
    }

    // Generate a result table name
    result.resultTableName = QStringLiteral("pivot_%1").arg(
        QString::number(
            std::chrono::steady_clock::now().time_since_epoch().count()));

    const QString createSQL = QStringLiteral("CREATE TABLE %1 AS %2")
                                  .arg(quoteName(result.resultTableName), pivotSQL);

    auto qr = m_engine->executeQuery(createSQL);
    if (!qr.success) {
        result.error = qr.error;
        logMsg(LogCategory::Database, LogLevel::Error,
               QStringLiteral("Pivot failed: %1").arg(qr.error));
        return result;
    }

    // Fetch preview data from the result table
    const QString previewSQL = QStringLiteral("SELECT * FROM %1 LIMIT 100")
                                   .arg(quoteName(result.resultTableName));
    result.previewData = m_engine->executeQuery(previewSQL);

    result.success = true;

    const auto t1 = std::chrono::steady_clock::now();
    result.elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    logMsg(LogCategory::Database, LogLevel::Info,
           QStringLiteral("Pivot completed → %1 in %2 ms")
               .arg(result.resultTableName)
               .arg(result.elapsedMs));

    return result;
}

// ---------------------------------------------------------------------------
// Preview pivot (no persistent table)
// ---------------------------------------------------------------------------

PivotService::PivotResult PivotService::previewPivot(const PivotConfig& config,
                                                     int maxRows)
{
    PivotResult result;
    const auto t0 = std::chrono::steady_clock::now();

    if (!m_engine || !m_engine->isInitialized()) {
        result.error = QStringLiteral("Engine not initialized");
        return result;
    }

    const QString pivotSQL = buildPivotQuery(config);
    if (pivotSQL.isEmpty()) {
        result.error = QStringLiteral("Invalid pivot configuration");
        return result;
    }

    const QString limitedSQL = QStringLiteral("SELECT * FROM (%1) AS __pivot__ LIMIT %2")
                                   .arg(pivotSQL)
                                   .arg(maxRows);

    result.previewData = m_engine->executeQuery(limitedSQL);
    result.success = result.previewData.success;
    if (!result.success) {
        result.error = result.previewData.error;
    }

    const auto t1 = std::chrono::steady_clock::now();
    result.elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    return result;
}

// ---------------------------------------------------------------------------
// Distinct values for a column
// ---------------------------------------------------------------------------

QStringList PivotService::getDistinctValues(const QString& tableName,
                                            const QString& columnName,
                                            int maxValues)
{
    if (!m_engine || !m_engine->isInitialized()) return {};

    const QString sql = QStringLiteral(
        "SELECT DISTINCT CAST(%1 AS VARCHAR) AS val FROM %2 "
        "WHERE %1 IS NOT NULL ORDER BY val LIMIT %3")
        .arg(quoteName(columnName), quoteName(tableName))
        .arg(maxValues);

    auto qr = m_engine->executeQuery(sql);
    QStringList values;
    if (qr.success) {
        values.reserve(static_cast<int>(qr.rows.size()));
        for (const auto& row : qr.rows) {
            if (!row.empty()) {
                values << row[0].toString();
            }
        }
    }
    return values;
}

} // namespace csvforge
