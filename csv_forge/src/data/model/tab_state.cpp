#include "data/model/tab_state.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>

namespace csvforge {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TabState::TabState(QObject* parent)
    : QObject(parent)
    , m_filterState(new FilterState(this))
    , m_sortState(new SortState(this))
{
}

TabState::~TabState() = default;

// ---------------------------------------------------------------------------
// Descriptor
// ---------------------------------------------------------------------------

const DatasetDescriptor& TabState::descriptor() const { return m_descriptor; }

void TabState::setDescriptor(const DatasetDescriptor& desc)
{
    m_descriptor = desc;
    emit descriptorChanged();
}

// ---------------------------------------------------------------------------
// Schema
// ---------------------------------------------------------------------------

const ColumnSchemaList& TabState::schema() const { return m_schema; }
void TabState::setSchema(const ColumnSchemaList& schema) { m_schema = schema; }

// ---------------------------------------------------------------------------
// Filter / Sort
// ---------------------------------------------------------------------------

FilterState* TabState::filterState() const { return m_filterState; }
SortState* TabState::sortState() const { return m_sortState; }

// ---------------------------------------------------------------------------
// Hidden / column order
// ---------------------------------------------------------------------------

QStringList TabState::hiddenColumns() const { return m_hiddenColumns; }
void TabState::setHiddenColumns(const QStringList& cols) { m_hiddenColumns = cols; }

QStringList TabState::columnOrder() const { return m_columnOrder; }
void TabState::setColumnOrder(const QStringList& order) { m_columnOrder = order; }

// ---------------------------------------------------------------------------
// Dirty
// ---------------------------------------------------------------------------

bool TabState::isDirty() const { return m_dirty; }
void TabState::setDirty(bool dirty)
{
    if (m_dirty != dirty) {
        m_dirty = dirty;
        emit dirtyChanged(dirty);
    }
}

// ---------------------------------------------------------------------------
// Pipeline JSON
// ---------------------------------------------------------------------------

void TabState::setPipelineJson(const QJsonObject& json) { m_pipelineJson = json; }
QJsonObject TabState::pipelineJson() const { return m_pipelineJson; }

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------

QJsonObject TabState::toJson() const
{
    QJsonObject obj;
    // Descriptor
    QJsonObject desc;
    desc[QStringLiteral("id")] = m_descriptor.id;
    desc[QStringLiteral("displayName")] = m_descriptor.displayName;
    desc[QStringLiteral("type")] = datasetTypeToString(m_descriptor.type);
    desc[QStringLiteral("tableName")] = m_descriptor.tableName;
    desc[QStringLiteral("sourceFilePath")] = m_descriptor.sourceFilePath;
    desc[QStringLiteral("parentDatasetId")] = m_descriptor.parentDatasetId;
    desc[QStringLiteral("rowCount")] = m_descriptor.rowCount;
    desc[QStringLiteral("columnCount")] = m_descriptor.columnCount;
    obj[QStringLiteral("descriptor")] = desc;

    // Schema
    QJsonArray schemaArr;
    for (const auto& col : m_schema) {
        QJsonObject c;
        c[QStringLiteral("index")] = col.index;
        c[QStringLiteral("name")] = col.name;
        c[QStringLiteral("type")] = ColumnSchema::typeToString(col.type);
        c[QStringLiteral("duckdbType")] = col.duckdbType;
        c[QStringLiteral("visible")] = col.visible;
        c[QStringLiteral("displayOrder")] = col.displayOrder;
        c[QStringLiteral("width")] = col.width;
        c[QStringLiteral("frozen")] = col.frozen;
        schemaArr.append(c);
    }
    obj[QStringLiteral("schema")] = schemaArr;

    // Hidden / column order
    obj[QStringLiteral("hiddenColumns")] = QJsonArray::fromStringList(m_hiddenColumns);
    obj[QStringLiteral("columnOrder")] = QJsonArray::fromStringList(m_columnOrder);

    // Filter
    // Store filter as where-clause for simplicity; full FilterGroup serialization
    // is handled by session_state.
    obj[QStringLiteral("filterWhereClause")] = m_filterState->toWhereClause();

    // Sort
    QJsonArray sortArr;
    for (const auto& sc : m_sortState->columns()) {
        QJsonObject so;
        so[QStringLiteral("column")] = sc.columnName;
        so[QStringLiteral("ascending")] = sc.ascending;
        sortArr.append(so);
    }
    obj[QStringLiteral("sorts")] = sortArr;

    // Pipeline
    if (!m_pipelineJson.isEmpty()) {
        obj[QStringLiteral("pipeline")] = m_pipelineJson;
    }

    obj[QStringLiteral("dirty")] = m_dirty;
    return obj;
}

std::unique_ptr<TabState> TabState::fromJson(const QJsonObject& obj, QObject* parent)
{
    auto tab = std::make_unique<TabState>(parent);

    // Descriptor
    const QJsonObject desc = obj[QStringLiteral("descriptor")].toObject();
    DatasetDescriptor d;
    d.id = desc[QStringLiteral("id")].toString();
    d.displayName = desc[QStringLiteral("displayName")].toString();
    d.type = datasetTypeFromString(desc[QStringLiteral("type")].toString());
    d.tableName = desc[QStringLiteral("tableName")].toString();
    d.sourceFilePath = desc[QStringLiteral("sourceFilePath")].toString();
    d.parentDatasetId = desc[QStringLiteral("parentDatasetId")].toString();
    d.rowCount = desc[QStringLiteral("rowCount")].toInteger();
    d.columnCount = desc[QStringLiteral("columnCount")].toInt();
    tab->setDescriptor(d);

    // Schema
    ColumnSchemaList schema;
    const QJsonArray schemaArr = obj[QStringLiteral("schema")].toArray();
    for (const auto& v : schemaArr) {
        const QJsonObject c = v.toObject();
        ColumnSchema col;
        col.index = c[QStringLiteral("index")].toInt();
        col.name = c[QStringLiteral("name")].toString();
        col.type = ColumnSchema::fromDuckDBType(c[QStringLiteral("type")].toString());
        col.duckdbType = c[QStringLiteral("duckdbType")].toString();
        col.visible = c[QStringLiteral("visible")].toBool(true);
        col.displayOrder = c[QStringLiteral("displayOrder")].toInt(-1);
        col.width = c[QStringLiteral("width")].toInt(120);
        col.frozen = c[QStringLiteral("frozen")].toBool(false);
        schema.push_back(col);
    }
    tab->setSchema(schema);

    // Hidden / column order
    QStringList hidden;
    for (const auto& v : obj[QStringLiteral("hiddenColumns")].toArray())
        hidden << v.toString();
    tab->setHiddenColumns(hidden);

    QStringList colOrder;
    for (const auto& v : obj[QStringLiteral("columnOrder")].toArray())
        colOrder << v.toString();
    tab->setColumnOrder(colOrder);

    // Sort
    const QJsonArray sortArr = obj[QStringLiteral("sorts")].toArray();
    for (const auto& v : sortArr) {
        const QJsonObject so = v.toObject();
        tab->sortState()->addSort(so[QStringLiteral("column")].toString(),
                                  so[QStringLiteral("ascending")].toBool(true));
    }

    // Pipeline
    if (obj.contains(QStringLiteral("pipeline"))) {
        tab->setPipelineJson(obj[QStringLiteral("pipeline")].toObject());
    }

    return tab;
}

} // namespace csvforge
