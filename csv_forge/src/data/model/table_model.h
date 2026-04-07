#pragma once

#include <QAbstractTableModel>
#include <QVariant>
#include <QVector>

#include "data/model/edit_delta_store.h"
#include "data/model/filter_state.h"
#include "data/model/row_window_cache.h"
#include "data/model/sort_state.h"
#include "data/types/column_schema.h"

namespace csvforge {

class DuckDBEngine;

class TableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit TableModel(DuckDBEngine* engine, QObject* parent = nullptr);

    // QAbstractTableModel overrides
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value,
                 int role = Qt::EditRole) override;

    // Setup
    void setTableName(const QString& tableName);
    void setSchema(const ColumnSchemaList& schema);
    void setTotalRowCount(qint64 count);
    void setFilteredRowCount(qint64 count);

    // Mode
    void setEditable(bool editable);
    bool isEditable() const;

    // Table name
    QString tableName() const;

    // Refresh data (refetch current window)
    void refresh();
    void invalidateCache();

    // Schema access
    const ColumnSchemaList& schema() const;
    ColumnSchema columnAt(int col) const;

    // Filter/sort state
    void setFilterState(FilterState* filterState);
    void setSortState(SortState* sortState);
    FilterState* filterState() const;
    SortState* sortState() const;

    // Row counts
    qint64 totalRowCount() const;
    qint64 filteredRowCount() const;

    // Edit support
    EditDeltaStore* editStore();

signals:
    void dataRefreshed();
    void fetchRequested(qint64 startRow, int count);
    void rowCountChanged(qint64 total, qint64 filtered);

public slots:
    void onPageFetched(qint64 startRow, const QVector<QVector<QVariant>>& rows);
    void onFilterChanged();
    void onSortChanged();

private:
    DuckDBEngine* m_engine;
    QString m_tableName;
    ColumnSchemaList m_schema;
    ColumnSchemaList m_visibleSchema; // cached visible-only columns
    qint64 m_totalRowCount = 0;
    qint64 m_filteredRowCount = 0;
    bool m_editable = false;

    RowWindowCache m_cache;
    FilterState* m_filterState = nullptr;
    SortState* m_sortState = nullptr;
    EditDeltaStore m_editStore;

    void fetchPage(qint64 startRow) const;
    void rebuildVisibleSchema();
    QString buildDataQuery(qint64 offset, int limit) const;
};

} // namespace csvforge
