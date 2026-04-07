#pragma once

#include "data/duckdb/duckdb_engine.h"
#include "data/types/column_schema.h"
#include "data/types/pivot_config.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>

namespace csvforge {

// ---------------------------------------------------------------------------
// PivotTableDialog – configure pivot table row/column/value fields
// ---------------------------------------------------------------------------

class PivotTableDialog : public QDialog {
    Q_OBJECT

public:
    explicit PivotTableDialog(const ColumnSchemaList& schema,
                              DuckDBEngine* engine,
                              QWidget* parent = nullptr);
    ~PivotTableDialog() override = default;

    PivotConfig getConfig() const;

private slots:
    void onAddRowField();
    void onRemoveRowField();
    void onAddColumnField();
    void onRemoveColumnField();
    void onAddValueField();
    void onRemoveValueField();
    void onOutputTargetChanged(int index);
    void onBrowseExport();
    void onPreview();

private:
    void buildUI();
    QGroupBox* buildRowFieldsGroup();
    QGroupBox* buildColumnFieldsGroup();
    QGroupBox* buildValueFieldsGroup();

    QString selectedAvailableField() const;

    // Left panel
    QListWidget* m_availableList = nullptr;

    // Right panel field lists
    QListWidget* m_rowFieldsList    = nullptr;
    QListWidget* m_columnFieldsList = nullptr;
    QListWidget* m_valueFieldsList  = nullptr;

    QPushButton* m_addRowBtn    = nullptr;
    QPushButton* m_removeRowBtn = nullptr;
    QPushButton* m_addColBtn    = nullptr;
    QPushButton* m_removeColBtn = nullptr;
    QPushButton* m_addValBtn    = nullptr;
    QPushButton* m_removeValBtn = nullptr;

    // Output section
    QComboBox*   m_outputTargetCombo = nullptr;
    QLineEdit*   m_exportPathEdit    = nullptr;
    QPushButton* m_browseExportBtn   = nullptr;
    QPushButton* m_previewBtn        = nullptr;
    QTableWidget* m_previewTable     = nullptr;

    QDialogButtonBox* m_buttonBox = nullptr;

    // Data
    ColumnSchemaList m_schema;
    DuckDBEngine*    m_engine;
    // Store aggregation for each value field
    QVector<PivotAggregation> m_valueAggregations;
};

} // namespace csvforge
