#include "workers/query_worker.h"

#include "utils/logging.h"
#include "utils/perf_timer.h"

namespace csvforge {

// --- QueryWorker ---

QueryWorker::QueryWorker(DuckDBEngine* engine, const QString& sql,
                         QObject* parent)
    : BackgroundTask(parent)
    , m_engine(engine)
    , m_sql(sql)
{
    setTaskName(QStringLiteral("Query"));
}

void QueryWorker::run()
{
    emit started(taskName());
    PERF_TIMER(QStringLiteral("QueryWorker::run"));

    if (checkCancelled()) return;

    emitProgress(0, QStringLiteral("Executing query..."));

    auto result = m_engine->executeQuery(m_sql);

    if (checkCancelled()) return;

    if (!result.success) {
        Logger::instance().error(LogCategory::Database,
                                 QStringLiteral("Query failed: %1").arg(result.error));
        emit error(result.error);
        emit finished(false, result.error);
        return;
    }

    emitProgress(50, QStringLiteral("Converting results..."));

    QVector<QVector<QVariant>> rows;
    rows.reserve(static_cast<int>(result.rows.size()));

    for (const auto& srcRow : result.rows) {
        if (checkCancelled()) return;

        QVector<QVariant> row;
        row.reserve(static_cast<int>(srcRow.size()));
        for (const auto& val : srcRow) {
            row.append(val);
        }
        rows.append(row);
    }

    emitProgress(100, QStringLiteral("Query complete"));

    Logger::instance().info(LogCategory::Database,
                            QStringLiteral("Query returned %1 rows")
                                .arg(rows.size()));

    emit dataReady(0, rows);
    emit finished(true, QStringLiteral("Returned %1 rows").arg(rows.size()));
}

// --- CountQueryWorker ---

CountQueryWorker::CountQueryWorker(DuckDBEngine* engine,
                                   const QString& tableName,
                                   const QString& whereClause,
                                   QObject* parent)
    : BackgroundTask(parent)
    , m_engine(engine)
    , m_tableName(tableName)
    , m_whereClause(whereClause)
{
    setTaskName(QStringLiteral("CountQuery"));
}

void CountQueryWorker::run()
{
    emit started(taskName());
    PERF_TIMER(QStringLiteral("CountQueryWorker::run"));

    if (checkCancelled()) return;

    emitProgress(0, QStringLiteral("Counting rows..."));

    qint64 totalRows = m_engine->getRowCount(m_tableName);

    if (checkCancelled()) return;

    emitProgress(50, QStringLiteral("Counting filtered rows..."));

    qint64 filteredRows = totalRows;
    if (!m_whereClause.isEmpty()) {
        filteredRows = m_engine->getFilteredRowCount(m_tableName, m_whereClause);
    }

    if (checkCancelled()) return;

    emitProgress(100, QStringLiteral("Count complete"));

    Logger::instance().info(LogCategory::Database,
                            QStringLiteral("Row counts: total=%1, filtered=%2")
                                .arg(totalRows)
                                .arg(filteredRows));

    emit countReady(totalRows, filteredRows);
    emit finished(true, QStringLiteral("Total: %1, Filtered: %2")
                            .arg(totalRows)
                            .arg(filteredRows));
}

} // namespace csvforge
