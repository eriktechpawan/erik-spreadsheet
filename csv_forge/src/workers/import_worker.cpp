#include "workers/import_worker.h"

#include "utils/logging.h"
#include "utils/perf_timer.h"

namespace csvforge {

ImportWorker::ImportWorker(DuckDBEngine* engine, const QString& filePath,
                           const QString& tableName,
                           const CSVImportOptions& options, QObject* parent)
    : BackgroundTask(parent)
    , m_engine(engine)
    , m_filePath(filePath)
    , m_tableName(tableName)
    , m_options(options)
{
    setTaskName(QStringLiteral("Import: %1").arg(filePath));
}

void ImportWorker::run()
{
    emit started(taskName());
    PERF_TIMER(QStringLiteral("ImportWorker::run"));

    if (checkCancelled()) return;

    emitProgress(0, QStringLiteral("Starting CSV import..."));

    Logger::instance().info(LogCategory::Import,
                            QStringLiteral("Importing '%1' as table '%2'")
                                .arg(m_filePath, m_tableName));

    CSVImportService importService(m_engine);

    // Forward progress from the import service to our own progress signal
    connect(&importService, &CSVImportService::progressChanged, this,
            [this](int percent, const QString& message) {
                emitProgress(percent, message);
            });

    if (checkCancelled()) return;

    auto result = importService.importCSV(m_filePath, m_tableName, m_options);

    if (checkCancelled()) return;

    if (result.success) {
        Logger::instance().info(LogCategory::Import,
                                QStringLiteral("Import complete: %1 rows in %2 ms")
                                    .arg(result.rowCount)
                                    .arg(result.elapsedMs));
        emitProgress(100, QStringLiteral("Import complete"));
        emit importComplete(result);
        emit finished(true, QStringLiteral("Imported %1 rows").arg(result.rowCount));
    } else {
        Logger::instance().error(LogCategory::Import,
                                 QStringLiteral("Import failed: %1").arg(result.error));
        emit importComplete(result);
        emit error(result.error);
        emit finished(false, result.error);
    }
}

} // namespace csvforge
