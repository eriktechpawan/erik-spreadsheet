#include "ui/widgets/result_tab_widget.h"
#include "app/tab_manager.h"

#include <QMenu>
#include <QTabBar>
#include <QWidget>

namespace csvforge {

ResultTabWidget::ResultTabWidget(QWidget* parent)
    : QTabWidget(parent)
{
    setTabsClosable(true);
    setMovable(false);
    setDocumentMode(true);

    connect(this, &QTabWidget::currentChanged, this, [this](int index) {
        if (!m_syncing && m_manager) {
            m_manager->setActiveTab(index);
        }
    });

    connect(this, &QTabWidget::tabCloseRequested, this, [this](int index) {
        emit closeRequested(index);
    });
}

void ResultTabWidget::setTabManager(TabManager* mgr)
{
    if (m_manager) {
        disconnect(m_manager, nullptr, this, nullptr);
    }
    m_manager = mgr;
    if (!m_manager) return;

    connect(m_manager, &TabManager::tabCreated,
            this, &ResultTabWidget::onTabCreated);
    connect(m_manager, &TabManager::tabClosed,
            this, &ResultTabWidget::onTabClosed);
    connect(m_manager, &TabManager::tabRenamed,
            this, &ResultTabWidget::onTabRenamed);
    connect(m_manager, &TabManager::activeTabChanged,
            this, &ResultTabWidget::onActiveTabChanged);
    connect(m_manager, &TabManager::tabDirtyChanged,
            this, &ResultTabWidget::onTabDirtyChanged);

    syncFromManager();
}

void ResultTabWidget::syncFromManager()
{
    if (!m_manager) return;
    m_syncing = true;

    // Remove excess tabs
    while (count() > m_manager->tabCount()) {
        QWidget* page = widget(count() - 1);
        removeTab(count() - 1);
        delete page;
    }
    // Add missing tabs
    while (count() < m_manager->tabCount()) {
        addTab(new QWidget(this), QString());
    }

    // Update labels
    for (int i = 0; i < m_manager->tabCount(); ++i) {
        updateTabLabel(i);
    }

    if (m_manager->activeTabIndex() >= 0) {
        setCurrentIndex(m_manager->activeTabIndex());
    }

    m_syncing = false;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void ResultTabWidget::onTabCreated(int /*index*/)
{
    syncFromManager();
}

void ResultTabWidget::onTabClosed(int /*index*/)
{
    syncFromManager();
}

void ResultTabWidget::onTabRenamed(int index, const QString& /*name*/)
{
    updateTabLabel(index);
}

void ResultTabWidget::onActiveTabChanged(int index)
{
    if (m_syncing) return;
    m_syncing = true;
    if (index >= 0 && index < count()) {
        setCurrentIndex(index);
    }
    m_syncing = false;
}

void ResultTabWidget::onTabDirtyChanged(int index, bool /*dirty*/)
{
    updateTabLabel(index);
}

// ---------------------------------------------------------------------------
// Context menu
// ---------------------------------------------------------------------------

void ResultTabWidget::contextMenuEvent(QContextMenuEvent* event)
{
    const int index = tabBar()->tabAt(tabBar()->mapFrom(this, event->pos()));
    if (index < 0) return;

    QMenu menu(this);
    auto* renameAct = menu.addAction(tr("Rename Tab…"));
    auto* duplicateAct = menu.addAction(tr("Duplicate Tab"));
    menu.addSeparator();
    auto* exportAct = menu.addAction(tr("Export Tab…"));
    auto* lineageAct = menu.addAction(tr("Show Lineage…"));
    menu.addSeparator();
    auto* closeAct = menu.addAction(tr("Close Tab"));

    QAction* chosen = menu.exec(event->globalPos());
    if (chosen == renameAct)    emit renameRequested(index);
    else if (chosen == duplicateAct) emit duplicateRequested(index);
    else if (chosen == exportAct)    emit exportRequested(index);
    else if (chosen == lineageAct)   emit showLineageRequested(index);
    else if (chosen == closeAct)     emit closeRequested(index);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void ResultTabWidget::updateTabLabel(int index)
{
    if (!m_manager || index < 0 || index >= m_manager->tabCount()) return;

    const auto* tab = m_manager->tabAt(index);
    if (!tab) return;

    const auto& desc = tab->descriptor();
    QString label = desc.displayName;

    // Badge
    if (desc.type != DatasetType::Source) {
        label = QStringLiteral("[%1] %2").arg(desc.typeBadge(), label);
    }

    // Dirty indicator
    if (tab->isDirty()) {
        label += QStringLiteral(" ●");
    }

    setTabText(index, label);
    setTabToolTip(index, desc.tooltipText());
}

} // namespace csvforge
