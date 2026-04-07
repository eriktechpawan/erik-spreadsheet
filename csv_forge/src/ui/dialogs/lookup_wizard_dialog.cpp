#include "ui/dialogs/lookup_wizard_dialog.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

namespace csvforge {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

LookupWizardDialog::LookupWizardDialog(const QString& sourceTable,
                                       const ColumnSchemaList& sourceSchema,
                                       DuckDBEngine* engine,
                                       QWidget* parent)
    : QDialog(parent)
    , m_sourceTable(sourceTable)
    , m_sourceSchema(sourceSchema)
    , m_engine(engine)
{
    setWindowTitle(QStringLiteral("Lookup Wizard"));
    resize(600, 700);
    buildUI();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

LookupConfig LookupWizardDialog::getConfig() const
{
    LookupConfig cfg;
    cfg.sourceTable     = m_sourceTable;
    cfg.sourceKeyColumn = m_sourceKeyCombo->currentText();

    cfg.targetFile  = m_targetFileEdit->text();
    cfg.targetTable = m_targetTableCombo->currentText();
    cfg.targetKeyColumn = m_targetKeyCombo->currentText();

    for (int i = 0; i < m_returnColumnsList->count(); ++i) {
        auto* item = m_returnColumnsList->item(i);
        if (item->checkState() == Qt::Checked)
            cfg.returnColumns.append(item->text());
    }

    cfg.type = (m_matchTypeCombo->currentIndex() == 0)
                   ? LookupType::ExactMatch
                   : LookupType::LeftJoin;

    cfg.outputTarget = (m_outputCombo->currentIndex() == 0)
                           ? LookupOutputTarget::AppendColumns
                           : LookupOutputTarget::NewResultTable;

    cfg.resultTableName = m_resultNameEdit->text();

    return cfg;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void LookupWizardDialog::onBrowseTarget()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Select Target File"), {},
        QStringLiteral("CSV Files (*.csv *.tsv *.txt);;All Files (*)"));
    if (!path.isEmpty()) {
        m_targetFileEdit->setText(path);
        onTargetFileChanged();
    }
}

void LookupWizardDialog::onTargetFileChanged()
{
    const QString path = m_targetFileEdit->text();
    if (path.isEmpty())
        return;

    // Load the target file temporarily to detect columns
    const QString tempTable = QStringLiteral("__lookup_preview__");
    CSVImportOptions opts;
    opts.autoDetect = true;
    const QString err = m_engine->loadCSVAsTable(path, tempTable, opts);
    if (!err.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Load Error"), err);
        return;
    }

    const ColumnSchemaList targetSchema = m_engine->getTableSchema(tempTable);
    populateTargetColumns(targetSchema);
    m_engine->dropTable(tempTable);
}

void LookupWizardDialog::onTargetTableChanged(int /*index*/)
{
    const QString table = m_targetTableCombo->currentText();
    if (table.isEmpty())
        return;
    const ColumnSchemaList schema = m_engine->getTableSchema(table);
    populateTargetColumns(schema);
}

void LookupWizardDialog::onSelectAll()
{
    for (int i = 0; i < m_returnColumnsList->count(); ++i)
        m_returnColumnsList->item(i)->setCheckState(Qt::Checked);
}

void LookupWizardDialog::onDeselectAll()
{
    for (int i = 0; i < m_returnColumnsList->count(); ++i)
        m_returnColumnsList->item(i)->setCheckState(Qt::Unchecked);
}

void LookupWizardDialog::onOutputTargetChanged(int index)
{
    m_resultNameEdit->setEnabled(index == 1); // NewResultTable
}

// ---------------------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------------------

void LookupWizardDialog::buildUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(buildSourceSection());
    mainLayout->addWidget(buildTargetSection());
    mainLayout->addWidget(buildReturnColumnsSection(), /*stretch=*/1);
    mainLayout->addWidget(buildOptionsSection());

    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(m_buttonBox);
}

QGroupBox* LookupWizardDialog::buildSourceSection()
{
    auto* group  = new QGroupBox(QStringLiteral("Source"), this);
    auto* layout = new QFormLayout(group);

    m_sourceTableLabel = new QLabel(m_sourceTable, group);
    layout->addRow(QStringLiteral("Source Table:"), m_sourceTableLabel);

    m_sourceKeyCombo = new QComboBox(group);
    for (const auto& col : m_sourceSchema)
        m_sourceKeyCombo->addItem(col.name);
    layout->addRow(QStringLiteral("Key Column:"), m_sourceKeyCombo);

    return group;
}

QGroupBox* LookupWizardDialog::buildTargetSection()
{
    auto* group  = new QGroupBox(QStringLiteral("Target"), this);
    auto* layout = new QFormLayout(group);

    // Target file
    auto* fileRow = new QHBoxLayout;
    m_targetFileEdit = new QLineEdit(group);
    m_targetFileEdit->setPlaceholderText(QStringLiteral("Select CSV file…"));
    m_browseTargetBtn = new QPushButton(QStringLiteral("Browse…"), group);
    connect(m_browseTargetBtn, &QPushButton::clicked,
            this, &LookupWizardDialog::onBrowseTarget);
    fileRow->addWidget(m_targetFileEdit, /*stretch=*/1);
    fileRow->addWidget(m_browseTargetBtn);
    layout->addRow(QStringLiteral("Target File:"), fileRow);

    // Target table (already loaded)
    m_targetTableCombo = new QComboBox(group);
    m_targetTableCombo->addItem(QString()); // empty = use file
    const QStringList tables = m_engine->listTables();
    for (const auto& t : tables)
        m_targetTableCombo->addItem(t);
    connect(m_targetTableCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LookupWizardDialog::onTargetTableChanged);
    layout->addRow(QStringLiteral("Target Table:"), m_targetTableCombo);

    // Target key column
    m_targetKeyCombo = new QComboBox(group);
    layout->addRow(QStringLiteral("Key Column:"), m_targetKeyCombo);

    return group;
}

QGroupBox* LookupWizardDialog::buildReturnColumnsSection()
{
    auto* group  = new QGroupBox(QStringLiteral("Return Columns"), this);
    auto* layout = new QVBoxLayout(group);

    m_returnColumnsList = new QListWidget(group);
    layout->addWidget(m_returnColumnsList, /*stretch=*/1);

    auto* btnRow = new QHBoxLayout;
    m_selectAllBtn   = new QPushButton(QStringLiteral("Select All"), group);
    m_deselectAllBtn = new QPushButton(QStringLiteral("Deselect All"), group);
    connect(m_selectAllBtn,   &QPushButton::clicked,
            this, &LookupWizardDialog::onSelectAll);
    connect(m_deselectAllBtn, &QPushButton::clicked,
            this, &LookupWizardDialog::onDeselectAll);
    btnRow->addWidget(m_selectAllBtn);
    btnRow->addWidget(m_deselectAllBtn);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    return group;
}

QGroupBox* LookupWizardDialog::buildOptionsSection()
{
    auto* group  = new QGroupBox(QStringLiteral("Options"), this);
    auto* layout = new QFormLayout(group);

    m_matchTypeCombo = new QComboBox(group);
    m_matchTypeCombo->addItem(QStringLiteral("Exact Match"));
    m_matchTypeCombo->addItem(QStringLiteral("Left Join"));
    layout->addRow(QStringLiteral("Match Type:"), m_matchTypeCombo);

    m_outputCombo = new QComboBox(group);
    m_outputCombo->addItem(QStringLiteral("Append Columns to Current Table"));
    m_outputCombo->addItem(QStringLiteral("Create New Result Table"));
    connect(m_outputCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LookupWizardDialog::onOutputTargetChanged);
    layout->addRow(QStringLiteral("Output:"), m_outputCombo);

    m_resultNameEdit = new QLineEdit(group);
    m_resultNameEdit->setPlaceholderText(QStringLiteral("result_table"));
    m_resultNameEdit->setEnabled(false);
    layout->addRow(QStringLiteral("Result Table Name:"), m_resultNameEdit);

    return group;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void LookupWizardDialog::populateTargetColumns(const ColumnSchemaList& schema)
{
    m_targetKeyCombo->clear();
    m_returnColumnsList->clear();

    for (const auto& col : schema) {
        m_targetKeyCombo->addItem(col.name);

        auto* item = new QListWidgetItem(col.name, m_returnColumnsList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
    }
}

} // namespace csvforge
