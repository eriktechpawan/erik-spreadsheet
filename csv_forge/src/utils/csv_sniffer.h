#pragma once

#include <QChar>
#include <QString>

namespace csvforge {

struct CSVFormat {
    QChar delimiter  = QLatin1Char(',');
    QChar quoteChar  = QLatin1Char('"');
    bool  hasHeader  = true;
    QString encoding = QStringLiteral("UTF-8");
    int   skipRows   = 0;
};

class CSVSniffer {
public:
    /// Analyse the first N lines of filePath and return the detected format.
    static CSVFormat detect(const QString& filePath, int sampleLines = 20);

private:
    static QChar guessDelimiter(const QStringList& lines);
    static bool  guessHasHeader(const QStringList& lines, QChar delimiter);
};

} // namespace csvforge
