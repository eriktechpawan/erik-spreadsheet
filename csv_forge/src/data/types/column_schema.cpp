#include "data/types/column_schema.h"

namespace csvforge {

ColumnType ColumnSchema::fromDuckDBType(const QString& dbType)
{
    const QString upper = dbType.toUpper().trimmed();

    if (upper == QLatin1String("BOOLEAN") || upper == QLatin1String("BOOL"))
        return ColumnType::Boolean;

    if (upper == QLatin1String("TINYINT") || upper == QLatin1String("SMALLINT")
        || upper == QLatin1String("INTEGER") || upper == QLatin1String("BIGINT")
        || upper == QLatin1String("HUGEINT") || upper == QLatin1String("INT")
        || upper == QLatin1String("INT8") || upper == QLatin1String("INT4")
        || upper == QLatin1String("INT2") || upper == QLatin1String("INT1")
        || upper == QLatin1String("UTINYINT") || upper == QLatin1String("USMALLINT")
        || upper == QLatin1String("UINTEGER") || upper == QLatin1String("UBIGINT"))
        return ColumnType::Integer;

    if (upper == QLatin1String("FLOAT") || upper == QLatin1String("DOUBLE")
        || upper == QLatin1String("REAL") || upper == QLatin1String("DECIMAL")
        || upper.startsWith(QLatin1String("DECIMAL("))
        || upper.startsWith(QLatin1String("NUMERIC")))
        return ColumnType::Double;

    if (upper == QLatin1String("DATE"))
        return ColumnType::Date;

    if (upper == QLatin1String("TIMESTAMP") || upper == QLatin1String("TIMESTAMP WITH TIME ZONE")
        || upper == QLatin1String("TIMESTAMPTZ") || upper == QLatin1String("DATETIME"))
        return ColumnType::DateTime;

    if (upper == QLatin1String("VARCHAR") || upper == QLatin1String("TEXT")
        || upper == QLatin1String("STRING") || upper == QLatin1String("CHAR")
        || upper.startsWith(QLatin1String("VARCHAR("))
        || upper.startsWith(QLatin1String("CHAR(")))
        return ColumnType::Text;

    return ColumnType::Unknown;
}

QString ColumnSchema::typeToString(ColumnType t)
{
    switch (t) {
    case ColumnType::Text:     return QStringLiteral("Text");
    case ColumnType::Integer:  return QStringLiteral("Integer");
    case ColumnType::Double:   return QStringLiteral("Double");
    case ColumnType::Boolean:  return QStringLiteral("Boolean");
    case ColumnType::Date:     return QStringLiteral("Date");
    case ColumnType::DateTime: return QStringLiteral("DateTime");
    case ColumnType::Unknown:  return QStringLiteral("Unknown");
    }
    return QStringLiteral("Unknown");
}

ColumnType ColumnSchema::typeFromString(const QString& str)
{
    if (str == QLatin1String("Text"))     return ColumnType::Text;
    if (str == QLatin1String("Integer"))  return ColumnType::Integer;
    if (str == QLatin1String("Double"))   return ColumnType::Double;
    if (str == QLatin1String("Boolean"))  return ColumnType::Boolean;
    if (str == QLatin1String("Date"))     return ColumnType::Date;
    if (str == QLatin1String("DateTime")) return ColumnType::DateTime;
    return ColumnType::Unknown;
}

} // namespace csvforge
