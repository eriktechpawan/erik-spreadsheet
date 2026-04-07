#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <vector>

namespace csvforge {

enum class PivotAggregation {
    Sum,
    Count,
    Average,
    Min,
    Max,
    CountDistinct
};

enum class PivotOutputTarget {
    NewTab,
    TemporaryTable,
    ExportFile
};

struct PivotValueField {
    QString columnName;
    PivotAggregation aggregation = PivotAggregation::Sum;
    QString alias;  // optional output alias

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
    bool includeGrandTotals = false;
    bool includeSubtotals = false;
    bool sortByValue = false;
    bool showRowCounts = false;
    int previewLimit = 100;

    bool isValid() const;

    QJsonObject toJson() const;
    static PivotConfig fromJson(const QJsonObject& obj);
};

/// Convert aggregation to/from string.
QString pivotAggregationToString(PivotAggregation agg);
PivotAggregation pivotAggregationFromString(const QString& str);

} // namespace csvforge
