#include "ui/dialogs/import_dialog.h"

#include "utils/file_utils.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QTextStream>
#include <QVBoxLayout>

namespace csvforge {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

ImportDialog::ImportDialog(const QString& filePath, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Import CSV"));
    resize(700, 500);

    buildUI();

    if (!filePath.isEmpty()) {
        m_filePath->setText(filePath);
        const CSVFormat fmt = CSVSniffer::detect(filePath, PreviewRows);
        applyDetectedFormat(fmt);
        loadPreview();
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

CSVImportOptions ImportDialog::importOptions() const
{
    return buildOptionsFromUI();
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void ImportDialog::onBrowse()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Select CSV File"),
        {},
        QStringLiteral("CSV Files (*.csv *.tsv *.txt);;All Files (*)"));

    if (path.isEmpty()) {
        return;
    }

    m_filePath->setText(path);

    const CSVFormat fmt = CSVSniffer::detect(path, PreviewRows);
    applyDetectedFormat(fmt);
    loadPreview();
}

void ImportDialog::onRefreshPreview()
{
    loadPreview();
}

void ImportDialog::onDelimiterChanged(int index)
{
    // Last item in the combo is "Custom"
    const bool isCustom = (index == m_delimiterCombo->count() - 1);
    m_customDelimiter->setVisible(isCustom);
}

// ---------------------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------------------

void ImportDialog::buildUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(buildFileSection());
    mainLayout->addWidget(buildAutoDetectSection());
    mainLayout->addWidget(buildOverrideSection());
    mainLayout->addWidget(buildPreviewSection(), /*stretch=*/1);

    // ---- Buttons ----------------------------------------------------------
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(m_buttonBox);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QGroupBox* ImportDialog::buildFileSection()
{
    auto* group  = new QGroupBox(QStringLiteral("File"), this);
    auto* layout = new QHBoxLayout(group);

    m_filePath = new QLineEdit(group);
    m_filePath->setReadOnly(true);
    m_filePath->setPlaceholderText(QStringLiteral("Select a CSV file…"));

    m_browseBtn = new QPushButton(QStringLiteral("Browse…"), group);
    connect(m_browseBtn, &QPushButton::clicked, this, &ImportDialog::onBrowse);

    layout->addWidget(m_filePath, /*stretch=*/1);
    layout->addWidget(m_browseBtn);
    return group;
}

QGroupBox* ImportDialog::buildAutoDetectSection()
{
    auto* group  = new QGroupBox(QStringLiteral("Auto-Detected Format"), this);
    auto* layout = new QFormLayout(group);

    m_detDelimiter = new QLabel(QStringLiteral("—"), group);
    m_detQuote     = new QLabel(QStringLiteral("—"), group);
    m_detHeader    = new QLabel(QStringLiteral("—"), group);
    m_detEncoding  = new QLabel(QStringLiteral("—"), group);

    layout->addRow(QStringLiteral("Delimiter:"), m_detDelimiter);
    layout->addRow(QStringLiteral("Quote Char:"), m_detQuote);
    layout->addRow(QStringLiteral("Has Header:"), m_detHeader);
    layout->addRow(QStringLiteral("Encoding:"), m_detEncoding);

    return group;
}

QGroupBox* ImportDialog::buildOverrideSection()
{
    auto* group  = new QGroupBox(QStringLiteral("Manual Override"), this);
    auto* layout = new QFormLayout(group);

    // ---- Delimiter --------------------------------------------------------
    m_delimiterCombo = new QComboBox(group);
    m_delimiterCombo->addItem(QStringLiteral("Comma  (,)"),
                              QVariant(QLatin1Char(',')));
    m_delimiterCombo->addItem(QStringLiteral("Tab  (\\t)"),
                              QVariant(QLatin1Char('\t')));
    m_delimiterCombo->addItem(QStringLiteral("Semicolon  (;)"),
                              QVariant(QLatin1Char(';')));
    m_delimiterCombo->addItem(QStringLiteral("Pipe  (|)"),
                              QVariant(QLatin1Char('|')));
    m_delimiterCombo->addItem(QStringLiteral("Custom"));

    m_customDelimiter = new QLineEdit(group);
    m_customDelimiter->setMaxLength(1);
    m_customDelimiter->setFixedWidth(40);
    m_customDelimiter->setVisible(false);

    auto* delimRow = new QHBoxLayout;
    delimRow->addWidget(m_delimiterCombo);
    delimRow->addWidget(m_customDelimiter);
    delimRow->addStretch();
    layout->addRow(QStringLiteral("Delimiter:"), delimRow);

    connect(m_delimiterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ImportDialog::onDelimiterChanged);

    // ---- Quote character --------------------------------------------------
    m_quoteCombo = new QComboBox(group);
    m_quoteCombo->addItem(QStringLiteral("Double Quote (\")"),
                          QVariant(QLatin1Char('"')));
    m_quoteCombo->addItem(QStringLiteral("Single Quote (')"),
                          QVariant(QLatin1Char('\'')));
    m_quoteCombo->addItem(QStringLiteral("None"), QVariant(QChar()));
    layout->addRow(QStringLiteral("Quote Char:"), m_quoteCombo);

    // ---- Encoding ---------------------------------------------------------
    m_encodingCombo = new QComboBox(group);
    m_encodingCombo->addItem(QStringLiteral("UTF-8"));
    m_encodingCombo->addItem(QStringLiteral("Latin-1"));
    m_encodingCombo->addItem(QStringLiteral("ASCII"));
    layout->addRow(QStringLiteral("Encoding:"), m_encodingCombo);

    // ---- Has Header -------------------------------------------------------
    m_headerCheck = new QCheckBox(QStringLiteral("First row is header"), group);
    m_headerCheck->setChecked(true);
    layout->addRow(QStringLiteral("Header Row:"), m_headerCheck);

    // ---- Skip rows --------------------------------------------------------
    m_skipRowsSpin = new QSpinBox(group);
    m_skipRowsSpin->setRange(0, 1000);
    layout->addRow(QStringLiteral("Skip Rows:"), m_skipRowsSpin);

    // ---- Ignore errors ----------------------------------------------------
    m_ignoreErrorsCheck = new QCheckBox(
        QStringLiteral("Continue on parse errors"), group);
    layout->addRow(QStringLiteral("Ignore Errors:"), m_ignoreErrorsCheck);

    return group;
}

QWidget* ImportDialog::buildPreviewSection()
{
    auto* container = new QWidget(this);
    auto* layout    = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    // ---- Header row with label + refresh button ---------------------------
    auto* headerRow = new QHBoxLayout;
    headerRow->addWidget(new QLabel(QStringLiteral("Preview (first %1 rows):")
                                        .arg(PreviewRows),
                                    container));
    headerRow->addStretch();

    m_refreshBtn = new QPushButton(QStringLiteral("Refresh Preview"), container);
    connect(m_refreshBtn, &QPushButton::clicked,
            this, &ImportDialog::onRefreshPreview);
    headerRow->addWidget(m_refreshBtn);

    layout->addLayout(headerRow);

    // ---- Table widget -----------------------------------------------------
    m_previewTable = new QTableWidget(container);
    m_previewTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_previewTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_previewTable->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_previewTable);

    return container;
}

// ---------------------------------------------------------------------------
// Logic helpers
// ---------------------------------------------------------------------------

CSVImportOptions ImportDialog::buildOptionsFromUI() const
{
    CSVImportOptions opts;

    // ---- Delimiter --------------------------------------------------------
    const int delimIdx = m_delimiterCombo->currentIndex();
    if (delimIdx == m_delimiterCombo->count() - 1) {
        // Custom delimiter
        const QString text = m_customDelimiter->text();
        opts.delimiter = text.isEmpty() ? QLatin1Char(',') : text.at(0);
    } else {
        opts.delimiter = m_delimiterCombo->currentData().value<QChar>();
    }

    // ---- Quote character --------------------------------------------------
    opts.quoteChar = m_quoteCombo->currentData().value<QChar>();

    // ---- Encoding ---------------------------------------------------------
    opts.encoding = m_encodingCombo->currentText();

    // ---- Simple flags / values --------------------------------------------
    opts.hasHeader    = m_headerCheck->isChecked();
    opts.skipRows     = m_skipRowsSpin->value();
    opts.ignoreErrors = m_ignoreErrorsCheck->isChecked();
    opts.autoDetect   = false; // user supplied explicit options

    return opts;
}

void ImportDialog::loadPreview()
{
    const QString path = m_filePath->text();
    if (path.isEmpty()) {
        return;
    }

    const CSVImportOptions opts = buildOptionsFromUI();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("Preview Error"),
                             QStringLiteral("Cannot open file:\n%1").arg(path));
        return;
    }

    QTextStream stream(&file);

    // ---- Select codec -----------------------------------------------------
    if (opts.encoding.compare(QStringLiteral("Latin-1"),
                              Qt::CaseInsensitive) == 0) {
        stream.setEncoding(QStringConverter::Latin1);
    } else if (opts.encoding.compare(QStringLiteral("ASCII"),
                                     Qt::CaseInsensitive) == 0) {
        stream.setEncoding(QStringConverter::Latin1); // ASCII subset
    } else {
        stream.setEncoding(QStringConverter::Utf8);
    }

    // ---- Skip leading rows ------------------------------------------------
    for (int i = 0; i < opts.skipRows && !stream.atEnd(); ++i) {
        stream.readLine();
    }

    // ---- Read sample lines ------------------------------------------------
    QStringList lines;
    while (!stream.atEnd() && lines.size() < PreviewRows + (opts.hasHeader ? 1 : 0)) {
        const QString line = stream.readLine();
        if (!line.isEmpty()) {
            lines.append(line);
        }
    }

    if (lines.isEmpty()) {
        m_previewTable->clear();
        m_previewTable->setRowCount(0);
        m_previewTable->setColumnCount(0);
        return;
    }

    // ---- Minimal CSV field splitting (respects quoting) -------------------
    auto splitLine = [&opts](const QString& line) -> QStringList {
        QStringList fields;
        QString field;
        bool inQuotes = false;
        const QChar delim = opts.delimiter;
        const QChar quote = opts.quoteChar;
        const bool  hasQuote = !quote.isNull();

        for (int i = 0; i < line.size(); ++i) {
            const QChar ch = line.at(i);

            if (hasQuote && ch == quote) {
                if (inQuotes && i + 1 < line.size() && line.at(i + 1) == quote) {
                    field.append(quote); // escaped quote
                    ++i;
                } else {
                    inQuotes = !inQuotes;
                }
            } else if (ch == delim && !inQuotes) {
                fields.append(field);
                field.clear();
            } else {
                field.append(ch);
            }
        }
        fields.append(field); // last field
        return fields;
    };

    // ---- Determine columns ------------------------------------------------
    int startRow = 0;
    QStringList headers;
    if (opts.hasHeader && !lines.isEmpty()) {
        headers = splitLine(lines.at(0));
        startRow = 1;
    } else if (!lines.isEmpty()) {
        const int colCount = splitLine(lines.at(0)).size();
        headers.reserve(colCount);
        for (int c = 0; c < colCount; ++c) {
            headers.append(QStringLiteral("Column %1").arg(c + 1));
        }
    }

    const int dataRows = qMin(static_cast<int>(lines.size()) - startRow,
                              PreviewRows);

    m_previewTable->clear();
    m_previewTable->setColumnCount(headers.size());
    m_previewTable->setHorizontalHeaderLabels(headers);
    m_previewTable->setRowCount(dataRows);

    for (int r = 0; r < dataRows; ++r) {
        const QStringList fields = splitLine(lines.at(r + startRow));
        for (int c = 0; c < headers.size(); ++c) {
            const QString text = (c < fields.size()) ? fields.at(c) : QString();
            m_previewTable->setItem(r, c, new QTableWidgetItem(text));
        }
    }

    m_previewTable->resizeColumnsToContents();
}

void ImportDialog::applyDetectedFormat(const CSVFormat& fmt)
{
    // ---- Update auto-detect labels ----------------------------------------
    auto charLabel = [](QChar ch) -> QString {
        if (ch == QLatin1Char(','))  return QStringLiteral("Comma  (,)");
        if (ch == QLatin1Char('\t')) return QStringLiteral("Tab  (\\t)");
        if (ch == QLatin1Char(';'))  return QStringLiteral("Semicolon  (;)");
        if (ch == QLatin1Char('|'))  return QStringLiteral("Pipe  (|)");
        if (ch == QLatin1Char('"'))  return QStringLiteral("Double Quote (\")");
        if (ch == QLatin1Char('\'')) return QStringLiteral("Single Quote (')");
        if (ch.isNull())             return QStringLiteral("None");
        return QStringLiteral("'%1'").arg(ch);
    };

    m_detDelimiter->setText(charLabel(fmt.delimiter));
    m_detQuote->setText(charLabel(fmt.quoteChar));
    m_detHeader->setText(fmt.hasHeader ? QStringLiteral("Yes")
                                       : QStringLiteral("No"));
    m_detEncoding->setText(fmt.encoding);

    // ---- Sync combo boxes to detected values ------------------------------

    // Delimiter
    bool delimMatched = false;
    for (int i = 0; i < m_delimiterCombo->count() - 1; ++i) {
        if (m_delimiterCombo->itemData(i).value<QChar>() == fmt.delimiter) {
            m_delimiterCombo->setCurrentIndex(i);
            delimMatched = true;
            break;
        }
    }
    if (!delimMatched) {
        // Select "Custom" and fill in the character
        m_delimiterCombo->setCurrentIndex(m_delimiterCombo->count() - 1);
        m_customDelimiter->setText(QString(fmt.delimiter));
    }

    // Quote character
    for (int i = 0; i < m_quoteCombo->count(); ++i) {
        if (m_quoteCombo->itemData(i).value<QChar>() == fmt.quoteChar) {
            m_quoteCombo->setCurrentIndex(i);
            break;
        }
    }

    // Encoding
    const int encIdx = m_encodingCombo->findText(fmt.encoding,
                                                  Qt::MatchFixedString);
    if (encIdx >= 0) {
        m_encodingCombo->setCurrentIndex(encIdx);
    }

    // Header & skip rows
    m_headerCheck->setChecked(fmt.hasHeader);
    m_skipRowsSpin->setValue(fmt.skipRows);
}

} // namespace csvforge
