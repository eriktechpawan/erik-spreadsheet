#pragma once

#include <QString>
#include <QStringList>
#include <vector>

namespace csvforge {

enum class PivotAggregation {
    Sum,
    Count,
    Average,
    Min,
    Max
};

enum class PivotOutputTarget {
    NewTab,
    TemporaryTable,
    ExportFile
};

struct PivotValueField {
    QString columnName;
    PivotAggregation aggregation = PivotAggregation::Sum;

    QString toSQL() const;
    QString displayName() const;
};

struct PivotConfig {
    QStringList rowFields;
    QStringList columnFields;
    std::vector<PivotValueField> valueFields;
    PivotOutputTarget outputTarget = PivotOutputTarget::NewTab;
    QString outputPath;   // for ExportFile
    QString sourceTable;

    bool isValid() const;
};

} // namespace csvforge
