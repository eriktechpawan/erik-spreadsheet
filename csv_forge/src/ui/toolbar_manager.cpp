#include "toolbar_manager.h"

namespace csvforge {

ToolbarManager::ToolbarManager(QToolBar* toolbar, QObject* parent)
    : QObject(parent)
    , m_toolbar(toolbar)
{
    setupToolbar();
}

QAction* ToolbarManager::action(const QString& name) const
{
    return m_actions.value(name, nullptr);
}

void ToolbarManager::setupToolbar()
{
    m_toolbar->setMovable(false);
    m_toolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);

    // --- File group ---
    auto* open   = createAction(QStringLiteral("open"),   tr("Open"),
                                tr("Open CSV file"), QKeySequence::Open);
    auto* save   = createAction(QStringLiteral("save"),   tr("Save Session"),
                                tr("Save current session"), QKeySequence::Save);
    auto* import = createAction(QStringLiteral("import"), tr("Import"),
                                tr("Import file with options"));
    auto* exp    = createAction(QStringLiteral("export"), tr("Export"),
                                tr("Export data"), QKeySequence(tr("Ctrl+E")));

    connect(open,   &QAction::triggered, this, &ToolbarManager::openFile);
    connect(save,   &QAction::triggered, this, &ToolbarManager::saveSession);
    connect(import, &QAction::triggered, this, &ToolbarManager::importFile);
    connect(exp,    &QAction::triggered, this, &ToolbarManager::exportData);

    m_toolbar->addSeparator();

    // --- Edit group ---
    auto* undoAct = createAction(QStringLiteral("undo"), tr("Undo"),
                                 tr("Undo last edit"), QKeySequence::Undo);
    auto* redoAct = createAction(QStringLiteral("redo"), tr("Redo"),
                                 tr("Redo last undone edit"), QKeySequence::Redo);
    auto* editMode = createAction(QStringLiteral("editMode"), tr("Edit Mode"),
                                  tr("Toggle edit mode"));
    editMode->setCheckable(true);

    connect(undoAct,  &QAction::triggered, this, &ToolbarManager::undo);
    connect(redoAct,  &QAction::triggered, this, &ToolbarManager::redo);
    connect(editMode, &QAction::triggered, this, &ToolbarManager::toggleEditMode);

    m_toolbar->addSeparator();

    // --- View group ---
    auto* filterToggle  = createAction(QStringLiteral("filterPanel"),  tr("Filters"),
                                       tr("Toggle filter panel"));
    filterToggle->setCheckable(true);
    auto* profileToggle = createAction(QStringLiteral("profilePanel"), tr("Profile"),
                                       tr("Toggle column profile panel"));
    profileToggle->setCheckable(true);
    auto* refresh = createAction(QStringLiteral("refresh"), tr("Refresh"),
                                 tr("Refresh data"));

    connect(filterToggle,  &QAction::triggered, this, &ToolbarManager::toggleFilterPanel);
    connect(profileToggle, &QAction::triggered, this, &ToolbarManager::toggleProfilePanel);
    connect(refresh,       &QAction::triggered, this, &ToolbarManager::refreshData);

    m_toolbar->addSeparator();

    // --- Data group ---
    auto* stats  = createAction(QStringLiteral("columnStats"), tr("Column Stats"),
                                tr("Show column statistics"));
    auto* pivot  = createAction(QStringLiteral("pivotTable"),  tr("Pivot Table"),
                                tr("Create pivot table"));
    auto* lookup = createAction(QStringLiteral("lookupWizard"), tr("Lookup"),
                                tr("Launch lookup wizard"));

    connect(stats,  &QAction::triggered, this, &ToolbarManager::showColumnStats);
    connect(pivot,  &QAction::triggered, this, &ToolbarManager::showPivotTable);
    connect(lookup, &QAction::triggered, this, &ToolbarManager::showLookupWizard);

    auto* calcCol = createAction(QStringLiteral("calculatedColumn"), tr("Calc Column"),
                                  tr("Add a calculated column"));
    auto* lineage = createAction(QStringLiteral("queryLineage"), tr("Lineage"),
                                  tr("View query lineage"));

    connect(calcCol, &QAction::triggered, this, &ToolbarManager::showCalculatedColumn);
    connect(lineage, &QAction::triggered, this, &ToolbarManager::showQueryLineage);

    m_toolbar->addSeparator();

    // --- Settings ---
    auto* prefs = createAction(QStringLiteral("preferences"), tr("Preferences"),
                               tr("Application preferences"));
    connect(prefs, &QAction::triggered, this, &ToolbarManager::showPreferences);

    // Initial state – no file loaded
    setFileLoaded(false);
    setEditActionsEnabled(false);
}

QAction* ToolbarManager::createAction(const QString& name, const QString& text,
                                      const QString& tooltip, const QKeySequence& shortcut)
{
    auto* act = new QAction(text, this);
    act->setToolTip(tooltip);
    if (!shortcut.isEmpty()) {
        act->setShortcut(shortcut);
    }
    m_actions.insert(name, act);
    m_toolbar->addAction(act);
    return act;
}

void ToolbarManager::setEditActionsEnabled(bool enabled)
{
    if (auto* a = action(QStringLiteral("undo")))
        a->setEnabled(enabled);
    if (auto* a = action(QStringLiteral("redo")))
        a->setEnabled(enabled);
}

void ToolbarManager::setUndoEnabled(bool enabled)
{
    if (auto* a = action(QStringLiteral("undo")))
        a->setEnabled(enabled);
}

void ToolbarManager::setRedoEnabled(bool enabled)
{
    if (auto* a = action(QStringLiteral("redo")))
        a->setEnabled(enabled);
}

void ToolbarManager::setFileLoaded(bool loaded)
{
    const QStringList dataActions = {
        QStringLiteral("export"),
        QStringLiteral("columnStats"),
        QStringLiteral("pivotTable"),
        QStringLiteral("lookupWizard"),
        QStringLiteral("filterPanel"),
        QStringLiteral("profilePanel"),
        QStringLiteral("refresh"),
        QStringLiteral("editMode"),
    };
    for (const auto& name : dataActions) {
        if (auto* a = action(name))
            a->setEnabled(loaded);
    }
}

} // namespace csvforge
