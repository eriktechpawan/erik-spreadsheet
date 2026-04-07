#include "data/model/transformation_step.h"
#include "data/model/sort_state.h"

#include <QJsonArray>

namespace csvforge {

// ---------------------------------------------------------------------------
// Type string conversion
// ---------------------------------------------------------------------------

QString transformationTypeToString(TransformationType type)
{
    switch (type) {
    case TransformationType::Source:           return QStringLiteral("Source");
    case TransformationType::Filter:           return QStringLiteral("Filter");
    case TransformationType::Sort:             return QStringLiteral("Sort");
    case TransformationType::CalculatedColumn: return QStringLiteral("CalculatedColumn");
    case TransformationType::Lookup:           return QStringLiteral("Lookup");
    case TransformationType::Pivot:            return QStringLiteral("Pivot");
    case TransformationType::HideColumns:      return QStringLiteral("HideColumns");
    case TransformationType::Rename:           return QStringLiteral("Rename");
    }
    return QStringLiteral("Unknown");
}

TransformationType transformationTypeFromString(const QString& str)
{
    if (str == QLatin1String("Source"))           return TransformationType::Source;
    if (str == QLatin1String("Filter"))           return TransformationType::Filter;
    if (str == QLatin1String("Sort"))             return TransformationType::Sort;
    if (str == QLatin1String("CalculatedColumn")) return TransformationType::CalculatedColumn;
    if (str == QLatin1String("Lookup"))           return TransformationType::Lookup;
    if (str == QLatin1String("Pivot"))            return TransformationType::Pivot;
    if (str == QLatin1String("HideColumns"))      return TransformationType::HideColumns;
    if (str == QLatin1String("Rename"))           return TransformationType::Rename;
    return TransformationType::Source;
}

// ---------------------------------------------------------------------------
// Factory helpers
// ---------------------------------------------------------------------------

TransformationStep TransformationStep::makeSource(const QString& filePath,
                                                   const QString& tableName)
{
    TransformationStep step;
    step.type = TransformationType::Source;
    step.description = QStringLiteral("Load '%1'").arg(filePath);
    QJsonObject cfg;
    cfg[QStringLiteral("filePath")] = filePath;
    cfg[QStringLiteral("tableName")] = tableName;
    step.config = cfg;
    return step;
}

TransformationStep TransformationStep::makeFilter(const FilterGroup& filters)
{
    TransformationStep step;
    step.type = TransformationType::Filter;
    step.description = QStringLiteral("Filter (%1 rule(s))").arg(filters.ruleCount());
    // Store filter as a simplified JSON for now
    QJsonObject cfg;
    cfg[QStringLiteral("ruleCount")] = filters.ruleCount();
    cfg[QStringLiteral("combination")] =
        (filters.combination == FilterCombination::And) ?
            QStringLiteral("AND") : QStringLiteral("OR");
    QJsonArray rulesArr;
    for (const auto& rule : filters.rules) {
        rulesArr.append(rule.toDisplayString());
    }
    cfg[QStringLiteral("rulesSummary")] = rulesArr;
    step.config = cfg;
    return step;
}

TransformationStep TransformationStep::makeSort(
    const std::vector<SortColumn>& sorts)
{
    TransformationStep step;
    step.type = TransformationType::Sort;
    QStringList parts;
    QJsonArray arr;
    for (const auto& sc : sorts) {
        parts << QStringLiteral("%1 %2").arg(sc.columnName,
                   sc.ascending ? QStringLiteral("ASC") : QStringLiteral("DESC"));
        QJsonObject o;
        o[QStringLiteral("column")] = sc.columnName;
        o[QStringLiteral("ascending")] = sc.ascending;
        arr.append(o);
    }
    step.description = QStringLiteral("Sort by %1").arg(parts.join(QStringLiteral(", ")));
    step.config[QStringLiteral("sorts")] = arr;
    return step;
}

TransformationStep TransformationStep::makeCalculatedColumn(
    const CalculatedColumnDef& def)
{
    TransformationStep step;
    step.type = TransformationType::CalculatedColumn;
    step.description = QStringLiteral("Add column '%1' = %2")
                           .arg(def.columnName, def.formulaText);
    step.config = def.toJson();
    return step;
}

TransformationStep TransformationStep::makeLookup(const LookupConfig& config)
{
    TransformationStep step;
    step.type = TransformationType::Lookup;
    step.description = QStringLiteral("Lookup from '%1' on %2 = %3")
                           .arg(config.targetTable.isEmpty() ? config.targetFile
                                                             : config.targetTable,
                                config.sourceKeyColumn, config.targetKeyColumn);
    QJsonObject cfg;
    cfg[QStringLiteral("sourceTable")] = config.sourceTable;
    cfg[QStringLiteral("sourceKeyColumn")] = config.sourceKeyColumn;
    cfg[QStringLiteral("targetTable")] = config.targetTable;
    cfg[QStringLiteral("targetFile")] = config.targetFile;
    cfg[QStringLiteral("targetKeyColumn")] = config.targetKeyColumn;
    cfg[QStringLiteral("returnColumns")] =
        QJsonArray::fromStringList(config.returnColumns);
    cfg[QStringLiteral("resultTableName")] = config.resultTableName;
    step.config = cfg;
    return step;
}

TransformationStep TransformationStep::makePivot(const PivotConfig& config)
{
    TransformationStep step;
    step.type = TransformationType::Pivot;
    step.description = QStringLiteral("Pivot on %1")
                           .arg(config.columnFields.join(QStringLiteral(", ")));
    QJsonObject cfg;
    cfg[QStringLiteral("sourceTable")] = config.sourceTable;
    cfg[QStringLiteral("rowFields")] = QJsonArray::fromStringList(config.rowFields);
    cfg[QStringLiteral("columnFields")] = QJsonArray::fromStringList(config.columnFields);
    QJsonArray valArr;
    for (const auto& vf : config.valueFields) {
        QJsonObject vo;
        vo[QStringLiteral("column")] = vf.columnName;
        vo[QStringLiteral("aggregation")] = vf.displayName();
        valArr.append(vo);
    }
    cfg[QStringLiteral("valueFields")] = valArr;
    step.config = cfg;
    return step;
}

TransformationStep TransformationStep::makeHideColumns(const QStringList& hidden)
{
    TransformationStep step;
    step.type = TransformationType::HideColumns;
    step.description = QStringLiteral("Hide %1 column(s)").arg(hidden.size());
    step.config[QStringLiteral("hiddenColumns")] =
        QJsonArray::fromStringList(hidden);
    return step;
}

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------

QJsonObject TransformationStep::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("type")] = transformationTypeToString(type);
    obj[QStringLiteral("description")] = description;
    obj[QStringLiteral("config")] = config;
    return obj;
}

TransformationStep TransformationStep::fromJson(const QJsonObject& obj)
{
    TransformationStep step;
    step.type = transformationTypeFromString(
        obj[QStringLiteral("type")].toString());
    step.description = obj[QStringLiteral("description")].toString();
    step.config = obj[QStringLiteral("config")].toObject();
    return step;
}

} // namespace csvforge
