#include "workers/stats_worker.h"

#include "utils/logging.h"
#include "utils/perf_timer.h"

namespace csvforge {

StatsWorker::StatsWorker(DuckDBEngine* engine, const QString& tableName,
                         const QString& columnName,
                         const QString& whereClause, QObject* parent)
    : BackgroundTask(parent)
    , m_engine(engine)
    , m_tableName(tableName)
    , m_columnName(columnName)
    , m_whereClause(whereClause)
{
    setTaskName(QStringLiteral("Stats: %1.%2").arg(tableName, columnName));
}

void StatsWorker::run()
{
    emit started(taskName());
    PERF_TIMER(QStringLiteral("StatsWorker::run"));

    if (checkCancelled()) return;

    emitProgress(0, QStringLiteral("Computing statistics for '%1'...").arg(m_columnName));

    Logger::instance().info(LogCategory::Database,
                            QStringLiteral("Computing stats for %1.%2")
                                .arg(m_tableName, m_columnName));

    StatsService statsService(m_engine);

    if (checkCancelled()) return;

    ColumnStats stats = statsService.computeStats(m_tableName, m_columnName,
                                                  m_whereClause);

    if (checkCancelled()) return;

    emitProgress(100, QStringLiteral("Statistics ready"));

    Logger::instance().info(LogCategory::Database,
                            QStringLiteral("Stats complete for %1.%2: %3 rows, %4 unique")
                                .arg(m_tableName, m_columnName)
                                .arg(stats.totalRows)
                                .arg(stats.uniqueCount));

    emit statsReady(stats);
    emit finished(true, QStringLiteral("Stats computed for %1").arg(m_columnName));
}

} // namespace csvforge
