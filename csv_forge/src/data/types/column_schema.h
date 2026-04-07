#pragma once

#include <QString>
#include <QVariant>
#include <vector>

namespace csvforge {

enum class ColumnType {
    Text,
    Integer,
    Double,
    Boolean,
    Date,
    DateTime,
    Unknown
};

struct ColumnSchema {
    int index = -1;
    QString name;
    ColumnType type = ColumnType::Unknown;
    QString duckdbType;  // raw DuckDB type string
    bool visible = true;
    int displayOrder = -1;
    int width = 120;
    bool frozen = false;

    static ColumnType fromDuckDBType(const QString& dbType);
    static QString typeToString(ColumnType t);
    static ColumnType typeFromString(const QString& str);
};

using ColumnSchemaList = std::vector<ColumnSchema>;

} // namespace csvforge
