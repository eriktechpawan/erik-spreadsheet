#include "data/types/formula_config.h"
#include <QJsonArray>

namespace csvforge {

// ---------------------------------------------------------------------------
// FormulaOutputType helpers
// ---------------------------------------------------------------------------

QString formulaOutputTypeToString(FormulaOutputType t)
{
    switch (t) {
    case FormulaOutputType::Text:    return QStringLiteral("Text");
    case FormulaOutputType::Numeric: return QStringLiteral("Numeric");
    case FormulaOutputType::Boolean: return QStringLiteral("Boolean");
    case FormulaOutputType::Date:    return QStringLiteral("Date");
    case FormulaOutputType::Unknown: return QStringLiteral("Unknown");
    }
    return QStringLiteral("Unknown");
}

FormulaOutputType formulaOutputTypeFromString(const QString& s)
{
    if (s == QLatin1String("Text"))    return FormulaOutputType::Text;
    if (s == QLatin1String("Numeric")) return FormulaOutputType::Numeric;
    if (s == QLatin1String("Boolean")) return FormulaOutputType::Boolean;
    if (s == QLatin1String("Date"))    return FormulaOutputType::Date;
    return FormulaOutputType::Unknown;
}

// ---------------------------------------------------------------------------
// CalculatedColumnDef
// ---------------------------------------------------------------------------

QJsonObject CalculatedColumnDef::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("columnName")] = columnName;
    obj[QStringLiteral("formulaText")] = formulaText;
    obj[QStringLiteral("outputType")] = formulaOutputTypeToString(outputType);
    obj[QStringLiteral("referencedColumns")] = QJsonArray::fromStringList(referencedColumns);
    return obj;
}

CalculatedColumnDef CalculatedColumnDef::fromJson(const QJsonObject& obj)
{
    CalculatedColumnDef def;
    def.columnName = obj[QStringLiteral("columnName")].toString();
    def.formulaText = obj[QStringLiteral("formulaText")].toString();
    def.outputType = formulaOutputTypeFromString(
        obj[QStringLiteral("outputType")].toString());
    for (const auto& v : obj[QStringLiteral("referencedColumns")].toArray())
        def.referencedColumns << v.toString();
    return def;
}

bool CalculatedColumnDef::isValid() const
{
    return !columnName.isEmpty() && !formulaText.isEmpty();
}

// ---------------------------------------------------------------------------
// FormulaConfig
// ---------------------------------------------------------------------------

QJsonObject FormulaConfig::toJson() const
{
    QJsonObject obj;
    QJsonArray arr;
    for (const auto& col : columns) {
        arr.append(col.toJson());
    }
    obj[QStringLiteral("columns")] = arr;
    return obj;
}

FormulaConfig FormulaConfig::fromJson(const QJsonObject& obj)
{
    FormulaConfig cfg;
    for (const auto& v : obj[QStringLiteral("columns")].toArray()) {
        cfg.columns.push_back(CalculatedColumnDef::fromJson(v.toObject()));
    }
    return cfg;
}

bool FormulaConfig::isEmpty() const
{
    return columns.empty();
}

} // namespace csvforge
