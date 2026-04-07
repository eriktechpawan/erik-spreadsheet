#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace csvforge {

enum class LookupType {
    ExactMatch,
    LeftJoin
};

enum class LookupOutputTarget {
    AppendColumns,
    NewResultTable
};

struct LookupReturnColumn {
    QString sourceColumn;
    QString outputAlias;  // empty = same as source
};

struct LookupConfig {
    QString sourceTable;
    QString sourceKeyColumn;
    QString targetFile;       // path to target CSV
    QString targetTable;      // or table name if already loaded
    QString targetKeyColumn;
    QStringList returnColumns;
    std::vector<LookupReturnColumn> returnColumnsDetailed;
    LookupType type = LookupType::ExactMatch;
    LookupOutputTarget outputTarget = LookupOutputTarget::AppendColumns;
    QString resultTableName;
    bool trimWhitespace = false;
    bool caseInsensitive = false;
    bool createNewTab = true;

    bool isValid() const;

    QJsonObject toJson() const;
    static LookupConfig fromJson(const QJsonObject& obj);
};

} // namespace csvforge
