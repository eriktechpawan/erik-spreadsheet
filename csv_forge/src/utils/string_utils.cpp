#include "string_utils.h"

#include <QLocale>
#include <QRegularExpression>

namespace csvforge {

QString escapeSQL(const QString& input)
{
    QString result = input;
    result.replace(QLatin1Char('\''), QLatin1String("''"));
    return result;
}

QString quoteName(const QString& identifier)
{
    QString escaped = identifier;
    escaped.replace(QLatin1Char('"'), QLatin1String("\"\""));
    return QLatin1Char('"') + escaped + QLatin1Char('"');
}

QString formatNumber(qint64 value)
{
    return QLocale::system().toString(static_cast<qlonglong>(value));
}

QString formatDouble(double value, int precision)
{
    return QLocale::system().toString(value, 'f', precision);
}

QString truncateString(const QString& text, int maxLen)
{
    if (text.length() <= maxLen) {
        return text;
    }
    return text.left(maxLen) + QStringLiteral("\u2026"); // ellipsis
}

bool isNumericString(const QString& text)
{
    if (text.isEmpty()) {
        return false;
    }
    static const QRegularExpression re(
        QStringLiteral(R"(^[+-]?(\d+\.?\d*|\.\d+)([eE][+-]?\d+)?$)"));
    return re.match(text.trimmed()).hasMatch();
}

QChar parseDelimiter(const QString& token)
{
    if (token.isEmpty()) {
        return QLatin1Char(',');
    }

    const QString lower = token.trimmed().toLower();
    if (lower == QLatin1String("\\t") || lower == QLatin1String("tab")) {
        return QLatin1Char('\t');
    }
    if (lower == QLatin1String("\\n") || lower == QLatin1String("newline")) {
        return QLatin1Char('\n');
    }
    if (lower == QLatin1String("pipe") || lower == QLatin1String("|")) {
        return QLatin1Char('|');
    }
    if (lower == QLatin1String("semicolon") || lower == QLatin1String(";")) {
        return QLatin1Char(';');
    }

    // Single character
    return token.at(0);
}

} // namespace csvforge
