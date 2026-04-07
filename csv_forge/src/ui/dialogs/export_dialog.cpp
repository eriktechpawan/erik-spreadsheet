#include "ui/dialogs/export_dialog.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace csvforge {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

ExportDialog::ExportDialog(const ColumnSchemaList& schema, QWidget* parent)
    : QDialog(parent)
    , m_schema(schema)
{
    setWindowTitle(QStringLiteral("Export Data"));
    resize(550, 500);
    buildUI();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

ExportService::ExportOptions ExportDialog::exportOptions() const
{
    ExportService::ExportOptions opts;
    opts.filePath      = m_filePath->text();
    opts.format        = m_formatCombo->currentData().toString();
    opts.delimiter     = m_delimiterCombo->currentData().value<QChar>();
    opts.includeHeader = m_headerCheck->isChecked();

    // Column selection
    if (m_allColsRadio->isChecked()) {
        // empty list means all
    } else if (m_visibleColsRadio->isChecked()) {
        for (const auto& col : m_schema) {
            if (col.visible) {
                opts.columns.append(col.name);
            }
        }
    } else {
        for (int i = 0; i < m_columnList->count(); ++i) {
            auto* item = m_columnList->item(i);
            if (item->checkState() == Qt::Checked) {
                opts.columns.append(item->text());
            }
        }
    }

    // Where clause based on scope
    if (m_filteredRowsRadio->isChecked()) {
        opts.whereClause = QStringLiteral("__filtered__");
    } else if (m_selectedRowsRadio->isChecked()) {
        opts.whereClause = QStringLiteral("__selected__");
    }

    return opts;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void ExportDialog::onBrowse()
{
    QString filter;
    const QString fmt = m_formatCombo->currentData().toString();
    if (fmt == QStringLiteral("parquet")) {
        filter = QStringLiteral("Parquet Files (*.parquet);;All Files (*)");
    } else if (fmt == QStringLiteral("tsv")) {
        filter = QStringLiteral("TSV Files (*.tsv);;All Files (*)");
    } else {
        filter = QStringLiteral("CSV Files (*.csv);;All Files (*)");
    }

    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Export To"), {}, filter);
    if (!path.isEmpty()) {
        m_filePath->setText(path);
    }
}

void ExportDialog::onFormatChanged(int /*index*/)
{
    const QString fmt = m_formatCombo->currentData().toString();
    const bool isDelimited = (fmt != QStringLiteral("parquet"));
    m_delimiterCombo->setEnabled(isDelimited);
}

void ExportDialog::onColumnModeChanged()
{
    const bool custom = m_customColsRadio->isChecked();
    m_columnList->setEnabled(custom);
    m_selectAllBtn->setEnabled(custom);
    m_selectNoneBtn->setEnabled(custom);
}

void ExportDialog::onSelectAll()
{
    for (int i = 0; i < m_columnList->count(); ++i) {
        m_columnList->item(i)->setCheckState(Qt::Checked);
    }
}

void ExportDialog::onSelectNone()
{
    for (int i = 0; i < m_columnList->count(); ++i) {
        m_columnList->item(i)->setCheckState(Qt::Unchecked);
    }
}

// ---------------------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------------------

void ExportDialog::buildUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(buildFileSection());
    mainLayout->addWidget(buildFormatSection());
    mainLayout->addWidget(buildScopeSection());
    mainLayout->addWidget(buildColumnSection(), /*stretch=*/1);

    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(m_buttonBox);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QGroupBox* ExportDialog::buildFileSection()
{
    auto* group  = new QGroupBox(QStringLiteral("Output File"), this);
    auto* layout = new QHBoxLayout(group);

    m_filePath = new QLineEdit(group);
    m_filePath->setPlaceholderText(QStringLiteral("Select output file…"));

    m_browseBtn = new QPushButton(QStringLiteral("Browse…"), group);
    connect(m_browseBtn, &QPushButton::clicked, this, &ExportDialog::onBrowse);

    layout->addWidget(m_filePath, /*stretch=*/1);
    layout->addWidget(m_browseBtn);
    return group;
}

QGroupBox* ExportDialog::buildFormatSection()
{
    auto* group  = new QGroupBox(QStringLiteral("Format"), this);
    auto* layout = new QFormLayout(group);

    m_formatCombo = new QComboBox(group);
    m_formatCombo->addItem(QStringLiteral("CSV"), QStringLiteral("csv"));
    m_formatCombo->addItem(QStringLiteral("TSV"), QStringLiteral("tsv"));
    m_formatCombo->addItem(QStringLiteral("Parquet"), QStringLiteral("parquet"));
    layout->addRow(QStringLiteral("Format:"), m_formatCombo);

    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ExportDialog::onFormatChanged);

    m_delimiterCombo = new QComboBox(group);
    m_delimiterCombo->addItem(QStringLiteral("Comma  (,)"),
                              QVariant(QLatin1Char(',')));
    m_delimiterCombo->addItem(QStringLiteral("Tab  (\\t)"),
                              QVariant(QLatin1Char('\t')));
    m_delimiterCombo->addItem(QStringLiteral("Semicolon  (;)"),
                              QVariant(QLatin1Char(';')));
    m_delimiterCombo->addItem(QStringLiteral("Pipe  (|)"),
                              QVariant(QLatin1Char('|')));
    layout->addRow(QStringLiteral("Delimiter:"), m_delimiterCombo);

    m_headerCheck = new QCheckBox(QStringLiteral("Include header row"), group);
    m_headerCheck->setChecked(true);
    layout->addRow(QStringLiteral("Header:"), m_headerCheck);

    return group;
}

QGroupBox* ExportDialog::buildScopeSection()
{
    auto* group  = new QGroupBox(QStringLiteral("Export Scope"), this);
    auto* layout = new QVBoxLayout(group);

    m_allRowsRadio      = new QRadioButton(QStringLiteral("All rows"), group);
    m_filteredRowsRadio = new QRadioButton(QStringLiteral("Filtered rows only"), group);
    m_selectedRowsRadio = new QRadioButton(QStringLiteral("Selected rows only"), group);
    m_allRowsRadio->setChecked(true);

    layout->addWidget(m_allRowsRadio);
    layout->addWidget(m_filteredRowsRadio);
    layout->addWidget(m_selectedRowsRadio);

    return group;
}

QGroupBox* ExportDialog::buildColumnSection()
{
    auto* group  = new QGroupBox(QStringLiteral("Column Selection"), this);
    auto* layout = new QVBoxLayout(group);

    m_allColsRadio     = new QRadioButton(QStringLiteral("All columns"), group);
    m_visibleColsRadio = new QRadioButton(QStringLiteral("Visible columns only"), group);
    m_customColsRadio  = new QRadioButton(QStringLiteral("Custom selection"), group);
    m_allColsRadio->setChecked(true);

    layout->addWidget(m_allColsRadio);
    layout->addWidget(m_visibleColsRadio);
    layout->addWidget(m_customColsRadio);

    connect(m_allColsRadio,     &QRadioButton::toggled,
            this, &ExportDialog::onColumnModeChanged);
    connect(m_visibleColsRadio, &QRadioButton::toggled,
            this, &ExportDialog::onColumnModeChanged);
    connect(m_customColsRadio,  &QRadioButton::toggled,
            this, &ExportDialog::onColumnModeChanged);

    // ---- Column list with checkboxes --------------------------------------
    m_columnList = new QListWidget(group);
    m_columnList->setEnabled(false);
    for (const auto& col : m_schema) {
        auto* item = new QListWidgetItem(col.name, m_columnList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
    }
    layout->addWidget(m_columnList, /*stretch=*/1);

    auto* btnRow = new QHBoxLayout;
    m_selectAllBtn  = new QPushButton(QStringLiteral("Select All"), group);
    m_selectNoneBtn = new QPushButton(QStringLiteral("Select None"), group);
    m_selectAllBtn->setEnabled(false);
    m_selectNoneBtn->setEnabled(false);
    btnRow->addWidget(m_selectAllBtn);
    btnRow->addWidget(m_selectNoneBtn);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    connect(m_selectAllBtn,  &QPushButton::clicked,
            this, &ExportDialog::onSelectAll);
    connect(m_selectNoneBtn, &QPushButton::clicked,
            this, &ExportDialog::onSelectNone);

    return group;
}

} // namespace csvforge
