#include "data/types/lookup_config.h"
#include <QJsonArray>

namespace csvforge {

bool LookupConfig::isValid() const
{
    return !sourceTable.isEmpty()
        && !sourceKeyColumn.isEmpty()
        && !targetKeyColumn.isEmpty()
        && (!targetFile.isEmpty() || !targetTable.isEmpty());
}

QJsonObject LookupConfig::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("sourceTable")] = sourceTable;
    obj[QStringLiteral("sourceKeyColumn")] = sourceKeyColumn;
    obj[QStringLiteral("targetFile")] = targetFile;
    obj[QStringLiteral("targetTable")] = targetTable;
    obj[QStringLiteral("targetKeyColumn")] = targetKeyColumn;
    obj[QStringLiteral("returnColumns")] = QJsonArray::fromStringList(returnColumns);
    obj[QStringLiteral("resultTableName")] = resultTableName;
    obj[QStringLiteral("trimWhitespace")] = trimWhitespace;
    obj[QStringLiteral("caseInsensitive")] = caseInsensitive;
    obj[QStringLiteral("createNewTab")] = createNewTab;

    QJsonArray detArr;
    for (const auto& rc : returnColumnsDetailed) {
        QJsonObject o;
        o[QStringLiteral("sourceColumn")] = rc.sourceColumn;
        o[QStringLiteral("outputAlias")] = rc.outputAlias;
        detArr.append(o);
    }
    obj[QStringLiteral("returnColumnsDetailed")] = detArr;

    return obj;
}

LookupConfig LookupConfig::fromJson(const QJsonObject& obj)
{
    LookupConfig cfg;
    cfg.sourceTable = obj[QStringLiteral("sourceTable")].toString();
    cfg.sourceKeyColumn = obj[QStringLiteral("sourceKeyColumn")].toString();
    cfg.targetFile = obj[QStringLiteral("targetFile")].toString();
    cfg.targetTable = obj[QStringLiteral("targetTable")].toString();
    cfg.targetKeyColumn = obj[QStringLiteral("targetKeyColumn")].toString();
    cfg.resultTableName = obj[QStringLiteral("resultTableName")].toString();
    cfg.trimWhitespace = obj[QStringLiteral("trimWhitespace")].toBool();
    cfg.caseInsensitive = obj[QStringLiteral("caseInsensitive")].toBool();
    cfg.createNewTab = obj[QStringLiteral("createNewTab")].toBool(true);

    for (const auto& v : obj[QStringLiteral("returnColumns")].toArray())
        cfg.returnColumns << v.toString();

    for (const auto& v : obj[QStringLiteral("returnColumnsDetailed")].toArray()) {
        const QJsonObject o = v.toObject();
        LookupReturnColumn rc;
        rc.sourceColumn = o[QStringLiteral("sourceColumn")].toString();
        rc.outputAlias = o[QStringLiteral("outputAlias")].toString();
        cfg.returnColumnsDetailed.push_back(rc);
    }

    return cfg;
}

} // namespace csvforge
