#include "perf_timer.h"
#include "logging.h"

namespace csvforge {

PerfTimer::PerfTimer(const QString& operation)
    : m_operation(operation)
    , m_start(std::chrono::steady_clock::now())
{
}

PerfTimer::~PerfTimer()
{
    const qint64 ms = elapsedMs();
    Logger::instance().log(LogCategory::Performance, LogLevel::Debug,
                           QStringLiteral("%1 completed in %2 ms").arg(m_operation).arg(ms));
}

qint64 PerfTimer::elapsedMs() const
{
    const auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_start).count();
}

void PerfTimer::checkpoint(const QString& label)
{
    const qint64 ms = elapsedMs();
    Logger::instance().log(LogCategory::Performance, LogLevel::Debug,
                           QStringLiteral("%1 [%2] at %3 ms").arg(m_operation, label).arg(ms));
}

} // namespace csvforge
