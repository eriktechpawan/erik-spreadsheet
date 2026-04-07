#include "ui/dialogs/calculated_column_dialog.h"
#include "data/duckdb/duckdb_engine.h"
#include "data/formula/formula_sql_translator.h"
#include "utils/string_utils.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>

namespace csvforge {

CalculatedColumnDialog::CalculatedColumnDialog(const ColumnSchemaList& schema,
                                               DuckDBEngine* engine,
                                               const QString& tableName,
                                               QWidget* parent)
    : QDialog(parent)
    , m_schema(schema)
    , m_engine(engine)
    , m_tableName(tableName)
{
    setWindowTitle(tr("Calculated Column"));
    setMinimumSize(800, 600);
    m_formulaEngine.setSchema(schema);
    buildUI();
}

CalculatedColumnDef CalculatedColumnDialog::getDefinition() const
{
    std::vector<FormulaError> errors;
    auto def = m_formulaEngine.buildDefinition(
        m_columnNameEdit->text().trimmed(),
        m_formulaEdit->toPlainText().trimmed(),
        errors);
    return def;
}

// ---------------------------------------------------------------------------
// Build UI
// ---------------------------------------------------------------------------

void CalculatedColumnDialog::buildUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // Column name
    auto* nameLayout = new QHBoxLayout;
    nameLayout->addWidget(new QLabel(tr("Column Name:"), this));
    m_columnNameEdit = new QLineEdit(this);
    m_columnNameEdit->setPlaceholderText(tr("new_column"));
    nameLayout->addWidget(m_columnNameEdit);
    mainLayout->addLayout(nameLayout);

    // Splitter: formula/palette | preview
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // Left: formula editor + palette
    auto* leftWidget = new QWidget(splitter);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(buildFormulaSection());
    leftLayout->addWidget(buildPaletteSection());
    splitter->addWidget(leftWidget);

    // Right: preview
    splitter->addWidget(buildPreviewSection());
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter, 1);

    // Output type and validation status
    auto* statusLayout = new QHBoxLayout;
    m_outputTypeLabel = new QLabel(tr("Output type: Unknown"), this);
    m_validationLabel = new QLabel(this);
    statusLayout->addWidget(m_outputTypeLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(m_validationLabel);
    mainLayout->addLayout(statusLayout);

    // Apply mode
    auto* applyLayout = new QHBoxLayout;
    applyLayout->addWidget(new QLabel(tr("Apply to:"), this));
    m_applyModeCombo = new QComboBox(this);
    m_applyModeCombo->addItem(tr("Current tab"));
    m_applyModeCombo->addItem(tr("New tab"));
    applyLayout->addWidget(m_applyModeCombo);
    applyLayout->addStretch();
    mainLayout->addLayout(applyLayout);

    // Buttons
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, [this] {
        // Validate before accepting
        onValidate();
        if (m_validationLabel->text().startsWith(QLatin1String("✓"))) {
            accept();
        }
    });
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(m_buttonBox);
}

QWidget* CalculatedColumnDialog::buildFormulaSection()
{
    auto* group = new QGroupBox(tr("Formula"), this);
    auto* layout = new QVBoxLayout(group);

    m_formulaEdit = new QTextEdit(this);
    m_formulaEdit->setFont(QFont(QStringLiteral("Menlo"), 13));
    m_formulaEdit->setPlaceholderText(tr("[price] * [qty]"));
    m_formulaEdit->setMaximumHeight(100);
    layout->addWidget(m_formulaEdit);

    auto* btnLayout = new QHBoxLayout;
    m_validateBtn = new QPushButton(tr("Validate"), this);
    m_previewBtn = new QPushButton(tr("Preview"), this);
    btnLayout->addWidget(m_validateBtn);
    btnLayout->addWidget(m_previewBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(m_validateBtn, &QPushButton::clicked, this, &CalculatedColumnDialog::onValidate);
    connect(m_previewBtn, &QPushButton::clicked, this, &CalculatedColumnDialog::onPreview);
    connect(m_formulaEdit, &QTextEdit::textChanged, this, &CalculatedColumnDialog::onFormulaChanged);

    return group;
}

QWidget* CalculatedColumnDialog::buildPaletteSection()
{
    auto* group = new QGroupBox(tr("Insert"), this);
    auto* layout = new QHBoxLayout(group);

    // Column list
    auto* colGroup = new QGroupBox(tr("Columns"), this);
    auto* colLayout = new QVBoxLayout(colGroup);
    m_columnList = new QListWidget(this);
    for (const auto& col : m_schema) {
        m_columnList->addItem(col.name);
    }
    colLayout->addWidget(m_columnList);
    layout->addWidget(colGroup);

    connect(m_columnList, &QListWidget::itemDoubleClicked,
            this, &CalculatedColumnDialog::onColumnDoubleClicked);

    // Function list
    auto* funcGroup = new QGroupBox(tr("Functions"), this);
    auto* funcLayout = new QVBoxLayout(funcGroup);
    m_functionList = new QListWidget(this);
    for (const auto& fn : FormulaEngine::availableFunctions()) {
        m_functionList->addItem(fn);
    }
    funcLayout->addWidget(m_functionList);

    m_helpLabel = new QLabel(this);
    m_helpLabel->setWordWrap(true);
    m_helpLabel->setStyleSheet(QStringLiteral("color: #666; font-size: 11px;"));
    funcLayout->addWidget(m_helpLabel);
    layout->addWidget(funcGroup);

    connect(m_functionList, &QListWidget::currentTextChanged,
            this, &CalculatedColumnDialog::onFunctionSelected);
    connect(m_functionList, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem* item) {
                insertAtCursor(item->text() + QStringLiteral("()"));
            });

    return group;
}

QWidget* CalculatedColumnDialog::buildPreviewSection()
{
    auto* group = new QGroupBox(tr("Preview"), this);
    auto* layout = new QVBoxLayout(group);

    m_previewTable = new QTableWidget(this);
    m_previewTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_previewTable->setAlternatingRowColors(true);
    m_previewTable->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_previewTable);

    return group;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void CalculatedColumnDialog::onValidate()
{
    const QString formula = m_formulaEdit->toPlainText().trimmed();
    if (formula.isEmpty()) {
        showValidationResult(false, tr("Formula is empty"));
        return;
    }

    if (m_columnNameEdit->text().trimmed().isEmpty()) {
        showValidationResult(false, tr("Column name is required"));
        return;
    }

    std::vector<FormulaError> errors;
    auto ast = m_formulaEngine.parseAndValidate(formula, errors);

    if (ast && errors.empty()) {
        auto outputType = m_formulaEngine.inferType(ast);
        m_outputTypeLabel->setText(
            QStringLiteral("Output type: %1")
                .arg(formulaOutputTypeToString(outputType)));
        showValidationResult(true, tr("✓ Formula is valid"));
    } else {
        QStringList msgs;
        for (const auto& err : errors) {
            msgs << err.message;
        }
        showValidationResult(false, msgs.join(QStringLiteral("; ")));
    }
}

void CalculatedColumnDialog::onPreview()
{
    if (!m_engine || !m_engine->isInitialized()) return;

    const QString formula = m_formulaEdit->toPlainText().trimmed();
    FormulaSQLTranslator translator;
    const QString sqlExpr = translator.translateFormula(formula);
    if (sqlExpr.isEmpty()) {
        showValidationResult(false, tr("Cannot translate formula to SQL"));
        return;
    }

    const QString colName = m_columnNameEdit->text().trimmed().isEmpty()
        ? QStringLiteral("result")
        : m_columnNameEdit->text().trimmed();

    const QString sql = QStringLiteral("SELECT *, %1 AS %2 FROM %3 LIMIT 25")
        .arg(sqlExpr, quoteName(colName), quoteName(m_tableName));

    auto qr = m_engine->executeQuery(sql);
    if (!qr.success) {
        showValidationResult(false, qr.error);
        return;
    }

    // Populate preview table
    m_previewTable->clear();
    const int colCount = static_cast<int>(qr.columnNames.size());
    const int rowCount = static_cast<int>(qr.rows.size());
    m_previewTable->setColumnCount(colCount);
    m_previewTable->setRowCount(rowCount);

    QStringList headers;
    headers.reserve(colCount);
    for (const auto& name : qr.columnNames) headers << name;
    m_previewTable->setHorizontalHeaderLabels(headers);

    for (int r = 0; r < rowCount; ++r) {
        for (int c = 0; c < colCount && c < static_cast<int>(qr.rows[static_cast<size_t>(r)].size()); ++c) {
            m_previewTable->setItem(r, c,
                new QTableWidgetItem(
                    qr.rows[static_cast<size_t>(r)][static_cast<size_t>(c)].toString()));
        }
    }

    showValidationResult(true, QStringLiteral("Preview: %1 rows").arg(rowCount));
}

void CalculatedColumnDialog::onFormulaChanged()
{
    m_validationLabel->clear();
    m_outputTypeLabel->setText(QStringLiteral("Output type: -"));
}

void CalculatedColumnDialog::onFunctionSelected(const QString& funcName)
{
    m_helpLabel->setText(FormulaEngine::functionHelp(funcName));
}

void CalculatedColumnDialog::onColumnDoubleClicked(QListWidgetItem* item)
{
    insertAtCursor(QStringLiteral("[%1]").arg(item->text()));
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void CalculatedColumnDialog::insertAtCursor(const QString& text)
{
    m_formulaEdit->insertPlainText(text);
    m_formulaEdit->setFocus();
}

void CalculatedColumnDialog::showValidationResult(bool valid, const QString& message)
{
    m_validationLabel->setText(message);
    m_validationLabel->setStyleSheet(
        valid ? QStringLiteral("color: green;") : QStringLiteral("color: red;"));
}

} // namespace csvforge
