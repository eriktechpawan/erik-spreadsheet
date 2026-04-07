#pragma once

#include "data/duckdb/duckdb_engine.h"
#include "data/duckdb/stats_service.h"
#include "workers/background_task.h"

namespace csvforge {

class StatsWorker : public BackgroundTask {
    Q_OBJECT
public:
    StatsWorker(DuckDBEngine* engine, const QString& tableName,
                const QString& columnName,
                const QString& whereClause = QString(),
                QObject* parent = nullptr);

    void run() override;

signals:
    void statsReady(const ColumnStats& stats);

private:
    DuckDBEngine* m_engine;
    QString m_tableName;
    QString m_columnName;
    QString m_whereClause;
};

} // namespace csvforge
