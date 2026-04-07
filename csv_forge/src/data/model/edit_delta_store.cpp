#include "data/model/edit_delta_store.h"

namespace csvforge {

EditDeltaStore::EditDeltaStore(QObject* parent)
    : QObject(parent)
{
}

void EditDeltaStore::recordCellEdit(qint64 row, int col, const QVariant& oldVal,
                                    const QVariant& newVal)
{
    EditDelta delta;
    delta.type = EditType::CellEdit;
    delta.row = row;
    delta.column = col;
    delta.oldValue = oldVal;
    delta.newValue = newVal;
    pushDelta(delta);
}

void EditDeltaStore::recordRowInsert(qint64 row, const QVector<QVariant>& data)
{
    EditDelta delta;
    delta.type = EditType::RowInsert;
    delta.row = row;
    delta.oldData.append(data);
    pushDelta(delta);
}

void EditDeltaStore::recordRowDelete(qint64 row, const QVector<QVariant>& oldData)
{
    EditDelta delta;
    delta.type = EditType::RowDelete;
    delta.row = row;
    delta.oldData.append(oldData);
    pushDelta(delta);
}

void EditDeltaStore::recordRowDuplicate(qint64 sourceRow, qint64 newRow)
{
    EditDelta delta;
    delta.type = EditType::RowDuplicate;
    delta.row = sourceRow;
    delta.newValue = QVariant::fromValue(newRow);
    pushDelta(delta);
}

void EditDeltaStore::recordColumnAdd(const QString& name, int index)
{
    EditDelta delta;
    delta.type = EditType::ColumnAdd;
    delta.column = index;
    delta.columnName = name;
    pushDelta(delta);
}

void EditDeltaStore::recordColumnRename(int col, const QString& oldName,
                                        const QString& newName)
{
    EditDelta delta;
    delta.type = EditType::ColumnRename;
    delta.column = col;
    delta.columnName = oldName;
    delta.newColumnName = newName;
    pushDelta(delta);
}

void EditDeltaStore::recordColumnDelete(int col, const QString& name,
                                        const QVector<QVariant>& oldData)
{
    EditDelta delta;
    delta.type = EditType::ColumnDelete;
    delta.column = col;
    delta.columnName = name;
    // Store column data as a single-row vector of all values in the column
    delta.oldData.append(oldData);
    pushDelta(delta);
}

void EditDeltaStore::recordClearCells(const QVector<QPair<qint64, int>>& cells,
                                      const QVector<QVariant>& oldValues)
{
    EditDelta delta;
    delta.type = EditType::ClearCells;
    delta.affectedCells = cells;
    // Store old values in a flat row within oldData
    delta.oldData.append(oldValues);
    pushDelta(delta);
}

void EditDeltaStore::recordFillDown(int col, qint64 startRow, qint64 endRow,
                                    const QVariant& value,
                                    const QVector<QVariant>& oldValues)
{
    EditDelta delta;
    delta.type = EditType::FillDown;
    delta.column = col;
    delta.row = startRow;
    delta.newValue = value;
    delta.oldData.append(oldValues);

    // Record affected cells for precise undo
    for (qint64 r = startRow; r <= endRow; ++r) {
        delta.affectedCells.append({r, col});
    }
    pushDelta(delta);
}

bool EditDeltaStore::canUndo() const
{
    return !m_undoStack.empty();
}

bool EditDeltaStore::canRedo() const
{
    return !m_redoStack.empty();
}

EditDelta EditDeltaStore::undo()
{
    if (m_undoStack.empty()) {
        return {};
    }

    EditDelta delta = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    m_redoStack.push_back(delta);

    const bool dirty = isDirty();
    emit undoRedoStateChanged(canUndo(), canRedo());
    emit dirtyStateChanged(dirty);

    return delta;
}

EditDelta EditDeltaStore::redo()
{
    if (m_redoStack.empty()) {
        return {};
    }

    EditDelta delta = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    m_undoStack.push_back(delta);

    const bool dirty = isDirty();
    emit undoRedoStateChanged(canUndo(), canRedo());
    emit dirtyStateChanged(dirty);

    return delta;
}

bool EditDeltaStore::isDirty() const
{
    return static_cast<int>(m_undoStack.size()) != m_savedIndex;
}

void EditDeltaStore::markSaved()
{
    m_savedIndex = static_cast<int>(m_undoStack.size());
    emit dirtyStateChanged(false);
}

int EditDeltaStore::undoCount() const
{
    return static_cast<int>(m_undoStack.size());
}

int EditDeltaStore::redoCount() const
{
    return static_cast<int>(m_redoStack.size());
}

void EditDeltaStore::clear()
{
    const bool wasDirty = isDirty();
    m_undoStack.clear();
    m_redoStack.clear();
    m_savedIndex = 0;

    emit undoRedoStateChanged(false, false);
    if (wasDirty) {
        emit dirtyStateChanged(false);
    }
}

void EditDeltaStore::pushDelta(const EditDelta& delta)
{
    m_undoStack.push_back(delta);
    m_redoStack.clear(); // new edit invalidates redo history

    emit undoRedoStateChanged(canUndo(), false);
    emit dirtyStateChanged(isDirty());
}

} // namespace csvforge
