#pragma once

#include <QString>
#include <QChar>

namespace csvforge {

/// Escape single quotes for SQL string literals: ' → ''
QString escapeSQL(const QString& input);

/// Wrap an identifier in double quotes, escaping embedded double quotes.
QString quoteName(const QString& identifier);

/// Format an integer with locale-aware thousands separators.
QString formatNumber(qint64 value);

/// Format a double to the given decimal precision.
QString formatDouble(double value, int precision = 2);

/// Truncate a string to maxLen characters, appending "…" if truncated.
QString truncateString(const QString& text, int maxLen);

/// Return true if the string represents a numeric value.
bool isNumericString(const QString& text);

/// Parse a human-readable delimiter token ("\\t", "tab", ",", etc.) into a QChar.
QChar parseDelimiter(const QString& token);

} // namespace csvforge
