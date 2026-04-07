#include "workers/task_dispatcher.h"

#include <algorithm>

#include "utils/logging.h"

namespace csvforge {

TaskDispatcher::TaskDispatcher(QObject* parent)
    : QObject(parent)
    , m_pool(new QThreadPool(this))
{
    m_pool->setMaxThreadCount(QThread::idealThreadCount());
    Logger::instance().info(LogCategory::General,
                            QStringLiteral("TaskDispatcher created with %1 threads")
                                .arg(m_pool->maxThreadCount()));
}

TaskDispatcher::~TaskDispatcher()
{
    cancelAll();
    m_pool->waitForDone();
}

void TaskDispatcher::submit(BackgroundTask* task)
{
    if (!task) {
        Logger::instance().warning(LogCategory::General,
                                   QStringLiteral("Null task submitted to dispatcher"));
        return;
    }

    {
        QMutexLocker locker(&m_mutex);
        m_activeTasks.push_back(task);
    }

    connectTask(task);

    Logger::instance().info(LogCategory::General,
                            QStringLiteral("Submitting task: %1").arg(task->taskName()));

    m_pool->start(task);
}

void TaskDispatcher::cancelAll()
{
    QMutexLocker locker(&m_mutex);
    for (auto* task : m_activeTasks) {
        task->cancel();
    }
    Logger::instance().info(LogCategory::General,
                            QStringLiteral("Cancelled all %1 active tasks")
                                .arg(m_activeTasks.size()));
}

int TaskDispatcher::activeTaskCount() const
{
    QMutexLocker locker(&m_mutex);
    return static_cast<int>(m_activeTasks.size());
}

bool TaskDispatcher::isIdle() const
{
    QMutexLocker locker(&m_mutex);
    return m_activeTasks.empty();
}

void TaskDispatcher::setMaxThreads(int count)
{
    if (count > 0) {
        m_pool->setMaxThreadCount(count);
        Logger::instance().info(LogCategory::General,
                                QStringLiteral("Max threads set to %1").arg(count));
    }
}

void TaskDispatcher::connectTask(BackgroundTask* task)
{
    connect(task, &BackgroundTask::started, this,
            [this](const QString& name) {
                emit taskStarted(name);
            });

    connect(task, &BackgroundTask::progressChanged, this,
            [this, task](int percent, const QString& message) {
                emit taskProgress(task->taskName(), percent, message);
            });

    connect(task, &BackgroundTask::finished, this,
            [this, task](bool success, const QString& result) {
                emit taskFinished(task->taskName(), success, result);
                removeTask(task);
            });

    connect(task, &BackgroundTask::error, this,
            [this, task](const QString& errorMessage) {
                Logger::instance().error(LogCategory::General,
                                         QStringLiteral("Task '%1' error: %2")
                                             .arg(task->taskName(), errorMessage));
            });
}

void TaskDispatcher::removeTask(BackgroundTask* task)
{
    bool allDone = false;
    {
        QMutexLocker locker(&m_mutex);
        m_activeTasks.erase(
            std::remove(m_activeTasks.begin(), m_activeTasks.end(), task),
            m_activeTasks.end());
        allDone = m_activeTasks.empty();
    }

    if (allDone) {
        emit allTasksCompleted();
    }
}

} // namespace csvforge
