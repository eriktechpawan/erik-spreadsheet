#include "data/duckdb/stats_service.h"
#include "utils/logging.h"
#include "utils/perf_timer.h"
#include "utils/string_utils.h"

namespace csvforge {

StatsService::StatsService(DuckDBEngine* engine, QObject* parent)
    : QObject(parent)
    , m_engine(engine)
{
}

// ---------------------------------------------------------------------------
// Full column statistics
// ---------------------------------------------------------------------------

ColumnStats StatsService::computeStats(const QString& tableName,
                                       const QString& columnName,
                                       const QString& whereClause)
{
    PERF_TIMER(QStringLiteral("computeStats(%1.%2)").arg(tableName, columnName));

    ColumnStats stats;
    stats.columnName = columnName;

    if (!m_engine || !m_engine->isInitialized()) return stats;

    const QString tbl = quoteName(tableName);
    const QString col = quoteName(columnName);
    const QString filter = whereClause.isEmpty()
                               ? QString()
                               : QStringLiteral(" WHERE %1").arg(whereClause);

    // Determine column type from schema
    const auto schema = m_engine->getTableSchema(tableName);
    for (const auto& cs : schema) {
        if (cs.name == columnName) {
            stats.type = cs.type;
            break;
        }
    }

    // Combined aggregate query for basic stats
    const bool isNumeric = (stats.type == ColumnType::Integer
                            || stats.type == ColumnType::Double);

    QString aggParts = QStringLiteral(
        "COUNT(*) AS total_rows, "
        "COUNT(%1) AS non_null_count, "
        "COUNT(*) - COUNT(%1) AS null_count, "
        "COUNT(DISTINCT %1) AS unique_count, "
        "MIN(%1) AS min_val, "
        "MAX(%1) AS max_val").arg(col);

    if (isNumeric) {
        aggParts += QStringLiteral(
            ", SUM(CAST(%1 AS DOUBLE)) AS sum_val"
            ", AVG(CAST(%1 AS DOUBLE)) AS avg_val"
            ", PERCENTILE_CONT(0.5) WITHIN GROUP (ORDER BY CAST(%1 AS DOUBLE)) AS median_val")
            .arg(col);
    }

    const QString sql = QStringLiteral("SELECT %1 FROM %2%3")
                            .arg(aggParts, tbl, filter);

    auto qr = m_engine->executeQuery(sql);
    if (qr.success && !qr.rows.empty()) {
        const auto& row = qr.rows[0];
        int i = 0;
        stats.totalRows    = row[i++].toLongLong();
        stats.nonNullCount = row[i++].toLongLong();
        stats.nullCount    = row[i++].toLongLong();
        stats.uniqueCount  = row[i++].toLongLong();
        stats.minValue     = row[i++];
        stats.maxValue     = row[i++];
        if (isNumeric) {
            stats.sum     = row[i++].toDouble();
            stats.average = row[i++].toDouble();
            stats.median  = row[i++].toDouble();
        }
        stats.filteredRows = stats.totalRows;
    }

    // Top 20 most frequent values
    const QString topSql = QStringLiteral(
        "SELECT CAST(%1 AS VARCHAR) AS val, COUNT(*) AS cnt "
        "FROM %2%3 "
        "WHERE %1 IS NOT NULL "
        "GROUP BY val ORDER BY cnt DESC LIMIT 20")
        .arg(col, tbl,
             whereClause.isEmpty()
                 ? QString()
                 : QStringLiteral(" ") /* WHERE already embedded via filter */);

    // Rebuild with proper WHERE combination for top values
    QString topWhere = QStringLiteral("%1 IS NOT NULL").arg(col);
    if (!whereClause.isEmpty()) {
        topWhere = QStringLiteral("(%1) AND %2").arg(whereClause, topWhere);
    }

    const QString topSql2 = QStringLiteral(
        "SELECT CAST(%1 AS VARCHAR) AS val, COUNT(*) AS cnt "
        "FROM %2 WHERE %3 GROUP BY val ORDER BY cnt DESC LIMIT 20")
        .arg(col, tbl, topWhere);

    auto topQr = m_engine->executeQuery(topSql2);
    if (topQr.success) {
        stats.topValues.reserve(static_cast<int>(topQr.rows.size()));
        for (const auto& r : topQr.rows) {
            if (r.size() >= 2) {
                stats.topValues.append({ r[0].toString(), r[1].toLongLong() });
            }
        }
    }

    return stats;
}

// ---------------------------------------------------------------------------
// Quick counts
// ---------------------------------------------------------------------------

qint64 StatsService::rowCount(const QString& tableName)
{
    return m_engine ? m_engine->getRowCount(tableName) : -1;
}

qint64 StatsService::filteredRowCount(const QString& tableName,
                                      const QString& whereClause)
{
    return m_engine ? m_engine->getFilteredRowCount(tableName, whereClause) : -1;
}

// ---------------------------------------------------------------------------
// Selection stats (specific row indices)
// ---------------------------------------------------------------------------

ColumnStats StatsService::computeSelectionStats(const QString& tableName,
                                                const QString& columnName,
                                                const QVector<qint64>& rowIndices)
{
    ColumnStats stats;
    stats.columnName = columnName;

    if (!m_engine || !m_engine->isInitialized() || rowIndices.isEmpty()) {
        return stats;
    }

    const QString tbl = quoteName(tableName);
    const QString col = quoteName(columnName);

    // Use a CTE with rowid filtering. DuckDB tables have implicit rowid.
    QStringList idStrs;
    idStrs.reserve(rowIndices.size());
    for (qint64 idx : rowIndices) {
        idStrs << QString::number(idx);
    }

    const QString idList = idStrs.join(QLatin1Char(','));

    // Determine column type
    const auto schema = m_engine->getTableSchema(tableName);
    for (const auto& cs : schema) {
        if (cs.name == columnName) {
            stats.type = cs.type;
            break;
        }
    }

    const bool isNumeric = (stats.type == ColumnType::Integer
                            || stats.type == ColumnType::Double);

    QString sql;
    if (isNumeric) {
        sql = QStringLiteral(
            "SELECT COUNT(%1) AS cnt, "
            "SUM(CAST(%1 AS DOUBLE)) AS s, "
            "AVG(CAST(%1 AS DOUBLE)) AS a "
            "FROM (SELECT %1, ROW_NUMBER() OVER () - 1 AS rn FROM %2) sub "
            "WHERE rn IN (%3)")
            .arg(col, tbl, idList);
    } else {
        sql = QStringLiteral(
            "SELECT COUNT(%1) AS cnt "
            "FROM (SELECT %1, ROW_NUMBER() OVER () - 1 AS rn FROM %2) sub "
            "WHERE rn IN (%3)")
            .arg(col, tbl, idList);
    }

    auto qr = m_engine->executeQuery(sql);
    if (qr.success && !qr.rows.empty()) {
        const auto& row = qr.rows[0];
        stats.selectionCount = row[0].toLongLong();
        if (isNumeric && row.size() >= 3) {
            stats.selectionSum = row[1].toDouble();
            stats.selectionAvg = row[2].toDouble();
        }
    }

    return stats;
}

} // namespace csvforge
