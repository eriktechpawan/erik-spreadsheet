#pragma once

#include "data/duckdb/duckdb_engine.h"
#include "utils/csv_sniffer.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>

namespace csvforge {

// ---------------------------------------------------------------------------
// ImportDialog – CSV import configuration with auto-detection and preview
// ---------------------------------------------------------------------------

class ImportDialog : public QDialog {
    Q_OBJECT

public:
    /// Construct the dialog.  If \a filePath is non-empty the file is
    /// auto-detected and a preview is loaded immediately.
    explicit ImportDialog(const QString& filePath = {},
                          QWidget* parent = nullptr);
    ~ImportDialog() override = default;

    /// Return the import options configured by the user.
    CSVImportOptions importOptions() const;

public slots:
    void onBrowse();
    void onRefreshPreview();
    void onDelimiterChanged(int index);

private:
    // ---- UI helpers -------------------------------------------------------
    void buildUI();
    QGroupBox* buildFileSection();
    QGroupBox* buildAutoDetectSection();
    QGroupBox* buildOverrideSection();
    QWidget*   buildPreviewSection();

    // ---- Logic helpers ----------------------------------------------------
    CSVImportOptions buildOptionsFromUI() const;
    void loadPreview();
    void applyDetectedFormat(const CSVFormat& fmt);

    // ---- File path row ----------------------------------------------------
    QLineEdit*   m_filePath       = nullptr;
    QPushButton* m_browseBtn      = nullptr;

    // ---- Auto-detect labels -----------------------------------------------
    QLabel* m_detDelimiter = nullptr;
    QLabel* m_detQuote     = nullptr;
    QLabel* m_detHeader    = nullptr;
    QLabel* m_detEncoding  = nullptr;

    // ---- Manual override controls -----------------------------------------
    QComboBox* m_delimiterCombo  = nullptr;
    QLineEdit* m_customDelimiter = nullptr;
    QComboBox* m_quoteCombo      = nullptr;
    QComboBox* m_encodingCombo   = nullptr;
    QCheckBox* m_headerCheck     = nullptr;
    QSpinBox*  m_skipRowsSpin    = nullptr;
    QCheckBox* m_ignoreErrorsCheck = nullptr;

    // ---- Preview ----------------------------------------------------------
    QTableWidget*    m_previewTable  = nullptr;
    QPushButton*     m_refreshBtn    = nullptr;
    QDialogButtonBox* m_buttonBox    = nullptr;

    // ---- State ------------------------------------------------------------
    static constexpr int PreviewRows = 20;
};

} // namespace csvforge
