#pragma once

#include <chrono>
#include <QString>

namespace csvforge {

class PerfTimer {
public:
    explicit PerfTimer(const QString& operation);
    ~PerfTimer();

    qint64 elapsedMs() const;
    void checkpoint(const QString& label);

private:
    QString m_operation;
    std::chrono::steady_clock::time_point m_start;
};

// Macro for easy use
#define PERF_TIMER(name) csvforge::PerfTimer _perf_timer_(name)

} // namespace csvforge
