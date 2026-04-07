#pragma once

#include <QString>
#include <QFile>
#include <QMutex>
#include <memory>

namespace csvforge {

enum class LogCategory {
    General,
    Database,
    Import,
    Export,
    UI,
    Performance,
    Filter,
    Session
};

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

class Logger {
public:
    static Logger& instance();

    void log(LogCategory category, LogLevel level, const QString& message);
    void setLogLevel(LogLevel level);
    void enableFileLogging(const QString& filePath);
    void disableFileLogging();

    // Convenience helpers
    void debug(LogCategory cat, const QString& msg);
    void info(LogCategory cat, const QString& msg);
    void warning(LogCategory cat, const QString& msg);
    void error(LogCategory cat, const QString& msg);
    void critical(LogCategory cat, const QString& msg);

    static QString categoryToString(LogCategory cat);
    static QString levelToString(LogLevel level);

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    QString formatMessage(LogCategory category, LogLevel level, const QString& message) const;

    LogLevel m_minLevel = LogLevel::Debug;
    std::unique_ptr<QFile> m_logFile;
    QMutex m_mutex;
};

// Free-function shorthand
inline void logMsg(LogCategory cat, LogLevel level, const QString& msg)
{
    Logger::instance().log(cat, level, msg);
}

} // namespace csvforge
