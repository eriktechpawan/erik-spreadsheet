#include "data/types/pivot_config.h"
#include "utils/string_utils.h"
#include <QJsonArray>

namespace csvforge {

// ---------------------------------------------------------------------------
// PivotAggregation helpers
// ---------------------------------------------------------------------------

QString pivotAggregationToString(PivotAggregation agg)
{
    switch (agg) {
    case PivotAggregation::Sum:           return QStringLiteral("SUM");
    case PivotAggregation::Count:         return QStringLiteral("COUNT");
    case PivotAggregation::Average:       return QStringLiteral("AVG");
    case PivotAggregation::Min:           return QStringLiteral("MIN");
    case PivotAggregation::Max:           return QStringLiteral("MAX");
    case PivotAggregation::CountDistinct: return QStringLiteral("COUNT_DISTINCT");
    }
    return QStringLiteral("SUM");
}

PivotAggregation pivotAggregationFromString(const QString& str)
{
    if (str == QLatin1String("SUM"))            return PivotAggregation::Sum;
    if (str == QLatin1String("COUNT"))          return PivotAggregation::Count;
    if (str == QLatin1String("AVG"))            return PivotAggregation::Average;
    if (str == QLatin1String("MIN"))            return PivotAggregation::Min;
    if (str == QLatin1String("MAX"))            return PivotAggregation::Max;
    if (str == QLatin1String("COUNT_DISTINCT")) return PivotAggregation::CountDistinct;
    return PivotAggregation::Sum;
}

// ---------------------------------------------------------------------------
// PivotValueField
// ---------------------------------------------------------------------------

QString PivotValueField::toSQL() const
{
    const QString quotedCol = quoteName(columnName);
    QString aggStr;
    switch (aggregation) {
    case PivotAggregation::Sum:           aggStr = QStringLiteral("SUM"); break;
    case PivotAggregation::Count:         aggStr = QStringLiteral("COUNT"); break;
    case PivotAggregation::Average:       aggStr = QStringLiteral("AVG"); break;
    case PivotAggregation::Min:           aggStr = QStringLiteral("MIN"); break;
    case PivotAggregation::Max:           aggStr = QStringLiteral("MAX"); break;
    case PivotAggregation::CountDistinct: aggStr = QStringLiteral("COUNT(DISTINCT"); break;
    }

    if (aggregation == PivotAggregation::CountDistinct) {
        return QStringLiteral("%1 %2)").arg(aggStr, quotedCol);
    }
    return QStringLiteral("%1(%2)").arg(aggStr, quotedCol);
}

QString PivotValueField::displayName() const
{
    if (!alias.isEmpty()) return alias;
    return QStringLiteral("%1(%2)")
        .arg(pivotAggregationToString(aggregation), columnName);
}

// ---------------------------------------------------------------------------
// PivotConfig
// ---------------------------------------------------------------------------

bool PivotConfig::isValid() const
{
    return !sourceTable.isEmpty() && !columnFields.isEmpty() && !valueFields.empty();
}

QJsonObject PivotConfig::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("sourceTable")] = sourceTable;
    obj[QStringLiteral("rowFields")] = QJsonArray::fromStringList(rowFields);
    obj[QStringLiteral("columnFields")] = QJsonArray::fromStringList(columnFields);

    QJsonArray valArr;
    for (const auto& vf : valueFields) {
        QJsonObject vo;
        vo[QStringLiteral("columnName")] = vf.columnName;
        vo[QStringLiteral("aggregation")] = pivotAggregationToString(vf.aggregation);
        vo[QStringLiteral("alias")] = vf.alias;
        valArr.append(vo);
    }
    obj[QStringLiteral("valueFields")] = valArr;

    obj[QStringLiteral("outputPath")] = outputPath;
    obj[QStringLiteral("outputTarget")] = static_cast<int>(outputTarget);
    obj[QStringLiteral("includeGrandTotals")] = includeGrandTotals;
    obj[QStringLiteral("includeSubtotals")] = includeSubtotals;
    obj[QStringLiteral("sortByValue")] = sortByValue;
    obj[QStringLiteral("showRowCounts")] = showRowCounts;
    obj[QStringLiteral("previewLimit")] = previewLimit;
    return obj;
}

PivotConfig PivotConfig::fromJson(const QJsonObject& obj)
{
    PivotConfig cfg;
    cfg.sourceTable = obj[QStringLiteral("sourceTable")].toString();

    for (const auto& v : obj[QStringLiteral("rowFields")].toArray())
        cfg.rowFields << v.toString();
    for (const auto& v : obj[QStringLiteral("columnFields")].toArray())
        cfg.columnFields << v.toString();

    for (const auto& v : obj[QStringLiteral("valueFields")].toArray()) {
        const QJsonObject vo = v.toObject();
        PivotValueField vf;
        vf.columnName = vo[QStringLiteral("columnName")].toString();
        vf.aggregation = pivotAggregationFromString(
            vo[QStringLiteral("aggregation")].toString());
        vf.alias = vo[QStringLiteral("alias")].toString();
        cfg.valueFields.push_back(vf);
    }

    cfg.outputPath = obj[QStringLiteral("outputPath")].toString();
    cfg.outputTarget = static_cast<PivotOutputTarget>(
        obj[QStringLiteral("outputTarget")].toInt(static_cast<int>(PivotOutputTarget::NewTab)));
    cfg.includeGrandTotals = obj[QStringLiteral("includeGrandTotals")].toBool();
    cfg.includeSubtotals = obj[QStringLiteral("includeSubtotals")].toBool();
    cfg.sortByValue = obj[QStringLiteral("sortByValue")].toBool();
    cfg.showRowCounts = obj[QStringLiteral("showRowCounts")].toBool();
    cfg.previewLimit = obj[QStringLiteral("previewLimit")].toInt(100);
    return cfg;
}

} // namespace csvforge
