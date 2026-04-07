#pragma once

#include <QObject>
#include <QStringList>

#include "data/duckdb/duckdb_engine.h"

namespace csvforge {

class ExportService : public QObject {
    Q_OBJECT
public:
    explicit ExportService(DuckDBEngine* engine, QObject* parent = nullptr);

    struct ExportOptions {
        QString filePath;
        QString format = QStringLiteral("csv");  // csv, tsv, parquet
        QChar delimiter = QLatin1Char(',');
        bool includeHeader = true;
        QStringList columns;      // empty = all
        QString whereClause;      // empty = all rows
        QString tableName;
    };

    struct ExportResult {
        bool success = false;
        QString error;
        qint64 rowsExported = 0;
        qint64 elapsedMs = 0;
    };

    ExportResult exportData(const ExportOptions& options);

signals:
    void progressChanged(int percent, const QString& message);

private:
    DuckDBEngine* m_engine;
};

} // namespace csvforge
