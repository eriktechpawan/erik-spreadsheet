#pragma once

#include <QObject>
#include <QString>

#include "data/duckdb/duckdb_connection.h"
#include "data/duckdb/duckdb_engine.h"
#include "data/types/column_schema.h"
#include "utils/csv_sniffer.h"

namespace csvforge {

class CSVImportService : public QObject {
    Q_OBJECT
public:
    explicit CSVImportService(DuckDBEngine* engine, QObject* parent = nullptr);

    struct ImportResult {
        bool success = false;
        QString error;
        QString tableName;
        qint64 rowCount = 0;
        qint64 errorRows = 0;
        ColumnSchemaList schema;
        qint64 elapsedMs = 0;
    };

    ImportResult importCSV(const QString& filePath, const QString& tableName,
                           const CSVImportOptions& options);

    DuckDBConnection::QueryResult previewCSV(const QString& filePath,
                                              int maxRows = 100,
                                              const CSVImportOptions& options = {});

    CSVFormat detectFormat(const QString& filePath);

signals:
    void progressChanged(int percent, const QString& message);

private:
    DuckDBEngine* m_engine;
};

} // namespace csvforge
