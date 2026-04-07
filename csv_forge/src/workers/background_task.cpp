#include "workers/background_task.h"

#include "utils/logging.h"

namespace csvforge {

BackgroundTask::BackgroundTask(QObject* parent)
    : QObject(parent)
{
    setAutoDelete(false);
}

BackgroundTask::~BackgroundTask() = default;

void BackgroundTask::cancel()
{
    m_cancelled.store(true, std::memory_order_release);
    Logger::instance().info(LogCategory::General,
                            QStringLiteral("Task cancelled: %1").arg(m_taskName));
}

bool BackgroundTask::isCancelled() const
{
    return m_cancelled.load(std::memory_order_acquire);
}

QString BackgroundTask::taskName() const
{
    return m_taskName;
}

void BackgroundTask::setTaskName(const QString& name)
{
    m_taskName = name;
}

void BackgroundTask::emitProgress(int percent, const QString& message)
{
    if (!isCancelled()) {
        emit progressChanged(percent, message);
    }
}

bool BackgroundTask::checkCancelled()
{
    if (isCancelled()) {
        emit error(QStringLiteral("Task '%1' was cancelled").arg(m_taskName));
        emit finished(false, QStringLiteral("Cancelled"));
        return true;
    }
    return false;
}

} // namespace csvforge
