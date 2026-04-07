#include "status_bar_manager.h"

#include "utils/string_utils.h"

#include <QFrame>

namespace csvforge {

StatusBarManager::StatusBarManager(QStatusBar* statusBar, QObject* parent)
    : QObject(parent)
    , m_statusBar(statusBar)
{
    setupStatusBar();
}

void StatusBarManager::setupStatusBar()
{
    m_fileLabel         = createStatusLabel(tr("No file"));
    m_totalRowsLabel    = createStatusLabel(tr("Rows: 0"));
    m_filteredRowsLabel = createStatusLabel(tr("Filtered: –"));
    m_selectedRowsLabel = createStatusLabel(tr("Selected: 0"));
    m_visibleColsLabel  = createStatusLabel(tr("Cols: 0/0"));
    m_modeLabel         = createStatusLabel(tr("Read-Only"));
    m_sortLabel         = createStatusLabel(tr("Unsorted"));
    m_dirtyLabel        = createStatusLabel(QString());

    m_dirtyLabel->setMinimumWidth(20);

    m_statusBar->addPermanentWidget(m_fileLabel);
    m_statusBar->addPermanentWidget(m_totalRowsLabel);
    m_statusBar->addPermanentWidget(m_filteredRowsLabel);
    m_statusBar->addPermanentWidget(m_selectedRowsLabel);
    m_statusBar->addPermanentWidget(m_visibleColsLabel);
    m_statusBar->addPermanentWidget(m_modeLabel);
    m_statusBar->addPermanentWidget(m_sortLabel);
    m_statusBar->addPermanentWidget(m_dirtyLabel);
}

QLabel* StatusBarManager::createStatusLabel(const QString& text)
{
    auto* label = new QLabel(text, m_statusBar);
    label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    label->setMinimumWidth(80);
    label->setContentsMargins(4, 0, 4, 0);
    return label;
}

void StatusBarManager::setFileName(const QString& name)
{
    m_fileLabel->setText(name.isEmpty() ? tr("No file") : name);
}

void StatusBarManager::setTotalRows(qint64 count)
{
    m_totalRowsLabel->setText(tr("Rows: %1").arg(formatNumber(count)));
}

void StatusBarManager::setFilteredRows(qint64 count)
{
    m_filteredRowsLabel->setText(tr("Filtered: %1").arg(formatNumber(count)));
}

void StatusBarManager::setSelectedRows(int count)
{
    m_selectedRowsLabel->setText(tr("Selected: %1").arg(formatNumber(count)));
}

void StatusBarManager::setVisibleColumns(int count, int total)
{
    m_visibleColsLabel->setText(tr("Cols: %1/%2").arg(count).arg(total));
}

void StatusBarManager::setMode(bool editable)
{
    m_modeLabel->setText(editable ? tr("Editable") : tr("Read-Only"));
}

void StatusBarManager::setSortState(const QString& sortDescription)
{
    m_sortLabel->setText(sortDescription.isEmpty() ? tr("Unsorted") : sortDescription);
}

void StatusBarManager::setDirtyState(bool dirty)
{
    m_dirtyLabel->setText(dirty ? QStringLiteral("\u25CF") : QString());
}

void StatusBarManager::setMessage(const QString& message, int timeoutMs)
{
    m_statusBar->showMessage(message, timeoutMs);
}

void StatusBarManager::clear()
{
    m_fileLabel->setText(tr("No file"));
    m_totalRowsLabel->setText(tr("Rows: 0"));
    m_filteredRowsLabel->setText(tr("Filtered: –"));
    m_selectedRowsLabel->setText(tr("Selected: 0"));
    m_visibleColsLabel->setText(tr("Cols: 0/0"));
    m_modeLabel->setText(tr("Read-Only"));
    m_sortLabel->setText(tr("Unsorted"));
    m_dirtyLabel->setText(QString());
}

} // namespace csvforge
