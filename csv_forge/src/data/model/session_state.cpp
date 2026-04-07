#include "data/model/session_state.h"
#include "utils/logging.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace csvforge {

// ---------------------------------------------------------------------------
// Filter serialisation helpers
// ---------------------------------------------------------------------------

namespace {

QString filterOperatorToString(FilterOperator op)
{
    switch (op) {
    case FilterOperator::Equals:              return QStringLiteral("Equals");
    case FilterOperator::NotEquals:           return QStringLiteral("NotEquals");
    case FilterOperator::Contains:            return QStringLiteral("Contains");
    case FilterOperator::NotContains:         return QStringLiteral("NotContains");
    case FilterOperator::StartsWith:          return QStringLiteral("StartsWith");
    case FilterOperator::EndsWith:            return QStringLiteral("EndsWith");
    case FilterOperator::ExactMatch:          return QStringLiteral("ExactMatch");
    case FilterOperator::GreaterThan:         return QStringLiteral("GreaterThan");
    case FilterOperator::GreaterEqual:        return QStringLiteral("GreaterEqual");
    case FilterOperator::LessThan:            return QStringLiteral("LessThan");
    case FilterOperator::LessEqual:           return QStringLiteral("LessEqual");
    case FilterOperator::Between:             return QStringLiteral("Between");
    case FilterOperator::IsZero:              return QStringLiteral("IsZero");
    case FilterOperator::NonZero:             return QStringLiteral("NonZero");
    case FilterOperator::IsPositive:          return QStringLiteral("IsPositive");
    case FilterOperator::IsNegative:          return QStringLiteral("IsNegative");
    case FilterOperator::IsEmpty:             return QStringLiteral("IsEmpty");
    case FilterOperator::IsNotEmpty:          return QStringLiteral("IsNotEmpty");
    case FilterOperator::IsNull:              return QStringLiteral("IsNull");
    case FilterOperator::IsNotNull:           return QStringLiteral("IsNotNull");
    case FilterOperator::InList:              return QStringLiteral("InList");
    case FilterOperator::NotInList:           return QStringLiteral("NotInList");
    case FilterOperator::Regex:               return QStringLiteral("Regex");
    case FilterOperator::AboveAverage:        return QStringLiteral("AboveAverage");
    case FilterOperator::BelowAverage:        return QStringLiteral("BelowAverage");
    case FilterOperator::CaseSensitiveEquals: return QStringLiteral("CaseSensitiveEquals");
    }
    return QStringLiteral("Equals");
}

FilterOperator filterOperatorFromString(const QString& s)
{
    static const QMap<QString, FilterOperator> map = {
        {QStringLiteral("Equals"),              FilterOperator::Equals},
        {QStringLiteral("NotEquals"),           FilterOperator::NotEquals},
        {QStringLiteral("Contains"),            FilterOperator::Contains},
        {QStringLiteral("NotContains"),         FilterOperator::NotContains},
        {QStringLiteral("StartsWith"),          FilterOperator::StartsWith},
        {QStringLiteral("EndsWith"),            FilterOperator::EndsWith},
        {QStringLiteral("ExactMatch"),          FilterOperator::ExactMatch},
        {QStringLiteral("GreaterThan"),         FilterOperator::GreaterThan},
        {QStringLiteral("GreaterEqual"),        FilterOperator::GreaterEqual},
        {QStringLiteral("LessThan"),            FilterOperator::LessThan},
        {QStringLiteral("LessEqual"),           FilterOperator::LessEqual},
        {QStringLiteral("Between"),             FilterOperator::Between},
        {QStringLiteral("IsZero"),              FilterOperator::IsZero},
        {QStringLiteral("NonZero"),             FilterOperator::NonZero},
        {QStringLiteral("IsPositive"),          FilterOperator::IsPositive},
        {QStringLiteral("IsNegative"),          FilterOperator::IsNegative},
        {QStringLiteral("IsEmpty"),             FilterOperator::IsEmpty},
        {QStringLiteral("IsNotEmpty"),          FilterOperator::IsNotEmpty},
        {QStringLiteral("IsNull"),              FilterOperator::IsNull},
        {QStringLiteral("IsNotNull"),           FilterOperator::IsNotNull},
        {QStringLiteral("InList"),              FilterOperator::InList},
        {QStringLiteral("NotInList"),           FilterOperator::NotInList},
        {QStringLiteral("Regex"),               FilterOperator::Regex},
        {QStringLiteral("AboveAverage"),        FilterOperator::AboveAverage},
        {QStringLiteral("BelowAverage"),        FilterOperator::BelowAverage},
        {QStringLiteral("CaseSensitiveEquals"), FilterOperator::CaseSensitiveEquals},
    };
    return map.value(s, FilterOperator::Equals);
}

QJsonObject filterRuleToJson(const FilterRule& rule)
{
    QJsonObject obj;
    obj[QStringLiteral("column")] = rule.columnName;
    obj[QStringLiteral("operator")] = filterOperatorToString(rule.op);
    obj[QStringLiteral("value")] = QJsonValue::fromVariant(rule.value);
    obj[QStringLiteral("secondValue")] = QJsonValue::fromVariant(rule.secondValue);
    obj[QStringLiteral("caseSensitive")] = rule.caseSensitive;

    if (!rule.inListValues.isEmpty()) {
        QJsonArray arr;
        for (const auto& v : rule.inListValues) {
            arr.append(v);
        }
        obj[QStringLiteral("inListValues")] = arr;
    }
    return obj;
}

FilterRule filterRuleFromJson(const QJsonObject& obj)
{
    FilterRule rule;
    rule.columnName = obj[QStringLiteral("column")].toString();
    rule.op = filterOperatorFromString(obj[QStringLiteral("operator")].toString());
    rule.value = obj[QStringLiteral("value")].toVariant();
    rule.secondValue = obj[QStringLiteral("secondValue")].toVariant();
    rule.caseSensitive = obj[QStringLiteral("caseSensitive")].toBool();

    const QJsonArray inList = obj[QStringLiteral("inListValues")].toArray();
    for (const auto& v : inList) {
        rule.inListValues.append(v.toString());
    }
    return rule;
}

QJsonObject filterGroupToJson(const FilterGroup& group)
{
    QJsonObject obj;
    obj[QStringLiteral("combination")] =
        (group.combination == FilterCombination::And) ? QStringLiteral("And")
                                                      : QStringLiteral("Or");
    QJsonArray rulesArr;
    for (const auto& r : group.rules) {
        rulesArr.append(filterRuleToJson(r));
    }
    obj[QStringLiteral("rules")] = rulesArr;

    if (!group.subGroups.empty()) {
        QJsonArray subArr;
        for (const auto& sg : group.subGroups) {
            subArr.append(filterGroupToJson(sg));
        }
        obj[QStringLiteral("subGroups")] = subArr;
    }
    return obj;
}

FilterGroup filterGroupFromJson(const QJsonObject& obj)
{
    FilterGroup group;
    group.combination = (obj[QStringLiteral("combination")].toString() == QStringLiteral("And"))
                            ? FilterCombination::And
                            : FilterCombination::Or;

    const QJsonArray rulesArr = obj[QStringLiteral("rules")].toArray();
    for (const auto& v : rulesArr) {
        group.rules.push_back(filterRuleFromJson(v.toObject()));
    }

    const QJsonArray subArr = obj[QStringLiteral("subGroups")].toArray();
    for (const auto& v : subArr) {
        group.subGroups.push_back(filterGroupFromJson(v.toObject()));
    }
    return group;
}

QJsonObject sortColumnToJson(const SortColumn& sc)
{
    QJsonObject obj;
    obj[QStringLiteral("column")] = sc.columnName;
    obj[QStringLiteral("ascending")] = sc.ascending;
    return obj;
}

SortColumn sortColumnFromJson(const QJsonObject& obj)
{
    SortColumn sc;
    sc.columnName = obj[QStringLiteral("column")].toString();
    sc.ascending = obj[QStringLiteral("ascending")].toBool(true);
    return sc;
}

QString pivotAggregationToString(PivotAggregation agg)
{
    switch (agg) {
    case PivotAggregation::Sum:     return QStringLiteral("Sum");
    case PivotAggregation::Count:   return QStringLiteral("Count");
    case PivotAggregation::Average: return QStringLiteral("Average");
    case PivotAggregation::Min:     return QStringLiteral("Min");
    case PivotAggregation::Max:     return QStringLiteral("Max");
    }
    return QStringLiteral("Sum");
}

PivotAggregation pivotAggregationFromString(const QString& s)
{
    if (s == QStringLiteral("Count"))   return PivotAggregation::Count;
    if (s == QStringLiteral("Average")) return PivotAggregation::Average;
    if (s == QStringLiteral("Min"))     return PivotAggregation::Min;
    if (s == QStringLiteral("Max"))     return PivotAggregation::Max;
    return PivotAggregation::Sum;
}

QString pivotOutputTargetToString(PivotOutputTarget t)
{
    switch (t) {
    case PivotOutputTarget::NewTab:          return QStringLiteral("NewTab");
    case PivotOutputTarget::TemporaryTable:  return QStringLiteral("TemporaryTable");
    case PivotOutputTarget::ExportFile:      return QStringLiteral("ExportFile");
    }
    return QStringLiteral("NewTab");
}

PivotOutputTarget pivotOutputTargetFromString(const QString& s)
{
    if (s == QStringLiteral("TemporaryTable")) return PivotOutputTarget::TemporaryTable;
    if (s == QStringLiteral("ExportFile"))     return PivotOutputTarget::ExportFile;
    return PivotOutputTarget::NewTab;
}

QJsonObject pivotConfigToJson(const PivotConfig& pc)
{
    QJsonObject obj;
    obj[QStringLiteral("sourceTable")] = pc.sourceTable;
    obj[QStringLiteral("outputTarget")] = pivotOutputTargetToString(pc.outputTarget);
    obj[QStringLiteral("outputPath")] = pc.outputPath;

    QJsonArray rowArr;
    for (const auto& f : pc.rowFields)    rowArr.append(f);
    obj[QStringLiteral("rowFields")] = rowArr;

    QJsonArray colArr;
    for (const auto& f : pc.columnFields) colArr.append(f);
    obj[QStringLiteral("columnFields")] = colArr;

    QJsonArray valArr;
    for (const auto& vf : pc.valueFields) {
        QJsonObject vObj;
        vObj[QStringLiteral("column")] = vf.columnName;
        vObj[QStringLiteral("aggregation")] = pivotAggregationToString(vf.aggregation);
        valArr.append(vObj);
    }
    obj[QStringLiteral("valueFields")] = valArr;

    return obj;
}

PivotConfig pivotConfigFromJson(const QJsonObject& obj)
{
    PivotConfig pc;
    pc.sourceTable = obj[QStringLiteral("sourceTable")].toString();
    pc.outputTarget = pivotOutputTargetFromString(
        obj[QStringLiteral("outputTarget")].toString());
    pc.outputPath = obj[QStringLiteral("outputPath")].toString();

    const QJsonArray rowArr = obj[QStringLiteral("rowFields")].toArray();
    for (const auto& v : rowArr)    pc.rowFields.append(v.toString());

    const QJsonArray colArr = obj[QStringLiteral("columnFields")].toArray();
    for (const auto& v : colArr)    pc.columnFields.append(v.toString());

    const QJsonArray valArr = obj[QStringLiteral("valueFields")].toArray();
    for (const auto& v : valArr) {
        PivotValueField vf;
        const QJsonObject vObj = v.toObject();
        vf.columnName = vObj[QStringLiteral("column")].toString();
        vf.aggregation = pivotAggregationFromString(
            vObj[QStringLiteral("aggregation")].toString());
        pc.valueFields.push_back(vf);
    }
    return pc;
}

QString lookupTypeToString(LookupType t)
{
    switch (t) {
    case LookupType::ExactMatch: return QStringLiteral("ExactMatch");
    case LookupType::LeftJoin:   return QStringLiteral("LeftJoin");
    }
    return QStringLiteral("ExactMatch");
}

LookupType lookupTypeFromString(const QString& s)
{
    if (s == QStringLiteral("LeftJoin")) return LookupType::LeftJoin;
    return LookupType::ExactMatch;
}

QString lookupOutputTargetToString(LookupOutputTarget t)
{
    switch (t) {
    case LookupOutputTarget::AppendColumns:   return QStringLiteral("AppendColumns");
    case LookupOutputTarget::NewResultTable:  return QStringLiteral("NewResultTable");
    }
    return QStringLiteral("AppendColumns");
}

LookupOutputTarget lookupOutputTargetFromString(const QString& s)
{
    if (s == QStringLiteral("NewResultTable")) return LookupOutputTarget::NewResultTable;
    return LookupOutputTarget::AppendColumns;
}

QJsonObject lookupConfigToJson(const LookupConfig& lc)
{
    QJsonObject obj;
    obj[QStringLiteral("sourceTable")] = lc.sourceTable;
    obj[QStringLiteral("sourceKeyColumn")] = lc.sourceKeyColumn;
    obj[QStringLiteral("targetFile")] = lc.targetFile;
    obj[QStringLiteral("targetTable")] = lc.targetTable;
    obj[QStringLiteral("targetKeyColumn")] = lc.targetKeyColumn;
    obj[QStringLiteral("resultTableName")] = lc.resultTableName;
    obj[QStringLiteral("type")] = lookupTypeToString(lc.type);
    obj[QStringLiteral("outputTarget")] = lookupOutputTargetToString(lc.outputTarget);

    QJsonArray retArr;
    for (const auto& c : lc.returnColumns) retArr.append(c);
    obj[QStringLiteral("returnColumns")] = retArr;

    return obj;
}

LookupConfig lookupConfigFromJson(const QJsonObject& obj)
{
    LookupConfig lc;
    lc.sourceTable = obj[QStringLiteral("sourceTable")].toString();
    lc.sourceKeyColumn = obj[QStringLiteral("sourceKeyColumn")].toString();
    lc.targetFile = obj[QStringLiteral("targetFile")].toString();
    lc.targetTable = obj[QStringLiteral("targetTable")].toString();
    lc.targetKeyColumn = obj[QStringLiteral("targetKeyColumn")].toString();
    lc.resultTableName = obj[QStringLiteral("resultTableName")].toString();
    lc.type = lookupTypeFromString(obj[QStringLiteral("type")].toString());
    lc.outputTarget = lookupOutputTargetFromString(
        obj[QStringLiteral("outputTarget")].toString());

    const QJsonArray retArr = obj[QStringLiteral("returnColumns")].toArray();
    for (const auto& v : retArr) lc.returnColumns.append(v.toString());

    return lc;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// SessionState::toJson
// ---------------------------------------------------------------------------

QJsonObject SessionState::toJson() const
{
    QJsonObject root;

    // File info
    root[QStringLiteral("sourceFilePath")] = sourceFilePath;
    root[QStringLiteral("sessionFilePath")] = sessionFilePath;
    root[QStringLiteral("editableMode")] = editableMode;

    // View state: filters
    root[QStringLiteral("filters")] = filterGroupToJson(filters);

    // View state: sorts
    {
        QJsonArray arr;
        for (const auto& s : sorts) {
            arr.append(sortColumnToJson(s));
        }
        root[QStringLiteral("sorts")] = arr;
    }

    // View state: columns
    {
        QJsonArray arr;
        for (const auto& c : hiddenColumns) arr.append(c);
        root[QStringLiteral("hiddenColumns")] = arr;
    }
    {
        QJsonArray arr;
        for (const auto& c : columnOrder) arr.append(c);
        root[QStringLiteral("columnOrder")] = arr;
    }
    {
        QJsonObject widths;
        for (auto it = columnWidths.constBegin(); it != columnWidths.constEnd(); ++it) {
            widths[it.key()] = it.value();
        }
        root[QStringLiteral("columnWidths")] = widths;
    }

    // Recent files
    {
        QJsonArray arr;
        for (const auto& f : recentFiles) arr.append(f);
        root[QStringLiteral("recentFiles")] = arr;
    }

    // Filter presets
    {
        QJsonArray arr;
        for (const auto& p : filterPresets) {
            QJsonObject pObj;
            pObj[QStringLiteral("name")] = p.name;
            pObj[QStringLiteral("group")] = filterGroupToJson(p.group);
            pObj[QStringLiteral("createdAt")] = p.createdAt.toString(Qt::ISODate);
            arr.append(pObj);
        }
        root[QStringLiteral("filterPresets")] = arr;
    }

    // Pivot configs
    {
        QJsonArray arr;
        for (const auto& pc : pivotConfigs) arr.append(pivotConfigToJson(pc));
        root[QStringLiteral("pivotConfigs")] = arr;
    }

    // Lookup configs
    {
        QJsonArray arr;
        for (const auto& lc : lookupConfigs) arr.append(lookupConfigToJson(lc));
        root[QStringLiteral("lookupConfigs")] = arr;
    }

    // Window geometry (Base64-encoded)
    root[QStringLiteral("windowGeometry")] =
        QString::fromLatin1(windowGeometry.toBase64());
    root[QStringLiteral("windowState")] =
        QString::fromLatin1(windowState.toBase64());

    return root;
}

// ---------------------------------------------------------------------------
// SessionState::fromJson
// ---------------------------------------------------------------------------

SessionState SessionState::fromJson(const QJsonObject& root)
{
    SessionState s;

    // File info
    s.sourceFilePath = root[QStringLiteral("sourceFilePath")].toString();
    s.sessionFilePath = root[QStringLiteral("sessionFilePath")].toString();
    s.editableMode = root[QStringLiteral("editableMode")].toBool();

    // Filters
    s.filters = filterGroupFromJson(root[QStringLiteral("filters")].toObject());

    // Sorts
    {
        const QJsonArray arr = root[QStringLiteral("sorts")].toArray();
        for (const auto& v : arr) {
            s.sorts.push_back(sortColumnFromJson(v.toObject()));
        }
    }

    // Hidden columns
    {
        const QJsonArray arr = root[QStringLiteral("hiddenColumns")].toArray();
        for (const auto& v : arr) s.hiddenColumns.append(v.toString());
    }
    // Column order
    {
        const QJsonArray arr = root[QStringLiteral("columnOrder")].toArray();
        for (const auto& v : arr) s.columnOrder.append(v.toString());
    }
    // Column widths
    {
        const QJsonObject widths = root[QStringLiteral("columnWidths")].toObject();
        for (auto it = widths.constBegin(); it != widths.constEnd(); ++it) {
            s.columnWidths[it.key()] = it.value().toInt();
        }
    }

    // Recent files
    {
        const QJsonArray arr = root[QStringLiteral("recentFiles")].toArray();
        for (const auto& v : arr) s.recentFiles.append(v.toString());
    }

    // Filter presets
    {
        const QJsonArray arr = root[QStringLiteral("filterPresets")].toArray();
        for (const auto& v : arr) {
            const QJsonObject pObj = v.toObject();
            FilterPreset p;
            p.name = pObj[QStringLiteral("name")].toString();
            p.group = filterGroupFromJson(pObj[QStringLiteral("group")].toObject());
            p.createdAt = QDateTime::fromString(
                pObj[QStringLiteral("createdAt")].toString(), Qt::ISODate);
            s.filterPresets.push_back(std::move(p));
        }
    }

    // Pivot configs
    {
        const QJsonArray arr = root[QStringLiteral("pivotConfigs")].toArray();
        for (const auto& v : arr) {
            s.pivotConfigs.push_back(pivotConfigFromJson(v.toObject()));
        }
    }

    // Lookup configs
    {
        const QJsonArray arr = root[QStringLiteral("lookupConfigs")].toArray();
        for (const auto& v : arr) {
            s.lookupConfigs.push_back(lookupConfigFromJson(v.toObject()));
        }
    }

    // Window geometry
    s.windowGeometry = QByteArray::fromBase64(
        root[QStringLiteral("windowGeometry")].toString().toLatin1());
    s.windowState = QByteArray::fromBase64(
        root[QStringLiteral("windowState")].toString().toLatin1());

    return s;
}

// ---------------------------------------------------------------------------
// SessionState::saveToFile / loadFromFile
// ---------------------------------------------------------------------------

bool SessionState::saveToFile(const QString& path) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        Logger::instance().error(
            LogCategory::Session,
            QStringLiteral("Failed to save session to %1: %2")
                .arg(path, file.errorString()));
        return false;
    }

    const QJsonDocument doc(toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    Logger::instance().info(
        LogCategory::Session,
        QStringLiteral("Session saved to %1").arg(path));
    return true;
}

std::optional<SessionState> SessionState::loadFromFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        Logger::instance().error(
            LogCategory::Session,
            QStringLiteral("Failed to open session file %1: %2")
                .arg(path, file.errorString()));
        return std::nullopt;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError) {
        Logger::instance().error(
            LogCategory::Session,
            QStringLiteral("Failed to parse session file %1: %2")
                .arg(path, parseError.errorString()));
        return std::nullopt;
    }

    if (!doc.isObject()) {
        Logger::instance().error(
            LogCategory::Session,
            QStringLiteral("Session file %1 does not contain a JSON object").arg(path));
        return std::nullopt;
    }

    Logger::instance().info(
        LogCategory::Session,
        QStringLiteral("Session loaded from %1").arg(path));

    return fromJson(doc.object());
}

} // namespace csvforge
