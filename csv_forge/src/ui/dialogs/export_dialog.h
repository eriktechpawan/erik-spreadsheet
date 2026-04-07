#pragma once

#include "data/duckdb/export_service.h"
#include "data/types/column_schema.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>

namespace csvforge {

// ---------------------------------------------------------------------------
// ExportDialog – configure export format, scope, and column selection
// ---------------------------------------------------------------------------

class ExportDialog : public QDialog {
    Q_OBJECT

public:
    enum class ExportScope { AllRows, FilteredRows, SelectedRows };

    explicit ExportDialog(const ColumnSchemaList& schema,
                          QWidget* parent = nullptr);
    ~ExportDialog() override = default;

    ExportService::ExportOptions exportOptions() const;

private slots:
    void onBrowse();
    void onFormatChanged(int index);
    void onColumnModeChanged();
    void onSelectAll();
    void onSelectNone();

private:
    void buildUI();
    QGroupBox* buildFileSection();
    QGroupBox* buildFormatSection();
    QGroupBox* buildScopeSection();
    QGroupBox* buildColumnSection();

    // ---- File -------------------------------------------------------------
    QLineEdit*   m_filePath  = nullptr;
    QPushButton* m_browseBtn = nullptr;

    // ---- Format -----------------------------------------------------------
    QComboBox* m_formatCombo    = nullptr;
    QComboBox* m_delimiterCombo = nullptr;
    QCheckBox* m_headerCheck    = nullptr;

    // ---- Scope ------------------------------------------------------------
    QRadioButton* m_allRowsRadio      = nullptr;
    QRadioButton* m_filteredRowsRadio = nullptr;
    QRadioButton* m_selectedRowsRadio = nullptr;

    // ---- Columns ----------------------------------------------------------
    QRadioButton* m_allColsRadio     = nullptr;
    QRadioButton* m_visibleColsRadio = nullptr;
    QRadioButton* m_customColsRadio  = nullptr;
    QListWidget*  m_columnList       = nullptr;
    QPushButton*  m_selectAllBtn     = nullptr;
    QPushButton*  m_selectNoneBtn    = nullptr;

    QDialogButtonBox* m_buttonBox = nullptr;

    // ---- Data -------------------------------------------------------------
    ColumnSchemaList m_schema;
};

} // namespace csvforge
