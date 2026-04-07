#include "data/duckdb/lookup_service.h"
#include "utils/logging.h"
#include "utils/perf_timer.h"
#include "utils/string_utils.h"

#include <QFileInfo>
#include <chrono>

namespace csvforge {

LookupService::LookupService(DuckDBEngine* engine, QObject* parent)
    : QObject(parent)
    , m_engine(engine)
{
}

// ---------------------------------------------------------------------------
// Ensure the target data source is available
// ---------------------------------------------------------------------------

QString LookupService::ensureTargetLoaded(const LookupConfig& config)
{
    // If a target table is already specified and exists, use it directly.
    if (!config.targetTable.isEmpty() && m_engine->tableExists(config.targetTable)) {
        return config.targetTable;
    }

    // If a target CSV file is provided, use read_csv_auto inline.
    if (!config.targetFile.isEmpty() && QFileInfo::exists(config.targetFile)) {
        // Return a read_csv_auto() expression instead of a table name.
        return QStringLiteral("read_csv_auto('%1')").arg(escapeSQL(config.targetFile));
    }

    // If targetTable was given but doesn't exist, try loading targetFile into it.
    if (!config.targetTable.isEmpty() && !config.targetFile.isEmpty()) {
        CSVImportOptions defaultOpts;
        const QString err = m_engine->loadCSVAsTable(config.targetFile,
                                                     config.targetTable, defaultOpts);
        if (err.isEmpty()) {
            return config.targetTable;
        }
        logMsg(LogCategory::Database, LogLevel::Warning,
               QStringLiteral("Failed to load target table '%1': %2")
                   .arg(config.targetTable, err));
    }

    return {};
}

// ---------------------------------------------------------------------------
// Build JOIN query
// ---------------------------------------------------------------------------

QString LookupService::buildLookupQuery(const LookupConfig& config)
{
    if (!config.isValid()) {
        return {};
    }

    // Resolve the target reference (table name or read_csv_auto expression)
    const QString targetRef = ensureTargetLoaded(config);
    if (targetRef.isEmpty()) {
        return {};
    }

    // Determine whether targetRef is an expression or a plain table name.
    const bool targetIsExpr = targetRef.startsWith(QStringLiteral("read_csv_auto("));
    const QString targetAlias = QStringLiteral("__target__");
    const QString targetFrom = targetIsExpr
                                   ? QStringLiteral("%1 AS %2").arg(targetRef, targetAlias)
                                   : QStringLiteral("%1 AS %2").arg(quoteName(targetRef), targetAlias);

    const QString srcAlias = QStringLiteral("__source__");

    // Columns to return from target — prefer returnColumnsDetailed with aliases
    QStringList returnColExpressions;
    if (!config.returnColumnsDetailed.empty()) {
        for (const auto& rc : config.returnColumnsDetailed) {
            QString expr = QStringLiteral("%1.%2").arg(targetAlias, quoteName(rc.sourceColumn));
            if (!rc.outputAlias.isEmpty() && rc.outputAlias != rc.sourceColumn) {
                expr += QStringLiteral(" AS %1").arg(quoteName(rc.outputAlias));
            }
            returnColExpressions << expr;
        }
    } else if (config.returnColumns.isEmpty()) {
        returnColExpressions << QStringLiteral("%1.*").arg(targetAlias);
    } else {
        for (const QString& col : config.returnColumns) {
            returnColExpressions << QStringLiteral("%1.%2").arg(targetAlias, quoteName(col));
        }
    }

    // Build SELECT list: all source columns + requested target columns.
    // Include the target key column with a unique alias for unmatched-row detection.
    static const QString kTargetKeyAlias = QStringLiteral("__lookup_target_key__");
    const QString selectList = QStringLiteral("%1.*, %2.%3 AS %4, %5")
                                   .arg(srcAlias,
                                        targetAlias, quoteName(config.targetKeyColumn),
                                        quoteName(kTargetKeyAlias),
                                        returnColExpressions.join(QStringLiteral(", ")));

    // JOIN condition
    const QString joinCond = QStringLiteral("%1.%2 = %3.%4")
                                 .arg(srcAlias, quoteName(config.sourceKeyColumn),
                                      targetAlias, quoteName(config.targetKeyColumn));

    // JOIN type — ExactMatch uses INNER JOIN (drops unmatched rows),
    // LeftJoin uses LEFT JOIN (keeps all source rows).
    const QString joinType = (config.type == LookupType::LeftJoin)
                                 ? QStringLiteral("LEFT JOIN")
                                 : QStringLiteral("INNER JOIN");

    const QString sql = QStringLiteral("SELECT %1 FROM %2 AS %3 %4 %5 ON %6")
                            .arg(selectList,
                                 quoteName(config.sourceTable), srcAlias,
                                 joinType, targetFrom, joinCond);

    return sql;
}

// ---------------------------------------------------------------------------
// Execute lookup
// ---------------------------------------------------------------------------

LookupService::LookupResult LookupService::executeLookup(const LookupConfig& config)
{
    LookupResult result;
    const auto t0 = std::chrono::steady_clock::now();

    if (!m_engine || !m_engine->isInitialized()) {
        result.error = QStringLiteral("Engine not initialized");
        return result;
    }

    const QString joinSQL = buildLookupQuery(config);
    if (joinSQL.isEmpty()) {
        result.error = QStringLiteral("Invalid lookup configuration or target not found");
        return result;
    }

    // Determine result table name
    result.resultTableName = config.resultTableName.isEmpty()
        ? QStringLiteral("lookup_%1").arg(
              QString::number(
                  std::chrono::steady_clock::now().time_since_epoch().count()))
        : config.resultTableName;

    const QString createSQL = QStringLiteral("CREATE TABLE %1 AS %2")
                                  .arg(quoteName(result.resultTableName), joinSQL);

    auto qr = m_engine->executeQuery(createSQL);
    if (!qr.success) {
        result.error = qr.error;
        logMsg(LogCategory::Database, LogLevel::Error,
               QStringLiteral("Lookup failed: %1").arg(qr.error));
        return result;
    }

    // Compute match statistics
    const qint64 totalRows = m_engine->getRowCount(result.resultTableName);

    // Count unmatched rows using the disambiguated target key alias.
    // After a LEFT JOIN the target key will be NULL for unmatched rows.
    const QString unmatchedSQL = QStringLiteral(
        "SELECT COUNT(*) FROM %1 WHERE %2 IS NULL")
        .arg(quoteName(result.resultTableName),
             quoteName(QStringLiteral("__lookup_target_key__")));

    const qint64 unmatched = m_engine->connection().executeCount(unmatchedSQL);
    result.unmatchedRows = (unmatched >= 0) ? unmatched : 0;
    result.matchedRows = totalRows - result.unmatchedRows;
    result.success = true;

    const auto t1 = std::chrono::steady_clock::now();
    result.elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    logMsg(LogCategory::Database, LogLevel::Info,
           QStringLiteral("Lookup completed → %1 (%2 matched, %3 unmatched) in %4 ms")
               .arg(result.resultTableName)
               .arg(result.matchedRows)
               .arg(result.unmatchedRows)
               .arg(result.elapsedMs));

    return result;
}

// ---------------------------------------------------------------------------
// Preview lookup
// ---------------------------------------------------------------------------

DuckDBConnection::QueryResult LookupService::previewLookup(
    const LookupConfig& config, int maxRows)
{
    if (!m_engine || !m_engine->isInitialized()) {
        return { false, QStringLiteral("Engine not initialized"), {}, {}, {}, 0, 0 };
    }

    const QString joinSQL = buildLookupQuery(config);
    if (joinSQL.isEmpty()) {
        return { false, QStringLiteral("Invalid lookup configuration"), {}, {}, {}, 0, 0 };
    }

    const QString limitedSQL = QStringLiteral("SELECT * FROM (%1) AS __preview__ LIMIT %2")
                                   .arg(joinSQL)
                                   .arg(maxRows);

    return m_engine->executeQuery(limitedSQL);
}

} // namespace csvforge
