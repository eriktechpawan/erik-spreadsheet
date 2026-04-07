#pragma once

#include <QContextMenuEvent>
#include <QTabWidget>
#include <QWidget>

namespace csvforge {

class TabManager;

/// Tab widget that displays dataset tabs with context menu and badges.
class ResultTabWidget : public QTabWidget {
    Q_OBJECT
public:
    explicit ResultTabWidget(QWidget* parent = nullptr);

    void setTabManager(TabManager* mgr);

    /// Sync tab bar from TabManager state. Call after model changes.
    void syncFromManager();

signals:
    void renameRequested(int index);
    void duplicateRequested(int index);
    void closeRequested(int index);
    void exportRequested(int index);
    void showLineageRequested(int index);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void onTabCreated(int index);
    void onTabClosed(int index);
    void onTabRenamed(int index, const QString& name);
    void onActiveTabChanged(int index);
    void onTabDirtyChanged(int index, bool dirty);

private:
    TabManager* m_manager = nullptr;
    bool m_syncing = false; // prevent re-entrant signals

    void updateTabLabel(int index);
};

} // namespace csvforge
