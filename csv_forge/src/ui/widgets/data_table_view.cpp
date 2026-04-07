#include "data_table_view.h"
#include "data/model/table_model.h"

#include <QContextMenuEvent>
#include <QFont>
#include <QHeaderView>
#include <QMenu>
#include <QScrollBar>

namespace csvforge {

DataTableView::DataTableView(QWidget* parent)
    : QTableView(parent)
{
    setupAppearance();
    connect(verticalScrollBar(), &QScrollBar::valueChanged,
            this, &DataTableView::onVerticalScrollChanged);
}

void DataTableView::setupAppearance()
{
    setAlternatingRowColors(true);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    QFont tableFont = font();
    tableFont.setPointSize(13);
    setFont(tableFont);

    verticalHeader()->setDefaultSectionSize(24);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setSectionsMovable(true);
    setShowGrid(true);
}

void DataTableView::setTableModel(TableModel* model)
{
    m_model = model;
    QTableView::setModel(model);

    horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(horizontalHeader(), &QHeaderView::sectionClicked,
            this, [this](int logicalIndex) {
                emit columnHeaderClicked(logicalIndex, Qt::LeftButton);
            });

    connect(horizontalHeader(), &QHeaderView::customContextMenuRequested,
            this, [this](const QPoint& pos) {
                int col = horizontalHeader()->logicalIndexAt(pos);
                if (col >= 0) {
                    createHeaderContextMenu(horizontalHeader()->mapToGlobal(pos), col);
                }
            });

    connect(selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this]() {
                emit selectionCountChanged(selectedRowCount());
            });
}

QVector<qint64> DataTableView::selectedRowIndices() const
{
    QVector<qint64> rows;
    if (!selectionModel()) {
        return rows;
    }
    const auto selected = selectionModel()->selectedRows();
    rows.reserve(selected.size());
    for (const auto& idx : selected) {
        rows.append(static_cast<qint64>(idx.row()));
    }
    return rows;
}

QVector<QPair<qint64, int>> DataTableView::selectedCells() const
{
    QVector<QPair<qint64, int>> cells;
    if (!selectionModel()) {
        return cells;
    }
    const auto indexes = selectionModel()->selectedIndexes();
    cells.reserve(indexes.size());
    for (const auto& idx : indexes) {
        cells.append({static_cast<qint64>(idx.row()), idx.column()});
    }
    return cells;
}

int DataTableView::selectedRowCount() const
{
    if (!selectionModel()) {
        return 0;
    }
    QSet<int> rows;
    const auto indexes = selectionModel()->selectedIndexes();
    for (const auto& idx : indexes) {
        rows.insert(idx.row());
    }
    return rows.size();
}

void DataTableView::hideColumn(int col)
{
    QTableView::hideColumn(col);
}

void DataTableView::showColumn(int col)
{
    QTableView::showColumn(col);
}

void DataTableView::setColumnOrder(const QStringList& order)
{
    if (!m_model) {
        return;
    }
    auto* header = horizontalHeader();
    for (int i = 0; i < order.size(); ++i) {
        for (int j = 0; j < m_model->columnCount(); ++j) {
            const QString colName = m_model->headerData(j, Qt::Horizontal).toString();
            if (colName == order[i]) {
                int visualIdx = header->visualIndex(j);
                if (visualIdx != i) {
                    header->moveSection(visualIdx, i);
                }
                break;
            }
        }
    }
}

QStringList DataTableView::visibleColumns() const
{
    QStringList cols;
    if (!m_model) {
        return cols;
    }
    auto* header = horizontalHeader();
    for (int i = 0; i < m_model->columnCount(); ++i) {
        int logical = header->logicalIndex(i);
        if (!header->isSectionHidden(logical)) {
            cols.append(m_model->headerData(logical, Qt::Horizontal).toString());
        }
    }
    return cols;
}

int DataTableView::visibleColumnCount() const
{
    if (!m_model) {
        return 0;
    }
    int count = 0;
    auto* header = horizontalHeader();
    for (int i = 0; i < m_model->columnCount(); ++i) {
        if (!header->isSectionHidden(i)) {
            ++count;
        }
    }
    return count;
}

void DataTableView::setContextMenuEnabled(bool enabled)
{
    m_contextMenuEnabled = enabled;
}

void DataTableView::setRowNumbersVisible(bool visible)
{
    m_rowNumbersVisible = visible;
    verticalHeader()->setVisible(visible);
}

void DataTableView::scrollContentsBy(int dx, int dy)
{
    QTableView::scrollContentsBy(dx, dy);
    if (dy != 0) {
        qint64 topRow = static_cast<qint64>(rowAt(0));
        if (topRow >= 0) {
            emit scrolledToRow(topRow);
        }
    }
}

void DataTableView::contextMenuEvent(QContextMenuEvent* event)
{
    if (!m_contextMenuEnabled) {
        QTableView::contextMenuEvent(event);
        return;
    }

    QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
        createCellContextMenu(event->globalPos(), index);
    }
    event->accept();
}

void DataTableView::createCellContextMenu(const QPoint& pos, const QModelIndex& index)
{
    QMenu menu(this);

    menu.addAction(tr("Copy"), this, [this, index]() {
        // Copy cell value to clipboard handled by parent
        Q_UNUSED(index);
    });

    menu.addAction(tr("Edit Cell"), this, [this, index]() {
        edit(index);
    });

    menu.addAction(tr("Clear Cell"), this, [this, index]() {
        if (m_model) {
            m_model->setData(index, QVariant(), Qt::EditRole);
        }
    });

    menu.addSeparator();

    menu.addAction(tr("Filter by this Value"), this, [this, index]() {
        if (m_model) {
            const QString colName = m_model->headerData(index.column(), Qt::Horizontal).toString();
            emit requestFilter(colName, index.data());
        }
    });

    menu.addAction(tr("Exclude this Value"), this, [this, index]() {
        Q_UNUSED(index);
    });

    menu.addSeparator();

    menu.addAction(tr("Column Stats"), this, [this, index]() {
        if (m_model) {
            const QString colName = m_model->headerData(index.column(), Qt::Horizontal).toString();
            emit requestColumnStats(colName);
        }
    });

    menu.exec(pos);
}

void DataTableView::createHeaderContextMenu(const QPoint& pos, int column)
{
    QMenu menu(this);

    const QString colName = m_model
        ? m_model->headerData(column, Qt::Horizontal).toString()
        : QString();

    menu.addAction(tr("Sort Ascending"), this, [this, colName]() {
        emit requestSort(colName);
    });

    menu.addAction(tr("Sort Descending"), this, [this, colName]() {
        emit requestSort(colName);
    });

    menu.addSeparator();

    menu.addAction(tr("Filter..."), this, [this, colName]() {
        emit requestFilter(colName, QVariant());
    });

    menu.addAction(tr("Column Statistics"), this, [this, colName]() {
        emit requestColumnStats(colName);
    });

    menu.addSeparator();

    menu.addAction(tr("Hide Column"), this, [this, column]() {
        emit requestHideColumn(column);
    });

    menu.exec(pos);
}

void DataTableView::onVerticalScrollChanged(int value)
{
    Q_UNUSED(value);
    qint64 firstVisible = static_cast<qint64>(rowAt(0));
    if (firstVisible >= 0) {
        emit scrolledToRow(firstVisible);
    }
}

} // namespace csvforge
