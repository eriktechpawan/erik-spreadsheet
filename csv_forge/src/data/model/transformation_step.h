#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <variant>
#include <vector>

#include "data/types/filter_rule.h"
#include "data/types/formula_config.h"
#include "data/types/lookup_config.h"
#include "data/types/pivot_config.h"

namespace csvforge {

/// Type of a single transformation step.
enum class TransformationType {
    Source,           ///< Load source CSV
    Filter,           ///< Apply filter rules
    Sort,             ///< Apply sort columns
    CalculatedColumn, ///< Add a calculated column
    Lookup,           ///< Join with another table
    Pivot,            ///< Pivot transformation
    HideColumns,      ///< Hide/reorder columns
    Rename            ///< Rename columns
};

/// A single step in the transformation pipeline.
struct TransformationStep {
    TransformationType type = TransformationType::Source;
    QString description;      ///< Human-readable summary

    // Type-specific payloads (stored as JSON for flexibility)
    QJsonObject config;

    // Convenience factories
    static TransformationStep makeSource(const QString& filePath,
                                         const QString& tableName);
    static TransformationStep makeFilter(const FilterGroup& filters);
    static TransformationStep makeSort(const std::vector<SortColumn>& sorts);
    static TransformationStep makeCalculatedColumn(const CalculatedColumnDef& def);
    static TransformationStep makeLookup(const LookupConfig& config);
    static TransformationStep makePivot(const PivotConfig& config);
    static TransformationStep makeHideColumns(const QStringList& hidden);

    QJsonObject toJson() const;
    static TransformationStep fromJson(const QJsonObject& obj);
};

/// Convert TransformationType to/from string.
QString transformationTypeToString(TransformationType type);
TransformationType transformationTypeFromString(const QString& str);

} // namespace csvforge
