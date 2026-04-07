#pragma once

#include "data/duckdb/duckdb_engine.h"
#include "data/types/column_schema.h"
#include "data/types/lookup_config.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

namespace csvforge {

// ---------------------------------------------------------------------------
// LookupWizardDialog – single-page lookup / join wizard
// ---------------------------------------------------------------------------

class LookupWizardDialog : public QDialog {
    Q_OBJECT

public:
    explicit LookupWizardDialog(const QString& sourceTable,
                                const ColumnSchemaList& sourceSchema,
                                DuckDBEngine* engine,
                                QWidget* parent = nullptr);
    ~LookupWizardDialog() override = default;

    LookupConfig getConfig() const;

private slots:
    void onBrowseTarget();
    void onTargetFileChanged();
    void onTargetTableChanged(int index);
    void onSelectAll();
    void onDeselectAll();
    void onOutputTargetChanged(int index);

private:
    void buildUI();
    QGroupBox* buildSourceSection();
    QGroupBox* buildTargetSection();
    QGroupBox* buildReturnColumnsSection();
    QGroupBox* buildOptionsSection();
    void populateTargetColumns(const ColumnSchemaList& schema);

    // Source
    QLabel*    m_sourceTableLabel = nullptr;
    QComboBox* m_sourceKeyCombo   = nullptr;

    // Target
    QLineEdit*   m_targetFileEdit   = nullptr;
    QPushButton* m_browseTargetBtn  = nullptr;
    QComboBox*   m_targetTableCombo = nullptr;
    QComboBox*   m_targetKeyCombo   = nullptr;

    // Return columns
    QListWidget* m_returnColumnsList = nullptr;
    QPushButton* m_selectAllBtn      = nullptr;
    QPushButton* m_deselectAllBtn    = nullptr;

    // Options
    QComboBox* m_matchTypeCombo    = nullptr;
    QComboBox* m_outputCombo       = nullptr;
    QLineEdit* m_resultNameEdit    = nullptr;

    QDialogButtonBox* m_buttonBox = nullptr;

    // Data
    QString          m_sourceTable;
    ColumnSchemaList m_sourceSchema;
    DuckDBEngine*    m_engine;
};

} // namespace csvforge
