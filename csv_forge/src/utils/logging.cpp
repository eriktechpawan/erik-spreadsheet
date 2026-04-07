#include "logging.h"

#include <QDateTime>
#include <QDebug>
#include <QMutexLocker>
#include <QTextStream>

namespace csvforge {

Logger& Logger::instance()
{
    static Logger s_instance;
    return s_instance;
}

Logger::Logger() = default;
Logger::~Logger() = default;

void Logger::setLogLevel(LogLevel level)
{
    QMutexLocker locker(&m_mutex);
    m_minLevel = level;
}

void Logger::enableFileLogging(const QString& filePath)
{
    QMutexLocker locker(&m_mutex);
    m_logFile = std::make_unique<QFile>(filePath);
    if (!m_logFile->open(QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Logger: failed to open log file" << filePath;
        m_logFile.reset();
    }
}

void Logger::disableFileLogging()
{
    QMutexLocker locker(&m_mutex);
    if (m_logFile) {
        m_logFile->close();
        m_logFile.reset();
    }
}

void Logger::log(LogCategory category, LogLevel level, const QString& message)
{
    QMutexLocker locker(&m_mutex);

    if (level < m_minLevel) {
        return;
    }

    const QString formatted = formatMessage(category, level, message);

    // Route to the appropriate Qt message handler
    switch (level) {
    case LogLevel::Debug:
        qDebug().noquote() << formatted;
        break;
    case LogLevel::Info:
        qInfo().noquote() << formatted;
        break;
    case LogLevel::Warning:
        qWarning().noquote() << formatted;
        break;
    case LogLevel::Error:
    case LogLevel::Critical:
        qCritical().noquote() << formatted;
        break;
    }

    // Optional file output
    if (m_logFile && m_logFile->isOpen()) {
        QTextStream stream(m_logFile.get());
        stream << formatted << '\n';
        stream.flush();
    }
}

void Logger::debug(LogCategory cat, const QString& msg)    { log(cat, LogLevel::Debug, msg); }
void Logger::info(LogCategory cat, const QString& msg)     { log(cat, LogLevel::Info, msg); }
void Logger::warning(LogCategory cat, const QString& msg)  { log(cat, LogLevel::Warning, msg); }
void Logger::error(LogCategory cat, const QString& msg)    { log(cat, LogLevel::Error, msg); }
void Logger::critical(LogCategory cat, const QString& msg) { log(cat, LogLevel::Critical, msg); }

QString Logger::formatMessage(LogCategory category, LogLevel level, const QString& message) const
{
    const QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    return QStringLiteral("[%1] [%2] [%3] %4")
        .arg(timestamp, levelToString(level), categoryToString(category), message);
}

QString Logger::categoryToString(LogCategory cat)
{
    switch (cat) {
    case LogCategory::General:     return QStringLiteral("General");
    case LogCategory::Database:    return QStringLiteral("Database");
    case LogCategory::Import:      return QStringLiteral("Import");
    case LogCategory::Export:      return QStringLiteral("Export");
    case LogCategory::UI:          return QStringLiteral("UI");
    case LogCategory::Performance: return QStringLiteral("Performance");
    case LogCategory::Filter:      return QStringLiteral("Filter");
    case LogCategory::Session:     return QStringLiteral("Session");
    }
    return QStringLiteral("Unknown");
}

QString Logger::levelToString(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug:    return QStringLiteral("DEBUG");
    case LogLevel::Info:     return QStringLiteral("INFO");
    case LogLevel::Warning:  return QStringLiteral("WARN");
    case LogLevel::Error:    return QStringLiteral("ERROR");
    case LogLevel::Critical: return QStringLiteral("CRIT");
    }
    return QStringLiteral("???");
}

} // namespace csvforge
