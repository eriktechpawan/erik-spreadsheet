#pragma once

#include <QObject>
#include <QStringList>

#include "data/duckdb/duckdb_connection.h"
#include "data/duckdb/duckdb_engine.h"
#include "data/types/pivot_config.h"

namespace csvforge {

class PivotService : public QObject {
    Q_OBJECT
public:
    explicit PivotService(DuckDBEngine* engine, QObject* parent = nullptr);

    struct PivotResult {
        bool success = false;
        QString error;
        QString resultTableName;
        DuckDBConnection::QueryResult previewData;
        qint64 elapsedMs = 0;
    };

    QString buildPivotQuery(const PivotConfig& config);

    PivotResult executePivot(const PivotConfig& config);
    PivotResult previewPivot(const PivotConfig& config, int maxRows = 100);

    QStringList getDistinctValues(const QString& tableName,
                                   const QString& columnName,
                                   int maxValues = 100);

private:
    DuckDBEngine* m_engine;
};

} // namespace csvforge
