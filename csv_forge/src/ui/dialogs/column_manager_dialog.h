#pragma once

#include "data/types/column_schema.h"

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>

namespace csvforge {

// ---------------------------------------------------------------------------
// ColumnManagerDialog – reorder, show/hide, resize, freeze columns
// ---------------------------------------------------------------------------

class ColumnManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit ColumnManagerDialog(const ColumnSchemaList& schema,
                                 QWidget* parent = nullptr);
    ~ColumnManagerDialog() override = default;

    ColumnSchemaList getSchema() const;

signals:
    void applied(const ColumnSchemaList& schema);

private slots:
    void onMoveUp();
    void onMoveDown();
    void onShowAll();
    void onHideAll();
    void onResetDefaults();
    void onApply();

private:
    void buildUI();
    void populateTable();
    void swapRows(int a, int b);

    QTableWidget* m_table = nullptr;

    QPushButton* m_moveUpBtn       = nullptr;
    QPushButton* m_moveDownBtn     = nullptr;
    QPushButton* m_showAllBtn      = nullptr;
    QPushButton* m_hideAllBtn      = nullptr;
    QPushButton* m_resetDefaultBtn = nullptr;

    QDialogButtonBox* m_buttonBox = nullptr;

    ColumnSchemaList m_schema;
    ColumnSchemaList m_originalSchema;
};

} // namespace csvforge
