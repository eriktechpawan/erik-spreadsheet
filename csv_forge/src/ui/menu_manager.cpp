#include "menu_manager.h"

#include <QFileInfo>

namespace csvforge {

MenuManager::MenuManager(QMenuBar* menuBar, QObject* parent)
    : QObject(parent)
    , m_menuBar(menuBar)
{
    setupMenus();
}

QAction* MenuManager::action(const QString& name) const
{
    return m_actions.value(name, nullptr);
}

QMenu* MenuManager::menu(const QString& name) const
{
    return m_menus.value(name, nullptr);
}

void MenuManager::setupMenus()
{
    createFileMenu();
    createEditMenu();
    createViewMenu();
    createDataMenu();
    createToolsMenu();
    createHelpMenu();
}

// ---------------------------------------------------------------------------
// File menu
// ---------------------------------------------------------------------------
void MenuManager::createFileMenu()
{
    auto* fileMenu = m_menuBar->addMenu(tr("&File"));
    m_menus.insert(QStringLiteral("file"), fileMenu);

    auto* open = createAction(QStringLiteral("open"), tr("&Open..."),
                              QKeySequence::Open);
    fileMenu->addAction(open);
    connect(open, &QAction::triggered, this, &MenuManager::openFile);

    // Recent files submenu
    m_recentFilesMenu = fileMenu->addMenu(tr("Open &Recent"));
    m_menus.insert(QStringLiteral("recentFiles"), m_recentFilesMenu);

    fileMenu->addSeparator();

    auto* save = createAction(QStringLiteral("saveSession"), tr("&Save Session"),
                              QKeySequence::Save);
    fileMenu->addAction(save);
    connect(save, &QAction::triggered, this, &MenuManager::saveSession);

    auto* saveAs = createAction(QStringLiteral("saveSessionAs"),
                                tr("Save Session &As..."),
                                QKeySequence(tr("Ctrl+Shift+S")));
    fileMenu->addAction(saveAs);
    connect(saveAs, &QAction::triggered, this, &MenuManager::saveSessionAs);

    auto* load = createAction(QStringLiteral("loadSession"), tr("&Load Session..."));
    fileMenu->addAction(load);
    connect(load, &QAction::triggered, this, &MenuManager::loadSession);

    fileMenu->addSeparator();

    auto* imp = createAction(QStringLiteral("import"), tr("&Import..."));
    fileMenu->addAction(imp);
    connect(imp, &QAction::triggered, this, &MenuManager::importFile);

    auto* exp = createAction(QStringLiteral("export"), tr("&Export..."),
                             QKeySequence(tr("Ctrl+E")));
    fileMenu->addAction(exp);
    connect(exp, &QAction::triggered, this, &MenuManager::exportData);

    fileMenu->addSeparator();

    auto* close = createAction(QStringLiteral("close"), tr("&Close"),
                               QKeySequence::Close);
    fileMenu->addAction(close);
    connect(close, &QAction::triggered, this, &MenuManager::closeFile);
}

// ---------------------------------------------------------------------------
// Edit menu
// ---------------------------------------------------------------------------
void MenuManager::createEditMenu()
{
    auto* editMenu = m_menuBar->addMenu(tr("&Edit"));
    m_menus.insert(QStringLiteral("edit"), editMenu);

    auto* undoAct = createAction(QStringLiteral("undo"), tr("&Undo"),
                                 QKeySequence::Undo);
    editMenu->addAction(undoAct);
    connect(undoAct, &QAction::triggered, this, &MenuManager::undo);

    auto* redoAct = createAction(QStringLiteral("redo"), tr("&Redo"),
                                 QKeySequence::Redo);
    editMenu->addAction(redoAct);
    connect(redoAct, &QAction::triggered, this, &MenuManager::redo);

    editMenu->addSeparator();

    auto* cutAct = createAction(QStringLiteral("cut"), tr("Cu&t"),
                                QKeySequence::Cut);
    editMenu->addAction(cutAct);
    connect(cutAct, &QAction::triggered, this, &MenuManager::cut);

    auto* copyAct = createAction(QStringLiteral("copy"), tr("&Copy"),
                                 QKeySequence::Copy);
    editMenu->addAction(copyAct);
    connect(copyAct, &QAction::triggered, this, &MenuManager::copy);

    auto* pasteAct = createAction(QStringLiteral("paste"), tr("&Paste"),
                                  QKeySequence::Paste);
    editMenu->addAction(pasteAct);
    connect(pasteAct, &QAction::triggered, this, &MenuManager::paste);

    editMenu->addSeparator();

    auto* selAll = createAction(QStringLiteral("selectAll"), tr("Select &All"),
                                QKeySequence::SelectAll);
    editMenu->addAction(selAll);
    connect(selAll, &QAction::triggered, this, &MenuManager::selectAll);

    auto* del = createAction(QStringLiteral("delete"), tr("&Delete"),
                             QKeySequence::Delete);
    editMenu->addAction(del);
    connect(del, &QAction::triggered, this, &MenuManager::deleteSelection);

    editMenu->addSeparator();

    auto* editModeAct = createAction(QStringLiteral("editMode"),
                                     tr("Toggle Edit &Mode"));
    editModeAct->setCheckable(true);
    editMenu->addAction(editModeAct);
    connect(editModeAct, &QAction::triggered, this, &MenuManager::toggleEditMode);

    editMenu->addSeparator();

    auto* addRowAct = createAction(QStringLiteral("addRow"), tr("Add Ro&w"));
    editMenu->addAction(addRowAct);
    connect(addRowAct, &QAction::triggered, this, &MenuManager::addRow);

    auto* addColAct = createAction(QStringLiteral("addColumn"), tr("Add Co&lumn"));
    editMenu->addAction(addColAct);
    connect(addColAct, &QAction::triggered, this, &MenuManager::addColumn);

    auto* dupAct = createAction(QStringLiteral("duplicateRows"),
                                tr("D&uplicate Rows"));
    editMenu->addAction(dupAct);
    connect(dupAct, &QAction::triggered, this, &MenuManager::duplicateRows);

    auto* fillAct = createAction(QStringLiteral("fillDown"), tr("Fill &Down"));
    editMenu->addAction(fillAct);
    connect(fillAct, &QAction::triggered, this, &MenuManager::fillDown);

    auto* clearAct = createAction(QStringLiteral("clearCells"),
                                  tr("Cl&ear Cells"));
    editMenu->addAction(clearAct);
    connect(clearAct, &QAction::triggered, this, &MenuManager::clearCells);
}

// ---------------------------------------------------------------------------
// View menu
// ---------------------------------------------------------------------------
void MenuManager::createViewMenu()
{
    auto* viewMenu = m_menuBar->addMenu(tr("&View"));
    m_menus.insert(QStringLiteral("view"), viewMenu);

    auto* filterAct = createAction(QStringLiteral("toggleFilter"),
                                   tr("&Filter Panel"),
                                   QKeySequence(tr("Ctrl+H")));
    filterAct->setCheckable(true);
    viewMenu->addAction(filterAct);
    connect(filterAct, &QAction::triggered, this, &MenuManager::toggleFilterPanel);

    auto* profileAct = createAction(QStringLiteral("toggleProfile"),
                                    tr("&Profile Panel"));
    profileAct->setCheckable(true);
    viewMenu->addAction(profileAct);
    connect(profileAct, &QAction::triggered, this, &MenuManager::toggleProfilePanel);

    auto* searchAct = createAction(QStringLiteral("toggleSearch"),
                                   tr("&Search Bar"),
                                   QKeySequence::Find);
    searchAct->setCheckable(true);
    viewMenu->addAction(searchAct);
    connect(searchAct, &QAction::triggered, this, &MenuManager::toggleSearchBar);

    auto* statusAct = createAction(QStringLiteral("toggleStatusBar"),
                                   tr("S&tatus Bar"));
    statusAct->setCheckable(true);
    statusAct->setChecked(true);
    viewMenu->addAction(statusAct);
    connect(statusAct, &QAction::triggered, this, &MenuManager::toggleStatusBar);

    viewMenu->addSeparator();

    auto* colMgr = createAction(QStringLiteral("columnManager"),
                                tr("&Column Manager..."));
    viewMenu->addAction(colMgr);
    connect(colMgr, &QAction::triggered, this, &MenuManager::showColumnManager);

    viewMenu->addSeparator();

    auto* zoomInAct = createAction(QStringLiteral("zoomIn"), tr("Zoom &In"),
                                   QKeySequence::ZoomIn);
    viewMenu->addAction(zoomInAct);
    connect(zoomInAct, &QAction::triggered, this, &MenuManager::zoomIn);

    auto* zoomOutAct = createAction(QStringLiteral("zoomOut"), tr("Zoom &Out"),
                                    QKeySequence::ZoomOut);
    viewMenu->addAction(zoomOutAct);
    connect(zoomOutAct, &QAction::triggered, this, &MenuManager::zoomOut);

    auto* zoomReset = createAction(QStringLiteral("resetZoom"), tr("&Reset Zoom"),
                                   QKeySequence(tr("Ctrl+0")));
    viewMenu->addAction(zoomReset);
    connect(zoomReset, &QAction::triggered, this, &MenuManager::resetZoom);
}

// ---------------------------------------------------------------------------
// Data menu
// ---------------------------------------------------------------------------
void MenuManager::createDataMenu()
{
    auto* dataMenu = m_menuBar->addMenu(tr("&Data"));
    m_menus.insert(QStringLiteral("data"), dataMenu);

    auto* applyAct = createAction(QStringLiteral("applyFilter"),
                                  tr("&Apply Filter..."));
    dataMenu->addAction(applyAct);
    connect(applyAct, &QAction::triggered, this, &MenuManager::applyFilter);

    auto* clearAct = createAction(QStringLiteral("clearFilters"),
                                  tr("&Clear Filters"));
    dataMenu->addAction(clearAct);
    connect(clearAct, &QAction::triggered, this, &MenuManager::clearFilters);

    dataMenu->addSeparator();

    auto* sortAct = createAction(QStringLiteral("sort"), tr("&Sort..."));
    dataMenu->addAction(sortAct);
    connect(sortAct, &QAction::triggered, this, &MenuManager::showSortDialog);

    dataMenu->addSeparator();

    auto* statsAct = createAction(QStringLiteral("columnStats"),
                                  tr("Column S&tatistics"));
    dataMenu->addAction(statsAct);
    connect(statsAct, &QAction::triggered, this, &MenuManager::showColumnStats);

    auto* pivotAct = createAction(QStringLiteral("pivotTable"),
                                  tr("&Pivot Table..."));
    dataMenu->addAction(pivotAct);
    connect(pivotAct, &QAction::triggered, this, &MenuManager::showPivotTable);

    auto* lookupAct = createAction(QStringLiteral("lookupWizard"),
                                   tr("&Lookup Wizard..."));
    dataMenu->addAction(lookupAct);
    connect(lookupAct, &QAction::triggered, this, &MenuManager::showLookupWizard);

    dataMenu->addSeparator();

    auto* refreshAct = createAction(QStringLiteral("refresh"), tr("&Refresh"),
                                    QKeySequence::Refresh);
    dataMenu->addAction(refreshAct);
    connect(refreshAct, &QAction::triggered, this, &MenuManager::refreshData);
}

// ---------------------------------------------------------------------------
// Tools menu
// ---------------------------------------------------------------------------
void MenuManager::createToolsMenu()
{
    auto* toolsMenu = m_menuBar->addMenu(tr("&Tools"));
    m_menus.insert(QStringLiteral("tools"), toolsMenu);

    auto* prefsAct = createAction(QStringLiteral("preferences"),
                                  tr("&Preferences..."),
                                  QKeySequence::Preferences);
    toolsMenu->addAction(prefsAct);
    connect(prefsAct, &QAction::triggered, this, &MenuManager::showPreferences);
}

// ---------------------------------------------------------------------------
// Help menu
// ---------------------------------------------------------------------------
void MenuManager::createHelpMenu()
{
    auto* helpMenu = m_menuBar->addMenu(tr("&Help"));
    m_menus.insert(QStringLiteral("help"), helpMenu);

    auto* aboutAct = createAction(QStringLiteral("about"),
                                  tr("&About CSV Forge"));
    helpMenu->addAction(aboutAct);
    connect(aboutAct, &QAction::triggered, this, &MenuManager::showAbout);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
QAction* MenuManager::createAction(const QString& name, const QString& text,
                                   const QKeySequence& shortcut)
{
    auto* act = new QAction(text, this);
    if (!shortcut.isEmpty()) {
        act->setShortcut(shortcut);
    }
    m_actions.insert(name, act);
    return act;
}

void MenuManager::setEditActionsEnabled(bool enabled)
{
    const QStringList names = {
        QStringLiteral("undo"),      QStringLiteral("redo"),
        QStringLiteral("cut"),       QStringLiteral("paste"),
        QStringLiteral("delete"),    QStringLiteral("addRow"),
        QStringLiteral("addColumn"), QStringLiteral("duplicateRows"),
        QStringLiteral("fillDown"),  QStringLiteral("clearCells"),
    };
    for (const auto& n : names) {
        if (auto* a = action(n))
            a->setEnabled(enabled);
    }
}

void MenuManager::setUndoEnabled(bool enabled)
{
    if (auto* a = action(QStringLiteral("undo")))
        a->setEnabled(enabled);
}

void MenuManager::setRedoEnabled(bool enabled)
{
    if (auto* a = action(QStringLiteral("redo")))
        a->setEnabled(enabled);
}

void MenuManager::setFileLoaded(bool loaded)
{
    const QStringList names = {
        QStringLiteral("export"),       QStringLiteral("columnStats"),
        QStringLiteral("pivotTable"),   QStringLiteral("lookupWizard"),
        QStringLiteral("applyFilter"),  QStringLiteral("clearFilters"),
        QStringLiteral("sort"),         QStringLiteral("refresh"),
        QStringLiteral("toggleFilter"), QStringLiteral("toggleProfile"),
        QStringLiteral("toggleSearch"), QStringLiteral("columnManager"),
        QStringLiteral("close"),        QStringLiteral("saveSession"),
        QStringLiteral("saveSessionAs"),
    };
    for (const auto& n : names) {
        if (auto* a = action(n))
            a->setEnabled(loaded);
    }
}

void MenuManager::updateRecentFiles(const QStringList& files)
{
    m_recentFilesMenu->clear();

    if (files.isEmpty()) {
        m_recentFilesMenu->addAction(tr("(no recent files)"))->setEnabled(false);
        return;
    }

    for (const auto& filePath : files) {
        QFileInfo fi(filePath);
        auto* act = m_recentFilesMenu->addAction(fi.fileName());
        act->setToolTip(filePath);
        act->setData(filePath);
        connect(act, &QAction::triggered, this, [this, filePath]() {
            emit openRecentFile(filePath);
        });
    }

    m_recentFilesMenu->addSeparator();
    auto* showAll = m_recentFilesMenu->addAction(tr("Show All..."));
    connect(showAll, &QAction::triggered, this, &MenuManager::showRecentFiles);
}

} // namespace csvforge
