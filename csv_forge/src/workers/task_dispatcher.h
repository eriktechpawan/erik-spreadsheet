#pragma once

#include <QMutex>
#include <QObject>
#include <QThreadPool>
#include <vector>

#include "workers/background_task.h"

namespace csvforge {

class TaskDispatcher : public QObject {
    Q_OBJECT
public:
    explicit TaskDispatcher(QObject* parent = nullptr);
    ~TaskDispatcher() override;

    // Submit tasks
    void submit(BackgroundTask* task);

    // Cancel all running tasks
    void cancelAll();

    // State
    int activeTaskCount() const;
    bool isIdle() const;

    // Configuration
    void setMaxThreads(int count);

signals:
    void taskStarted(const QString& taskName);
    void taskProgress(const QString& taskName, int percent, const QString& message);
    void taskFinished(const QString& taskName, bool success, const QString& result);
    void allTasksCompleted();

private:
    QThreadPool* m_pool;
    std::vector<BackgroundTask*> m_activeTasks;
    mutable QMutex m_mutex;

    void connectTask(BackgroundTask* task);
    void removeTask(BackgroundTask* task);
};

} // namespace csvforge
