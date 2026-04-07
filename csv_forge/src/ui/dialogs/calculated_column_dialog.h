#pragma once

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>

#include "data/formula/formula_engine.h"
#include "data/types/column_schema.h"

namespace csvforge {

class DuckDBEngine;

/// Dialog for creating/editing a calculated column using spreadsheet-style formulas.
class CalculatedColumnDialog : public QDialog {
    Q_OBJECT
public:
    explicit CalculatedColumnDialog(const ColumnSchemaList& schema,
                                    DuckDBEngine* engine,
                                    const QString& tableName,
                                    QWidget* parent = nullptr);
    ~CalculatedColumnDialog() override = default;

    CalculatedColumnDef getDefinition() const;

private slots:
    void onValidate();
    void onPreview();
    void onFormulaChanged();
    void onFunctionSelected(const QString& funcName);
    void onColumnDoubleClicked(QListWidgetItem* item);

private:
    void buildUI();
    QWidget* buildFormulaSection();
    QWidget* buildPaletteSection();
    QWidget* buildPreviewSection();
    void insertAtCursor(const QString& text);
    void showValidationResult(bool valid, const QString& message);

    ColumnSchemaList m_schema;
    DuckDBEngine* m_engine;
    QString m_tableName;
    FormulaEngine m_formulaEngine;

    // UI
    QLineEdit*    m_columnNameEdit  = nullptr;
    QTextEdit*    m_formulaEdit     = nullptr;
    QListWidget*  m_columnList      = nullptr;
    QListWidget*  m_functionList    = nullptr;
    QLabel*       m_helpLabel       = nullptr;
    QLabel*       m_outputTypeLabel = nullptr;
    QLabel*       m_validationLabel = nullptr;
    QPushButton*  m_validateBtn     = nullptr;
    QPushButton*  m_previewBtn      = nullptr;
    QTableWidget* m_previewTable    = nullptr;
    QComboBox*    m_applyModeCombo  = nullptr;
    QDialogButtonBox* m_buttonBox   = nullptr;
};

} // namespace csvforge
