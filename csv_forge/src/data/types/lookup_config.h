#pragma once

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

struct LookupConfig {
    QString sourceTable;
    QString sourceKeyColumn;
    QString targetFile;       // path to target CSV
    QString targetTable;      // or table name if already loaded
    QString targetKeyColumn;
    QStringList returnColumns;
    LookupType type = LookupType::ExactMatch;
    LookupOutputTarget outputTarget = LookupOutputTarget::AppendColumns;
    QString resultTableName;

    bool isValid() const;
};

} // namespace csvforge
