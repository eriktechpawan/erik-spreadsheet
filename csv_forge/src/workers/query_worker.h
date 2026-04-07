#pragma once

#include <QVariant>
#include <QVector>

#include "data/duckdb/duckdb_engine.h"
#include "workers/background_task.h"

namespace csvforge {

class QueryWorker : public BackgroundTask {
    Q_OBJECT
public:
    QueryWorker(DuckDBEngine* engine, const QString& sql,
                QObject* parent = nullptr);

    void run() override;

signals:
    void dataReady(qint64 startRow, const QVector<QVector<QVariant>>& rows);

private:
    DuckDBEngine* m_engine;
    QString m_sql;
};

class CountQueryWorker : public BackgroundTask {
    Q_OBJECT
public:
    CountQueryWorker(DuckDBEngine* engine, const QString& tableName,
                     const QString& whereClause = QString(),
                     QObject* parent = nullptr);

    void run() override;

signals:
    void countReady(qint64 totalRows, qint64 filteredRows);

private:
    DuckDBEngine* m_engine;
    QString m_tableName;
    QString m_whereClause;
};

} // namespace csvforge
