#pragma once

#include <QAction>
#include <QMap>
#include <QObject>
#include <QToolBar>

namespace csvforge {

class ToolbarManager : public QObject {
    Q_OBJECT
public:
    explicit ToolbarManager(QToolBar* toolbar, QObject* parent = nullptr);

    QAction* action(const QString& name) const;

    void setEditActionsEnabled(bool enabled);
    void setUndoEnabled(bool enabled);
    void setRedoEnabled(bool enabled);
    void setFileLoaded(bool loaded);

signals:
    void openFile();
    void saveSession();
    void exportData();
    void importFile();
    void undo();
    void redo();
    void toggleFilterPanel();
    void toggleProfilePanel();
    void toggleEditMode();
    void showColumnStats();
    void showPivotTable();
    void showLookupWizard();
    void showPreferences();
    void refreshData();

private:
    QToolBar* m_toolbar;
    QMap<QString, QAction*> m_actions;

    void setupToolbar();
    QAction* createAction(const QString& name, const QString& text,
                          const QString& tooltip, const QKeySequence& shortcut = {});
};

} // namespace csvforge
