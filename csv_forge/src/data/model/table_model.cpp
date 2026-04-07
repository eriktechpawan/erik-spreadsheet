#include "data/model/table_model.h"
#include "data/duckdb/duckdb_engine.h"
#include "data/duckdb/query_builder.h"
#include "utils/logging.h"

#include <algorithm>

namespace csvforge {

TableModel::TableModel(DuckDBEngine* engine, QObject* parent)
    : QAbstractTableModel(parent)
    , m_engine(engine)
    , m_cache(this)
    , m_editStore(this)
{
}

// ---------------------------------------------------------------------------
// QAbstractTableModel overrides
// ---------------------------------------------------------------------------

int TableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    const qint64 count = (m_filterState && m_filterState->hasActiveFilter())
                             ? m_filteredRowCount
                             : m_totalRowCount;
    // QAbstractTableModel uses int; clamp to INT_MAX for safety
    return static_cast<int>(qMin(count, static_cast<qint64>(std::numeric_limits<int>::max())));
}

int TableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_visibleSchema.size());
}

QVariant TableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return {};
    }

    const qint64 row = index.row();
    const int col = index.column();

    if (col < 0 || col >= static_cast<int>(m_visibleSchema.size())) {
        return {};
    }

    if (m_cache.hasRow(row)) {
        const QVector<QVariant>& rowData = m_cache.getRow(row);
        // Map visible column index to the actual schema index
        const int actualCol = m_visibleSchema[static_cast<size_t>(col)].index;
        if (actualCol >= 0 && actualCol < rowData.size()) {
            return rowData[actualCol];
        }
        return {};
    }

    // Cache miss: trigger async fetch
    if (m_cache.needsFetch(row)) {
        fetchPage(m_cache.pageStartForRow(row));
    }

    return QVariant(); // placeholder until data arrives
}

QVariant TableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return {};
    }

    if (orientation == Qt::Horizontal) {
        if (section >= 0 && section < static_cast<int>(m_visibleSchema.size())) {
            return m_visibleSchema[static_cast<size_t>(section)].name;
        }
    } else {
        // Vertical header: 1-based row numbers
        return section + 1;
    }
    return {};
}

Qt::ItemFlags TableModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f = QAbstractTableModel::flags(index);
    if (m_editable && index.isValid()) {
        f |= Qt::ItemIsEditable;
    }
    return f;
}

bool TableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!m_editable || role != Qt::EditRole || !index.isValid()) {
        return false;
    }

    const qint64 row = index.row();
    const int visCol = index.column();

    if (visCol < 0 || visCol >= static_cast<int>(m_visibleSchema.size())) {
        return false;
    }

    const int actualCol = m_visibleSchema[static_cast<size_t>(visCol)].index;
    const QVariant oldValue = data(index, Qt::DisplayRole);

    if (oldValue == value) {
        return false; // no change
    }

    // Record the edit delta
    m_editStore.recordCellEdit(row, actualCol, oldValue, value);

    // Update DuckDB temp table via direct SQL
    if (m_engine && !m_tableName.isEmpty()) {
        const QString& colName = m_visibleSchema[static_cast<size_t>(visCol)].name;
        const QString sql = QStringLiteral(
            "UPDATE \"%1\" SET \"%2\" = %3 WHERE rowid = %4")
            .arg(m_tableName, colName,
                 value.isNull() ? QStringLiteral("NULL")
                                : QStringLiteral("'%1'").arg(value.toString()),
                 QString::number(row));
        m_engine->executeQuery(sql);
    }

    emit dataChanged(index, index, {role});
    return true;
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

void TableModel::setTableName(const QString& tableName)
{
    m_tableName = tableName;
}

void TableModel::setSchema(const ColumnSchemaList& schema)
{
    beginResetModel();
    m_schema = schema;
    rebuildVisibleSchema();
    endResetModel();
}

void TableModel::setTotalRowCount(qint64 count)
{
    if (m_totalRowCount != count) {
        m_totalRowCount = count;
        emit rowCountChanged(m_totalRowCount, m_filteredRowCount);
    }
}

void TableModel::setFilteredRowCount(qint64 count)
{
    if (m_filteredRowCount != count) {
        m_filteredRowCount = count;
        emit rowCountChanged(m_totalRowCount, m_filteredRowCount);
    }
}

// ---------------------------------------------------------------------------
// Mode
// ---------------------------------------------------------------------------

void TableModel::setEditable(bool editable)
{
    m_editable = editable;
}

bool TableModel::isEditable() const
{
    return m_editable;
}

// ---------------------------------------------------------------------------
// Table name / schema
// ---------------------------------------------------------------------------

QString TableModel::tableName() const
{
    return m_tableName;
}

const ColumnSchemaList& TableModel::schema() const
{
    return m_schema;
}

ColumnSchema TableModel::columnAt(int col) const
{
    if (col >= 0 && col < static_cast<int>(m_visibleSchema.size())) {
        return m_visibleSchema[static_cast<size_t>(col)];
    }
    return {};
}

// ---------------------------------------------------------------------------
// Refresh / cache
// ---------------------------------------------------------------------------

void TableModel::refresh()
{
    beginResetModel();
    m_cache.clear();
    endResetModel();
    emit dataRefreshed();
}

void TableModel::invalidateCache()
{
    m_cache.clear();
}

// ---------------------------------------------------------------------------
// Filter / Sort state
// ---------------------------------------------------------------------------

void TableModel::setFilterState(FilterState* filterState)
{
    if (m_filterState) {
        disconnect(m_filterState, nullptr, this, nullptr);
    }
    m_filterState = filterState;
    if (m_filterState) {
        connect(m_filterState, &FilterState::filterChanged,
                this, &TableModel::onFilterChanged);
    }
}

void TableModel::setSortState(SortState* sortState)
{
    if (m_sortState) {
        disconnect(m_sortState, nullptr, this, nullptr);
    }
    m_sortState = sortState;
    if (m_sortState) {
        connect(m_sortState, &SortState::sortChanged,
                this, &TableModel::onSortChanged);
    }
}

FilterState* TableModel::filterState() const
{
    return m_filterState;
}

SortState* TableModel::sortState() const
{
    return m_sortState;
}

// ---------------------------------------------------------------------------
// Row counts
// ---------------------------------------------------------------------------

qint64 TableModel::totalRowCount() const
{
    return m_totalRowCount;
}

qint64 TableModel::filteredRowCount() const
{
    return m_filteredRowCount;
}

// ---------------------------------------------------------------------------
// Edit store
// ---------------------------------------------------------------------------

EditDeltaStore* TableModel::editStore()
{
    return &m_editStore;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void TableModel::onPageFetched(qint64 startRow, const QVector<QVector<QVariant>>& rows)
{
    m_cache.insertPage(startRow, rows);

    // Notify views that the fetched region has new data
    const int firstRow = static_cast<int>(qMax(qint64(0), startRow));
    const int lastRow = static_cast<int>(qMin(startRow + rows.size() - 1,
                                               static_cast<qint64>(rowCount() - 1)));
    if (lastRow >= firstRow) {
        emit dataChanged(index(firstRow, 0),
                         index(lastRow, columnCount() - 1));
    }
}

void TableModel::onFilterChanged()
{
    beginResetModel();
    m_cache.clear();

    // Update filtered row count from engine
    if (m_engine && !m_tableName.isEmpty() && m_filterState) {
        const QString where = m_filterState->toWhereClause();
        if (!where.isEmpty()) {
            m_filteredRowCount = m_engine->getFilteredRowCount(m_tableName, where);
        } else {
            m_filteredRowCount = m_totalRowCount;
        }
    }

    endResetModel();
    emit rowCountChanged(m_totalRowCount, m_filteredRowCount);
}

void TableModel::onSortChanged()
{
    beginResetModel();
    m_cache.clear();
    endResetModel();
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void TableModel::fetchPage(qint64 startRow) const
{
    Logger::instance().debug(
        LogCategory::Database,
        QStringLiteral("Requesting page fetch at row %1").arg(startRow));

    // Emit signal for async fetch by external worker
    emit const_cast<TableModel*>(this)->fetchRequested(
        startRow, m_cache.pageSize());
}

void TableModel::rebuildVisibleSchema()
{
    m_visibleSchema.clear();
    for (const auto& col : m_schema) {
        if (col.visible) {
            m_visibleSchema.push_back(col);
        }
    }
}

QString TableModel::buildDataQuery(qint64 offset, int limit) const
{
    QueryBuilder qb;
    qb.selectAll().from(m_tableName);

    if (m_filterState && m_filterState->hasActiveFilter()) {
        qb.where(m_filterState->currentFilter());
    }

    if (m_sortState && m_sortState->hasActiveSort()) {
        std::vector<std::pair<QString, bool>> sorts;
        for (const auto& sc : m_sortState->columns()) {
            sorts.emplace_back(sc.columnName, sc.ascending);
        }
        qb.orderBy(sorts);
    }

    qb.limit(limit).offset(offset);
    return qb.build();
}

} // namespace csvforge
