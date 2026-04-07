#pragma once
#include <QTableView>
#include <QVector>
#include <QPair>

namespace csvforge {

class TableModel;

class DataTableView : public QTableView {
    Q_OBJECT
public:
    explicit DataTableView(QWidget* parent = nullptr);

    void setTableModel(TableModel* model);

    QVector<qint64> selectedRowIndices() const;
    QVector<QPair<qint64, int>> selectedCells() const;
    int selectedRowCount() const;

    void hideColumn(int col);
    void showColumn(int col);
    void setColumnOrder(const QStringList& order);
    QStringList visibleColumns() const;
    int visibleColumnCount() const;

    void setContextMenuEnabled(bool enabled);
    void setRowNumbersVisible(bool visible);

signals:
    void cellDoubleClicked(qint64 row, int col);
    void columnHeaderClicked(int col, Qt::MouseButton button);
    void columnHeaderRightClicked(int col, const QPoint& pos);
    void selectionCountChanged(int rowCount);
    void requestColumnStats(const QString& columnName);
    void requestSort(const QString& columnName);
    void requestFilter(const QString& columnName, const QVariant& value);
    void requestHideColumn(int col);
    void scrolledToRow(qint64 row);

protected:
    void scrollContentsBy(int dx, int dy) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    TableModel* m_model = nullptr;
    bool m_contextMenuEnabled = true;
    bool m_rowNumbersVisible = true;

    void setupAppearance();
    void createCellContextMenu(const QPoint& pos, const QModelIndex& index);
    void createHeaderContextMenu(const QPoint& pos, int column);
    void onVerticalScrollChanged(int value);
};

} // namespace csvforge
