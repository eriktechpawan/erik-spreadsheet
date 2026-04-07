#include "workers/export_worker.h"

#include "utils/logging.h"
#include "utils/perf_timer.h"

namespace csvforge {

ExportWorker::ExportWorker(DuckDBEngine* engine,
                           const ExportService::ExportOptions& options,
                           QObject* parent)
    : BackgroundTask(parent)
    , m_engine(engine)
    , m_options(options)
{
    setTaskName(QStringLiteral("Export: %1").arg(options.filePath));
}

void ExportWorker::run()
{
    emit started(taskName());
    PERF_TIMER(QStringLiteral("ExportWorker::run"));

    if (checkCancelled()) return;

    emitProgress(0, QStringLiteral("Starting export to '%1'...").arg(m_options.filePath));

    Logger::instance().info(LogCategory::Export,
                            QStringLiteral("Exporting table '%1' to '%2' (format: %3)")
                                .arg(m_options.tableName, m_options.filePath,
                                     m_options.format));

    ExportService exportService(m_engine);

    // Forward progress from the export service to our own progress signal
    connect(&exportService, &ExportService::progressChanged, this,
            [this](int percent, const QString& message) {
                emitProgress(percent, message);
            });

    if (checkCancelled()) return;

    auto result = exportService.exportData(m_options);

    if (checkCancelled()) return;

    if (result.success) {
        Logger::instance().info(LogCategory::Export,
                                QStringLiteral("Export complete: %1 rows in %2 ms")
                                    .arg(result.rowsExported)
                                    .arg(result.elapsedMs));
        emitProgress(100, QStringLiteral("Export complete"));
        emit exportComplete(result);
        emit finished(true, QStringLiteral("Exported %1 rows").arg(result.rowsExported));
    } else {
        Logger::instance().error(LogCategory::Export,
                                 QStringLiteral("Export failed: %1").arg(result.error));
        emit exportComplete(result);
        emit error(result.error);
        emit finished(false, result.error);
    }
}

} // namespace csvforge
