#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>
#include <vector>

#include "data/model/dataset_descriptor.h"
#include "data/model/filter_state.h"
#include "data/model/sort_state.h"
#include "data/types/column_schema.h"
#include "data/types/filter_rule.h"
#include "data/types/lookup_config.h"
#include "data/types/pivot_config.h"

namespace csvforge {

class TableModel;
class DuckDBEngine;
struct TransformationPipeline;
struct FormulaConfig;

/// Holds all per-tab state: dataset metadata, filter/sort, schema, pipeline.
class TabState : public QObject {
    Q_OBJECT
public:
    explicit TabState(QObject* parent = nullptr);
    ~TabState() override;

    // --- Dataset descriptor ---
    const DatasetDescriptor& descriptor() const;
    void setDescriptor(const DatasetDescriptor& desc);

    // --- Schema ---
    const ColumnSchemaList& schema() const;
    void setSchema(const ColumnSchemaList& schema);

    // --- Filter / Sort ---
    FilterState* filterState() const;
    SortState* sortState() const;

    // --- Hidden / column order ---
    QStringList hiddenColumns() const;
    void setHiddenColumns(const QStringList& cols);
    QStringList columnOrder() const;
    void setColumnOrder(const QStringList& order);

    // --- Dirty (unsaved changes) ---
    bool isDirty() const;
    void setDirty(bool dirty);

    // --- Pipeline reference (owned externally) ---
    void setPipelineJson(const QJsonObject& json);
    QJsonObject pipelineJson() const;

    // --- Serialization ---
    QJsonObject toJson() const;
    static std::unique_ptr<TabState> fromJson(const QJsonObject& obj,
                                              QObject* parent = nullptr);

signals:
    void dirtyChanged(bool dirty);
    void descriptorChanged();

private:
    DatasetDescriptor m_descriptor;
    ColumnSchemaList m_schema;
    FilterState* m_filterState = nullptr;
    SortState* m_sortState = nullptr;
    QStringList m_hiddenColumns;
    QStringList m_columnOrder;
    bool m_dirty = false;
    QJsonObject m_pipelineJson;
};

} // namespace csvforge
