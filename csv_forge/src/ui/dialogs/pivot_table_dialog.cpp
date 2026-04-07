#include "ui/dialogs/pivot_table_dialog.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>

namespace csvforge {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

PivotTableDialog::PivotTableDialog(const ColumnSchemaList& schema,
                                   DuckDBEngine* engine,
                                   QWidget* parent)
    : QDialog(parent)
    , m_schema(schema)
    , m_engine(engine)
{
    setWindowTitle(QStringLiteral("Pivot Table"));
    resize(800, 600);
    buildUI();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

PivotConfig PivotTableDialog::getConfig() const
{
    PivotConfig cfg;

    for (int i = 0; i < m_rowFieldsList->count(); ++i)
        cfg.rowFields.append(m_rowFieldsList->item(i)->text());

    for (int i = 0; i < m_columnFieldsList->count(); ++i)
        cfg.columnFields.append(m_columnFieldsList->item(i)->text());

    for (int i = 0; i < m_valueFieldsList->count(); ++i) {
        PivotValueField vf;
        vf.columnName  = m_valueFieldsList->item(i)->data(Qt::UserRole).toString();
        vf.aggregation = (i < m_valueAggregations.size())
                             ? m_valueAggregations[i]
                             : PivotAggregation::Sum;
        cfg.valueFields.push_back(vf);
    }

    const int idx = m_outputTargetCombo->currentIndex();
    cfg.outputTarget = static_cast<PivotOutputTarget>(idx);
    cfg.outputPath   = m_exportPathEdit->text();

    return cfg;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

QString PivotTableDialog::selectedAvailableField() const
{
    auto* item = m_availableList->currentItem();
    return item ? item->text() : QString();
}

void PivotTableDialog::onAddRowField()
{
    const QString field = selectedAvailableField();
    if (!field.isEmpty())
        m_rowFieldsList->addItem(field);
}

void PivotTableDialog::onRemoveRowField()
{
    delete m_rowFieldsList->takeItem(m_rowFieldsList->currentRow());
}

void PivotTableDialog::onAddColumnField()
{
    const QString field = selectedAvailableField();
    if (!field.isEmpty())
        m_columnFieldsList->addItem(field);
}

void PivotTableDialog::onRemoveColumnField()
{
    delete m_columnFieldsList->takeItem(m_columnFieldsList->currentRow());
}

void PivotTableDialog::onAddValueField()
{
    const QString field = selectedAvailableField();
    if (field.isEmpty())
        return;

    const QStringList aggNames = {
        QStringLiteral("Sum"), QStringLiteral("Count"),
        QStringLiteral("Average"), QStringLiteral("Min"),
        QStringLiteral("Max")};

    bool ok = false;
    const QString chosen = QInputDialog::getItem(
        this, QStringLiteral("Aggregation"),
        QStringLiteral("Select aggregation for \"%1\":").arg(field),
        aggNames, 0, false, &ok);

    if (!ok)
        return;

    auto agg = PivotAggregation::Sum;
    if (chosen == QStringLiteral("Count"))        agg = PivotAggregation::Count;
    else if (chosen == QStringLiteral("Average")) agg = PivotAggregation::Average;
    else if (chosen == QStringLiteral("Min"))     agg = PivotAggregation::Min;
    else if (chosen == QStringLiteral("Max"))     agg = PivotAggregation::Max;

    auto* item = new QListWidgetItem(
        QStringLiteral("%1 (%2)").arg(field, chosen), m_valueFieldsList);
    item->setData(Qt::UserRole, field);
    m_valueAggregations.append(agg);
}

void PivotTableDialog::onRemoveValueField()
{
    const int row = m_valueFieldsList->currentRow();
    if (row >= 0) {
        delete m_valueFieldsList->takeItem(row);
        if (row < m_valueAggregations.size())
            m_valueAggregations.remove(row);
    }
}

void PivotTableDialog::onOutputTargetChanged(int index)
{
    const bool exportFile = (index == static_cast<int>(PivotOutputTarget::ExportFile));
    m_exportPathEdit->setEnabled(exportFile);
    m_browseExportBtn->setEnabled(exportFile);
}

void PivotTableDialog::onBrowseExport()
{
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Export Pivot To"), {},
        QStringLiteral("CSV Files (*.csv);;All Files (*)"));
    if (!path.isEmpty())
        m_exportPathEdit->setText(path);
}

void PivotTableDialog::onPreview()
{
    PivotConfig cfg = getConfig();
    if (cfg.valueFields.empty() || cfg.rowFields.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Preview"),
                             QStringLiteral("Add at least one row field and one value field."));
        return;
    }

    // Build a simple pivot preview query
    QStringList selectParts;
    for (const auto& r : cfg.rowFields)
        selectParts << r;
    for (const auto& vf : cfg.valueFields)
        selectParts << vf.toSQL();

    QStringList groupByParts = cfg.rowFields;
    QString sql = QStringLiteral("SELECT %1 FROM %2 GROUP BY %3 LIMIT 10")
                      .arg(selectParts.join(QStringLiteral(", ")),
                           cfg.sourceTable.isEmpty()
                               ? QStringLiteral("current_table")
                               : cfg.sourceTable,
                           groupByParts.join(QStringLiteral(", ")));

    auto result = m_engine->executeQuery(sql);
    if (!result.success) {
        QMessageBox::warning(this, QStringLiteral("Preview Error"), result.errorMessage);
        return;
    }

    // Populate preview table
    const int cols = result.columnNames.size();
    const int rows = result.rows.size();
    m_previewTable->setColumnCount(cols);
    m_previewTable->setRowCount(rows);
    m_previewTable->setHorizontalHeaderLabels(result.columnNames);

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            m_previewTable->setItem(
                r, c, new QTableWidgetItem(result.rows[r][c].toString()));
        }
    }
    m_previewTable->resizeColumnsToContents();
}

// ---------------------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------------------

void PivotTableDialog::buildUI()
{
    auto* mainLayout = new QHBoxLayout(this);

    // Left: available fields
    auto* leftPanel = new QVBoxLayout;
    leftPanel->addWidget(new QLabel(QStringLiteral("Available Fields"), this));
    m_availableList = new QListWidget(this);
    for (const auto& col : m_schema)
        m_availableList->addItem(col.name);
    leftPanel->addWidget(m_availableList, /*stretch=*/1);
    mainLayout->addLayout(leftPanel);

    // Right: field groups + output
    auto* rightPanel = new QVBoxLayout;
    rightPanel->addWidget(buildRowFieldsGroup());
    rightPanel->addWidget(buildColumnFieldsGroup());
    rightPanel->addWidget(buildValueFieldsGroup());

    // Output section
    auto* outputLayout = new QHBoxLayout;
    outputLayout->addWidget(new QLabel(QStringLiteral("Output:"), this));
    m_outputTargetCombo = new QComboBox(this);
    m_outputTargetCombo->addItem(QStringLiteral("New Tab"));
    m_outputTargetCombo->addItem(QStringLiteral("Temporary Table"));
    m_outputTargetCombo->addItem(QStringLiteral("Export File"));
    connect(m_outputTargetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PivotTableDialog::onOutputTargetChanged);
    outputLayout->addWidget(m_outputTargetCombo);

    m_exportPathEdit = new QLineEdit(this);
    m_exportPathEdit->setPlaceholderText(QStringLiteral("Export path…"));
    m_exportPathEdit->setEnabled(false);
    outputLayout->addWidget(m_exportPathEdit);

    m_browseExportBtn = new QPushButton(QStringLiteral("Browse…"), this);
    m_browseExportBtn->setEnabled(false);
    connect(m_browseExportBtn, &QPushButton::clicked,
            this, &PivotTableDialog::onBrowseExport);
    outputLayout->addWidget(m_browseExportBtn);
    rightPanel->addLayout(outputLayout);

    // Preview
    m_previewBtn = new QPushButton(QStringLiteral("Preview"), this);
    connect(m_previewBtn, &QPushButton::clicked,
            this, &PivotTableDialog::onPreview);
    rightPanel->addWidget(m_previewBtn);

    m_previewTable = new QTableWidget(this);
    m_previewTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_previewTable->horizontalHeader()->setStretchLastSection(true);
    m_previewTable->setMaximumHeight(200);
    rightPanel->addWidget(m_previewTable);

    // Buttons
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    rightPanel->addWidget(m_buttonBox);

    mainLayout->addLayout(rightPanel, /*stretch=*/1);
}

QGroupBox* PivotTableDialog::buildRowFieldsGroup()
{
    auto* group  = new QGroupBox(QStringLiteral("Row Fields"), this);
    auto* layout = new QHBoxLayout(group);

    m_rowFieldsList = new QListWidget(group);
    layout->addWidget(m_rowFieldsList, /*stretch=*/1);

    auto* btnLayout = new QVBoxLayout;
    m_addRowBtn = new QPushButton(QStringLiteral("Add →"), group);
    m_removeRowBtn = new QPushButton(QStringLiteral("← Remove"), group);
    connect(m_addRowBtn, &QPushButton::clicked,
            this, &PivotTableDialog::onAddRowField);
    connect(m_removeRowBtn, &QPushButton::clicked,
            this, &PivotTableDialog::onRemoveRowField);
    btnLayout->addWidget(m_addRowBtn);
    btnLayout->addWidget(m_removeRowBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    return group;
}

QGroupBox* PivotTableDialog::buildColumnFieldsGroup()
{
    auto* group  = new QGroupBox(QStringLiteral("Column Fields"), this);
    auto* layout = new QHBoxLayout(group);

    m_columnFieldsList = new QListWidget(group);
    layout->addWidget(m_columnFieldsList, /*stretch=*/1);

    auto* btnLayout = new QVBoxLayout;
    m_addColBtn = new QPushButton(QStringLiteral("Add →"), group);
    m_removeColBtn = new QPushButton(QStringLiteral("← Remove"), group);
    connect(m_addColBtn, &QPushButton::clicked,
            this, &PivotTableDialog::onAddColumnField);
    connect(m_removeColBtn, &QPushButton::clicked,
            this, &PivotTableDialog::onRemoveColumnField);
    btnLayout->addWidget(m_addColBtn);
    btnLayout->addWidget(m_removeColBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    return group;
}

QGroupBox* PivotTableDialog::buildValueFieldsGroup()
{
    auto* group  = new QGroupBox(QStringLiteral("Value Fields"), this);
    auto* layout = new QHBoxLayout(group);

    m_valueFieldsList = new QListWidget(group);
    layout->addWidget(m_valueFieldsList, /*stretch=*/1);

    auto* btnLayout = new QVBoxLayout;
    m_addValBtn = new QPushButton(QStringLiteral("Add →"), group);
    m_removeValBtn = new QPushButton(QStringLiteral("← Remove"), group);
    connect(m_addValBtn, &QPushButton::clicked,
            this, &PivotTableDialog::onAddValueField);
    connect(m_removeValBtn, &QPushButton::clicked,
            this, &PivotTableDialog::onRemoveValueField);
    btnLayout->addWidget(m_addValBtn);
    btnLayout->addWidget(m_removeValBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    return group;
}

} // namespace csvforge
