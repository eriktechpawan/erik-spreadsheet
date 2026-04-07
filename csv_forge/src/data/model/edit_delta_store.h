#pragma once

#include <QObject>
#include <QPair>
#include <QString>
#include <QVariant>
#include <QVector>
#include <vector>

namespace csvforge {

enum class EditType {
    CellEdit,
    RowInsert,
    RowDelete,
    RowDuplicate,
    ColumnAdd,
    ColumnRename,
    ColumnDelete,
    ClearCells,
    FillDown,
    PasteClipboard
};

struct EditDelta {
    EditType type = EditType::CellEdit;
    qint64 row = -1;
    int column = -1;
    QVariant oldValue;
    QVariant newValue;
    QString columnName;
    QString newColumnName;
    QVector<QPair<qint64, int>> affectedCells; // for multi-cell operations
    QVector<QVector<QVariant>> oldData;         // for undo of multi-row ops
};

class EditDeltaStore : public QObject {
    Q_OBJECT
public:
    explicit EditDeltaStore(QObject* parent = nullptr);

    // Apply edits
    void recordCellEdit(qint64 row, int col, const QVariant& oldVal,
                        const QVariant& newVal);
    void recordRowInsert(qint64 row, const QVector<QVariant>& data);
    void recordRowDelete(qint64 row, const QVector<QVariant>& oldData);
    void recordRowDuplicate(qint64 sourceRow, qint64 newRow);
    void recordColumnAdd(const QString& name, int index);
    void recordColumnRename(int col, const QString& oldName, const QString& newName);
    void recordColumnDelete(int col, const QString& name,
                            const QVector<QVariant>& oldData);
    void recordClearCells(const QVector<QPair<qint64, int>>& cells,
                          const QVector<QVariant>& oldValues);
    void recordFillDown(int col, qint64 startRow, qint64 endRow,
                        const QVariant& value, const QVector<QVariant>& oldValues);

    // Undo/redo
    bool canUndo() const;
    bool canRedo() const;
    EditDelta undo();
    EditDelta redo();

    // State
    bool isDirty() const;
    void markSaved();
    int undoCount() const;
    int redoCount() const;
    void clear();

signals:
    void dirtyStateChanged(bool dirty);
    void undoRedoStateChanged(bool canUndo, bool canRedo);

private:
    std::vector<EditDelta> m_undoStack;
    std::vector<EditDelta> m_redoStack;
    int m_savedIndex = 0;

    void pushDelta(const EditDelta& delta);
};

} // namespace csvforge
