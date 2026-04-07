#pragma once

#include "data/duckdb/duckdb_engine.h"
#include "data/duckdb/export_service.h"
#include "workers/background_task.h"

namespace csvforge {

class ExportWorker : public BackgroundTask {
    Q_OBJECT
public:
    ExportWorker(DuckDBEngine* engine, const ExportService::ExportOptions& options,
                 QObject* parent = nullptr);

    void run() override;

signals:
    void exportComplete(const ExportService::ExportResult& result);

private:
    DuckDBEngine* m_engine;
    ExportService::ExportOptions m_options;
};

} // namespace csvforge
