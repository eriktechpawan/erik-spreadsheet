#pragma once

#include <QObject>
#include <QRunnable>
#include <QString>
#include <atomic>

namespace csvforge {

class BackgroundTask : public QObject, public QRunnable {
    Q_OBJECT
public:
    explicit BackgroundTask(QObject* parent = nullptr);
    ~BackgroundTask() override;

    // QRunnable
    void run() override = 0;

    // Cancellation
    void cancel();
    bool isCancelled() const;

    // Task info
    QString taskName() const;
    void setTaskName(const QString& name);

signals:
    void started(const QString& taskName);
    void progressChanged(int percent, const QString& message);
    void finished(bool success, const QString& result);
    void error(const QString& errorMessage);

protected:
    void emitProgress(int percent, const QString& message);
    bool checkCancelled();

private:
    std::atomic<bool> m_cancelled{false};
    QString m_taskName;
};

} // namespace csvforge
