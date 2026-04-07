#pragma once

#include "data/duckdb/csv_import_service.h"
#include "data/duckdb/duckdb_engine.h"
#include "workers/background_task.h"

namespace csvforge {

class ImportWorker : public BackgroundTask {
    Q_OBJECT
public:
    ImportWorker(DuckDBEngine* engine, const QString& filePath,
                 const QString& tableName, const CSVImportOptions& options,
                 QObject* parent = nullptr);

    void run() override;

signals:
    void importComplete(const CSVImportService::ImportResult& result);

private:
    DuckDBEngine* m_engine;
    QString m_filePath;
    QString m_tableName;
    CSVImportOptions m_options;
};

} // namespace csvforge
