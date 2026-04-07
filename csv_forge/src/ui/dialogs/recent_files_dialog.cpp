#include "ui/dialogs/recent_files_dialog.h"

#include "utils/file_utils.h"

#include <QDateTime>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>

namespace csvforge {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

RecentFilesDialog::RecentFilesDialog(const QStringList& recentFiles,
                                     QWidget* parent)
    : QDialog(parent)
    , m_recentFiles(recentFiles)
{
    setWindowTitle(QStringLiteral("Recent Files"));
    resize(700, 400);
    buildUI();
    populateTable();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

QString RecentFilesDialog::selectedFile() const
{
    return m_selectedFile;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void RecentFilesDialog::onOpen()
{
    const int row = m_table->currentRow();
    if (row >= 0) {
        m_selectedFile = m_table->item(row, 1)->text();
        accept();
    }
}

void RecentFilesDialog::onRemove()
{
    const int row = m_table->currentRow();
    if (row >= 0 && row < m_recentFiles.size()) {
        m_recentFiles.removeAt(row);
        populateTable();
        emit filesChanged(m_recentFiles);
    }
}

void RecentFilesDialog::onClearAll()
{
    m_recentFiles.clear();
    populateTable();
    emit filesChanged(m_recentFiles);
}

void RecentFilesDialog::onRowDoubleClicked(int row, int /*column*/)
{
    if (row >= 0) {
        m_selectedFile = m_table->item(row, 1)->text();
        accept();
    }
}

// ---------------------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------------------

void RecentFilesDialog::buildUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels(
        {QStringLiteral("File Name"), QStringLiteral("File Path"),
         QStringLiteral("Size"), QStringLiteral("Last Modified")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->setVisible(false);
    connect(m_table, &QTableWidget::cellDoubleClicked,
            this, &RecentFilesDialog::onRowDoubleClicked);
    mainLayout->addWidget(m_table, /*stretch=*/1);

    // Buttons
    auto* btnLayout = new QHBoxLayout;
    m_openBtn     = new QPushButton(QStringLiteral("Open"), this);
    m_removeBtn   = new QPushButton(QStringLiteral("Remove"), this);
    m_clearAllBtn = new QPushButton(QStringLiteral("Clear All"), this);

    connect(m_openBtn,     &QPushButton::clicked, this, &RecentFilesDialog::onOpen);
    connect(m_removeBtn,   &QPushButton::clicked, this, &RecentFilesDialog::onRemove);
    connect(m_clearAllBtn, &QPushButton::clicked, this, &RecentFilesDialog::onClearAll);

    btnLayout->addWidget(m_openBtn);
    btnLayout->addWidget(m_removeBtn);
    btnLayout->addWidget(m_clearAllBtn);
    btnLayout->addStretch();

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    btnLayout->addWidget(m_buttonBox);

    mainLayout->addLayout(btnLayout);
}

void RecentFilesDialog::populateTable()
{
    m_table->setRowCount(0);
    m_table->setRowCount(m_recentFiles.size());

    for (int i = 0; i < m_recentFiles.size(); ++i) {
        const QFileInfo fi(m_recentFiles[i]);

        m_table->setItem(i, 0, new QTableWidgetItem(fi.fileName()));
        m_table->setItem(i, 1, new QTableWidgetItem(fi.absoluteFilePath()));

        const qint64 bytes = fileSize(fi.absoluteFilePath());
        m_table->setItem(i, 2, new QTableWidgetItem(
            bytes >= 0 ? fileSizeFormatted(bytes) : QStringLiteral("N/A")));

        m_table->setItem(i, 3, new QTableWidgetItem(
            fi.lastModified().toString(Qt::DefaultLocaleShortDate)));
    }

    m_table->resizeColumnsToContents();
}

} // namespace csvforge
