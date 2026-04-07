#pragma once

#include <QObject>
#include <QPair>
#include <QString>
#include <QVariant>
#include <QVector>

#include "data/duckdb/duckdb_engine.h"
#include "data/types/column_schema.h"

namespace csvforge {

struct ColumnStats {
    QString columnName;
    ColumnType type = ColumnType::Unknown;
    qint64 totalRows = 0;
    qint64 filteredRows = 0;
    qint64 nullCount = 0;
    qint64 nonNullCount = 0;
    qint64 uniqueCount = 0;
    QVariant minValue;
    QVariant maxValue;
    double sum = 0;
    double average = 0;
    double median = 0;
    QVector<QPair<QString, qint64>> topValues;

    // Selection stats
    double selectionSum = 0;
    double selectionAvg = 0;
    qint64 selectionCount = 0;
};

class StatsService : public QObject {
    Q_OBJECT
public:
    explicit StatsService(DuckDBEngine* engine, QObject* parent = nullptr);

    ColumnStats computeStats(const QString& tableName, const QString& columnName,
                              const QString& whereClause = QString());

    qint64 rowCount(const QString& tableName);
    qint64 filteredRowCount(const QString& tableName, const QString& whereClause);

    ColumnStats computeSelectionStats(const QString& tableName,
                                       const QString& columnName,
                                       const QVector<qint64>& rowIndices);

private:
    DuckDBEngine* m_engine;
};

} // namespace csvforge
