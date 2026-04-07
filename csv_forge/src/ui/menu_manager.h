#pragma once

#include <QAction>
#include <QMap>
#include <QMenu>
#include <QMenuBar>
#include <QObject>
#include <QStringList>

namespace csvforge {

class MenuManager : public QObject {
    Q_OBJECT
public:
    explicit MenuManager(QMenuBar* menuBar, QObject* parent = nullptr);

    QAction* action(const QString& name) const;
    QMenu* menu(const QString& name) const;

    void setEditActionsEnabled(bool enabled);
    void setUndoEnabled(bool enabled);
    void setRedoEnabled(bool enabled);
    void setFileLoaded(bool loaded);
    void updateRecentFiles(const QStringList& files);

signals:
    // File
    void openFile();
    void openRecentFile(const QString& path);
    void saveSession();
    void saveSessionAs();
    void loadSession();
    void importFile();
    void exportData();
    void showRecentFiles();
    void closeFile();

    // Edit
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void selectAll();
    void deleteSelection();
    void addRow();
    void addColumn();
    void duplicateRows();
    void fillDown();
    void clearCells();
    void toggleEditMode();

    // View
    void toggleFilterPanel();
    void toggleProfilePanel();
    void toggleSearchBar();
    void toggleStatusBar();
    void showColumnManager();
    void zoomIn();
    void zoomOut();
    void resetZoom();

    // Data
    void applyFilter();
    void clearFilters();
    void showSortDialog();
    void showColumnStats();
    void showPivotTable();
    void showLookupWizard();
    void showCalculatedColumn();
    void showQueryLineage();
    void refreshData();

    // Tools
    void showPreferences();

    // Help
    void showAbout();

private:
    QMenuBar* m_menuBar;
    QMap<QString, QAction*> m_actions;
    QMap<QString, QMenu*> m_menus;
    QMenu* m_recentFilesMenu = nullptr;

    void setupMenus();
    void createFileMenu();
    void createEditMenu();
    void createViewMenu();
    void createDataMenu();
    void createToolsMenu();
    void createHelpMenu();

    QAction* createAction(const QString& name, const QString& text,
                          const QKeySequence& shortcut = {});
};

} // namespace csvforge
