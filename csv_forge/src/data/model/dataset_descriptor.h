#pragma once

#include <QString>
#include <QStringList>

namespace csvforge {

/// Describes the origin/type of a dataset tab.
enum class DatasetType {
    Source,      ///< Original imported CSV/file
    Filtered,    ///< Filtered view of another dataset
    Pivot,       ///< Result of a pivot operation
    Lookup,      ///< Result of a lookup/join operation
    Calculated,  ///< Dataset with calculated columns added
    Derived      ///< Generic derived dataset
};

/// Lightweight descriptor attached to every open dataset/tab.
struct DatasetDescriptor {
    QString id;                ///< Unique identifier (generated UUID)
    QString displayName;       ///< User-visible tab name
    DatasetType type = DatasetType::Source;
    QString tableName;         ///< DuckDB table name holding data
    QString sourceFilePath;    ///< Original CSV path (Source type)
    QString parentDatasetId;   ///< Dataset this was derived from
    qint64 rowCount = 0;
    int columnCount = 0;

    /// Generate a short type badge string.
    QString typeBadge() const;

    /// Generate tooltip text summarizing the dataset.
    QString tooltipText() const;
};

/// Convert DatasetType to a display string.
QString datasetTypeToString(DatasetType type);

/// Convert a string back to DatasetType.
DatasetType datasetTypeFromString(const QString& str);

} // namespace csvforge
