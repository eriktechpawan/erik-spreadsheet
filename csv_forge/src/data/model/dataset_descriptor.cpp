#include "data/model/dataset_descriptor.h"

namespace csvforge {

QString DatasetDescriptor::typeBadge() const
{
    return datasetTypeToString(type);
}

QString DatasetDescriptor::tooltipText() const
{
    QString tip = QStringLiteral("%1 [%2]").arg(displayName, typeBadge());
    if (!sourceFilePath.isEmpty()) {
        tip += QStringLiteral("\nSource: %1").arg(sourceFilePath);
    }
    if (rowCount > 0) {
        tip += QStringLiteral("\nRows: %1").arg(rowCount);
    }
    if (columnCount > 0) {
        tip += QStringLiteral("\nColumns: %1").arg(columnCount);
    }
    return tip;
}

QString datasetTypeToString(DatasetType type)
{
    switch (type) {
    case DatasetType::Source:     return QStringLiteral("Source");
    case DatasetType::Filtered:  return QStringLiteral("Filtered");
    case DatasetType::Pivot:     return QStringLiteral("Pivot");
    case DatasetType::Lookup:    return QStringLiteral("Lookup");
    case DatasetType::Calculated:return QStringLiteral("Calculated");
    case DatasetType::Derived:   return QStringLiteral("Derived");
    }
    return QStringLiteral("Unknown");
}

DatasetType datasetTypeFromString(const QString& str)
{
    if (str == QLatin1String("Source"))     return DatasetType::Source;
    if (str == QLatin1String("Filtered"))   return DatasetType::Filtered;
    if (str == QLatin1String("Pivot"))      return DatasetType::Pivot;
    if (str == QLatin1String("Lookup"))     return DatasetType::Lookup;
    if (str == QLatin1String("Calculated")) return DatasetType::Calculated;
    if (str == QLatin1String("Derived"))    return DatasetType::Derived;
    return DatasetType::Source;
}

} // namespace csvforge
