#include "csv_sniffer.h"
#include "string_utils.h"

#include <QFile>
#include <QTextStream>
#include <QMap>

namespace csvforge {

CSVFormat CSVSniffer::detect(const QString& filePath, int sampleLines)
{
    CSVFormat fmt;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return fmt;
    }

    QTextStream stream(&file);
    QStringList lines;
    while (!stream.atEnd() && lines.size() < sampleLines) {
        lines.append(stream.readLine());
    }
    file.close();

    if (lines.isEmpty()) {
        return fmt;
    }

    fmt.delimiter = guessDelimiter(lines);
    fmt.hasHeader = guessHasHeader(lines, fmt.delimiter);
    return fmt;
}

QChar CSVSniffer::guessDelimiter(const QStringList& lines)
{
    // Candidates in priority order
    const QList<QChar> candidates = {
        QLatin1Char(','),
        QLatin1Char('\t'),
        QLatin1Char(';'),
        QLatin1Char('|')
    };

    QChar best = QLatin1Char(',');
    int bestScore = 0;

    for (const QChar& candidate : candidates) {
        // Count occurrences per line and compute consistency score.
        QList<int> counts;
        for (const QString& line : lines) {
            counts.append(line.count(candidate));
        }

        if (counts.isEmpty() || counts.first() == 0) {
            continue;
        }

        // A good delimiter appears roughly the same number of times in every line.
        const int expected = counts.first();
        int consistent = 0;
        for (int c : counts) {
            if (c == expected && c > 0) {
                ++consistent;
            }
        }

        const int score = consistent * expected;
        if (score > bestScore) {
            bestScore = score;
            best = candidate;
        }
    }

    return best;
}

bool CSVSniffer::guessHasHeader(const QStringList& lines, QChar delimiter)
{
    if (lines.size() < 2) {
        return true; // default assumption
    }

    // Heuristic: if any field in the first row is numeric and the corresponding
    // field in the second row is also numeric, the first row is probably not a header.
    const QStringList firstRow  = lines.at(0).split(delimiter);
    const QStringList secondRow = lines.at(1).split(delimiter);

    int numericInFirst  = 0;
    int numericInSecond = 0;
    const int cols = qMin(firstRow.size(), secondRow.size());

    for (int i = 0; i < cols; ++i) {
        if (isNumericString(firstRow.at(i).trimmed())) {
            ++numericInFirst;
        }
        if (isNumericString(secondRow.at(i).trimmed())) {
            ++numericInSecond;
        }
    }

    // If the first row has significantly fewer numeric fields, it is likely a header.
    return numericInFirst < numericInSecond;
}

} // namespace csvforge
