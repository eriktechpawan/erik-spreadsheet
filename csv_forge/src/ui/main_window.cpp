#include "main_window.h"

#include "ui/menu_manager.h"
#include "ui/status_bar_manager.h"
#include "ui/toolbar_manager.h"

#include "ui/dialogs/column_manager_dialog.h"
#include "ui/dialogs/column_stats_dialog.h"
#include "ui/dialogs/export_dialog.h"
#include "ui/dialogs/filter_builder_dialog.h"
#include "ui/dialogs/import_dialog.h"
#include "ui/dialogs/lookup_wizard_dialog.h"
#include "ui/dialogs/pivot_table_dialog.h"
#include "ui/dialogs/preferences_dialog.h"
#include "ui/dialogs/recent_files_dialog.h"

#include "ui/widgets/active_filter_chip_bar.h"
#include "ui/widgets/column_profile_panel.h"
#include "ui/widgets/data_table_view.h"
#include "ui/widgets/filter_panel.h"
#include "ui/widgets/progress_overlay.h"
#include "ui/widgets/search_bar.h"

#include "data/duckdb/csv_import_service.h"
#include "data/duckdb/duckdb_engine.h"
#include "data/duckdb/export_service.h"
#include "data/duckdb/lookup_service.h"
#include "data/duckdb/pivot_service.h"
#include "data/duckdb/stats_service.h"

#include "data/model/edit_delta_store.h"
#include "data/model/filter_state.h"
#include "data/model/session_state.h"
#include "data/model/sort_state.h"
#include "data/model/table_model.h"

#include "data/types/column_schema.h"

#include "utils/constants.h"
#include "utils/file_utils.h"
#include "utils/string_utils.h"

#include "workers/export_worker.h"
#include "workers/import_worker.h"
#include "workers/query_worker.h"
#include "workers/stats_worker.h"
#include "workers/task_dispatcher.h"

#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMimeData>
#include <QSettings>
#include <QToolBar>
#include <QVBoxLayout>

namespace csvforge {

// ===========================================================================
// Construction / destruction
// ===========================================================================

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupEngine();
    setupUI();
    setupCentralWidget();
    setupConnections();
    setupDragDrop();
    loadSettings();

    setWindowTitle(QLatin1String(constants::APP_NAME));
    resize(1400, 900);
}

MainWindow::~MainWindow()
{
    saveSettings();
}

// ===========================================================================
// Setup helpers
// ===========================================================================

void MainWindow::setupEngine()
{
    m_engine = std::make_unique<DuckDBEngine>();
    m_engine->initialize();

    m_statsService  = std::make_unique<StatsService>(m_engine.get());
    m_exportService = std::make_unique<ExportService>(m_engine.get());
    m_pivotService  = std::make_unique<PivotService>(m_engine.get());
    m_lookupService = std::make_unique<LookupService>(m_engine.get());
    m_importService = std::make_unique<CSVImportService>(m_engine.get());

    m_taskDispatcher = std::make_unique<TaskDispatcher>();
    m_filterState    = new FilterState(this);
    m_sortState      = new SortState(this);
}

void MainWindow::setupUI()
{
    // Toolbar
    auto* toolbar = addToolBar(tr("Main"));
    m_toolbarManager = new ToolbarManager(toolbar, this);

    // Menu bar
    m_menuManager = new MenuManager(menuBar(), this);

    // Status bar
    m_statusBarManager = new StatusBarManager(statusBar(), this);
}

void MainWindow::setupCentralWidget()
{
    auto* centralWidget = new QWidget(this);
    auto* centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);

    // Search bar (hidden by default)
    m_searchBar = new SearchBar(centralWidget);
    m_searchBar->hide();
    centralLayout->addWidget(m_searchBar);

    // Active filter chip bar
    m_chipBar = new ActiveFilterChipBar(centralWidget);
    m_chipBar->setFilterState(m_filterState);
    m_chipBar->hide();
    centralLayout->addWidget(m_chipBar);

    // Main splitter: [filter panel | table view | profile panel]
    m_mainSplitter = new QSplitter(Qt::Horizontal, centralWidget);

    m_filterPanel = new FilterPanel(m_mainSplitter);
    m_filterPanel->setMinimumWidth(200);
    m_filterPanel->setMaximumWidth(400);
    m_filterPanel->hide();
    m_mainSplitter->addWidget(m_filterPanel);

    auto* tableContainer = new QWidget(m_mainSplitter);
    auto* tableLayout = new QVBoxLayout(tableContainer);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    m_tableView = new DataTableView(tableContainer);
    tableLayout->addWidget(m_tableView);
    m_mainSplitter->addWidget(tableContainer);

    m_profilePanel = new ColumnProfilePanel(m_mainSplitter);
    m_profilePanel->setMinimumWidth(220);
    m_profilePanel->setMaximumWidth(400);
    m_profilePanel->hide();
    m_mainSplitter->addWidget(m_profilePanel);

    // Give the table the majority of space
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 1);
    m_mainSplitter->setStretchFactor(2, 0);

    centralLayout->addWidget(m_mainSplitter, 1);

    setCentralWidget(centralWidget);

    // Progress overlay (covers the central widget)
    m_progressOverlay = new ProgressOverlay(centralWidget);
    m_progressOverlay->hide();
}

void MainWindow::setupConnections()
{
    // ---- ToolbarManager signals ----
    connect(m_toolbarManager, &ToolbarManager::openFile,
            this, &MainWindow::onOpenFile);
    connect(m_toolbarManager, &ToolbarManager::saveSession,
            this, &MainWindow::onSaveSession);
    connect(m_toolbarManager, &ToolbarManager::importFile,
            this, &MainWindow::onImportFile);
    connect(m_toolbarManager, &ToolbarManager::exportData,
            this, &MainWindow::onExportData);
    connect(m_toolbarManager, &ToolbarManager::undo,
            this, &MainWindow::onUndo);
    connect(m_toolbarManager, &ToolbarManager::redo,
            this, &MainWindow::onRedo);
    connect(m_toolbarManager, &ToolbarManager::toggleEditMode,
            this, &MainWindow::onToggleEditMode);
    connect(m_toolbarManager, &ToolbarManager::toggleFilterPanel,
            this, &MainWindow::onToggleFilterPanel);
    connect(m_toolbarManager, &ToolbarManager::toggleProfilePanel,
            this, &MainWindow::onToggleProfilePanel);
    connect(m_toolbarManager, &ToolbarManager::refreshData,
            this, &MainWindow::onRefreshData);
    connect(m_toolbarManager, &ToolbarManager::showColumnStats,
            this, &MainWindow::onShowColumnStats);
    connect(m_toolbarManager, &ToolbarManager::showPivotTable,
            this, &MainWindow::onShowPivotTable);
    connect(m_toolbarManager, &ToolbarManager::showLookupWizard,
            this, &MainWindow::onShowLookupWizard);
    connect(m_toolbarManager, &ToolbarManager::showPreferences,
            this, &MainWindow::onShowPreferences);

    // ---- MenuManager signals ----
    connect(m_menuManager, &MenuManager::openFile,
            this, &MainWindow::onOpenFile);
    connect(m_menuManager, &MenuManager::openRecentFile,
            this, &MainWindow::onRecentFileSelected);
    connect(m_menuManager, &MenuManager::saveSession,
            this, &MainWindow::onSaveSession);
    connect(m_menuManager, &MenuManager::saveSessionAs,
            this, &MainWindow::onSaveSessionAs);
    connect(m_menuManager, &MenuManager::loadSession,
            this, &MainWindow::onLoadSession);
    connect(m_menuManager, &MenuManager::importFile,
            this, &MainWindow::onImportFile);
    connect(m_menuManager, &MenuManager::exportData,
            this, &MainWindow::onExportData);
    connect(m_menuManager, &MenuManager::showRecentFiles,
            this, &MainWindow::onShowRecentFiles);
    connect(m_menuManager, &MenuManager::closeFile,
            this, &MainWindow::onCloseFile);
    connect(m_menuManager, &MenuManager::undo,
            this, &MainWindow::onUndo);
    connect(m_menuManager, &MenuManager::redo,
            this, &MainWindow::onRedo);
    connect(m_menuManager, &MenuManager::cut,
            this, &MainWindow::onCopy);  // cut delegates to copy in read-heavy tool
    connect(m_menuManager, &MenuManager::copy,
            this, &MainWindow::onCopy);
    connect(m_menuManager, &MenuManager::paste,
            this, &MainWindow::onPaste);
    connect(m_menuManager, &MenuManager::selectAll,
            this, &MainWindow::onSelectAll);
    connect(m_menuManager, &MenuManager::deleteSelection,
            this, &MainWindow::onDeleteSelection);
    connect(m_menuManager, &MenuManager::addRow,
            this, &MainWindow::onAddRow);
    connect(m_menuManager, &MenuManager::addColumn,
            this, &MainWindow::onAddColumn);
    connect(m_menuManager, &MenuManager::duplicateRows,
            this, &MainWindow::onDuplicateRows);
    connect(m_menuManager, &MenuManager::fillDown,
            this, &MainWindow::onFillDown);
    connect(m_menuManager, &MenuManager::clearCells,
            this, &MainWindow::onClearCells);
    connect(m_menuManager, &MenuManager::toggleEditMode,
            this, &MainWindow::onToggleEditMode);
    connect(m_menuManager, &MenuManager::toggleFilterPanel,
            this, &MainWindow::onToggleFilterPanel);
    connect(m_menuManager, &MenuManager::toggleProfilePanel,
            this, &MainWindow::onToggleProfilePanel);
    connect(m_menuManager, &MenuManager::toggleSearchBar,
            this, &MainWindow::onToggleSearchBar);
    connect(m_menuManager, &MenuManager::showColumnManager,
            this, &MainWindow::onShowColumnManager);
    connect(m_menuManager, &MenuManager::applyFilter,
            this, &MainWindow::onApplyFilter);
    connect(m_menuManager, &MenuManager::clearFilters,
            this, &MainWindow::onClearFilters);
    connect(m_menuManager, &MenuManager::showColumnStats,
            this, &MainWindow::onShowColumnStats);
    connect(m_menuManager, &MenuManager::showPivotTable,
            this, &MainWindow::onShowPivotTable);
    connect(m_menuManager, &MenuManager::showLookupWizard,
            this, &MainWindow::onShowLookupWizard);
    connect(m_menuManager, &MenuManager::refreshData,
            this, &MainWindow::onRefreshData);
    connect(m_menuManager, &MenuManager::showPreferences,
            this, &MainWindow::onShowPreferences);
    connect(m_menuManager, &MenuManager::showAbout,
            this, &MainWindow::onShowAbout);

    // ---- DataTableView signals ----
    connect(m_tableView, &DataTableView::requestColumnStats,
            this, &MainWindow::onRequestColumnStats);
    connect(m_tableView, &DataTableView::requestSort,
            this, &MainWindow::onRequestSort);
    connect(m_tableView, &DataTableView::requestFilter,
            this, [this](const QString& col, const QVariant& val) {
                m_filterPanel->showQuickFilterForColumn(col, val);
                m_filterPanel->show();
            });
    connect(m_tableView, &DataTableView::scrolledToRow,
            this, &MainWindow::onScrolledToRow);
    connect(m_tableView, &DataTableView::selectionCountChanged,
            this, &MainWindow::onSelectionChanged);

    // ---- FilterPanel ----
    connect(m_filterPanel, &FilterPanel::filterChanged,
            this, [this](const FilterGroup& /*group*/) { onFilterChanged(); });
    connect(m_filterPanel, &FilterPanel::filterCleared,
            this, &MainWindow::onClearFilters);

    // ---- Chip bar ----
    connect(m_chipBar, &ActiveFilterChipBar::filterRemoved,
            this, &MainWindow::onFilterRemoved);
    connect(m_chipBar, &ActiveFilterChipBar::allFiltersCleared,
            this, &MainWindow::onAllFiltersCleared);

    // ---- Search bar ----
    connect(m_searchBar, &SearchBar::searchRequested,
            this, &MainWindow::onSearchRequested);
    connect(m_searchBar, &SearchBar::nextMatch,
            this, &MainWindow::onNextMatch);
    connect(m_searchBar, &SearchBar::previousMatch,
            this, &MainWindow::onPreviousMatch);
    connect(m_searchBar, &SearchBar::searchCleared, this, [this]() {
        m_searchResults.clear();
        m_currentSearchIndex = -1;
        m_searchBar->setMatchCount(0);
    });

    // ---- State objects ----
    connect(m_filterState, &FilterState::filterChanged,
            this, &MainWindow::onFilterChanged);
    connect(m_sortState, &SortState::sortChanged,
            this, &MainWindow::onSortChanged);

    // ---- Task dispatcher ----
    connect(m_taskDispatcher.get(), &TaskDispatcher::taskProgress,
            this, &MainWindow::onTaskProgress);
    connect(m_taskDispatcher.get(), &TaskDispatcher::taskFinished,
            this, &MainWindow::onTaskFinished);

    // ---- Profile panel filter request ----
    connect(m_profilePanel, &ColumnProfilePanel::filterRequested,
            this, [this](const QString& col, FilterOperator op,
                         const QVariant& val) {
                m_filterState->applyQuickFilter(col, op, val);
            });

    // ---- Progress overlay cancel ----
    connect(m_progressOverlay, &ProgressOverlay::cancelled,
            this, [this]() { m_taskDispatcher->cancelAll(); });
}

void MainWindow::setupDragDrop()
{
    setAcceptDrops(true);
}

// ===========================================================================
// File operations
// ===========================================================================

void MainWindow::onOpenFile()
{
    const QString filter = tr("CSV files (*.csv *.tsv *.txt);;All files (*)");
    const QString filePath =
        QFileDialog::getOpenFileName(this, tr("Open File"), QString(), filter);
    if (filePath.isEmpty())
        return;

    ImportDialog dlg(filePath, this);
    if (dlg.exec() == QDialog::Accepted) {
        loadFileIntoEngine(filePath, dlg.importOptions());
    }
}

void MainWindow::openFile(const QString& filePath)
{
    if (filePath.isEmpty())
        return;
    ImportDialog dlg(filePath, this);
    if (dlg.exec() == QDialog::Accepted) {
        loadFileIntoEngine(filePath, dlg.importOptions());
    }
}

void MainWindow::onImportFile()
{
    ImportDialog dlg({}, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    // The ImportDialog's file path line edit stores the user-chosen path.
    // Retrieve it from the dialog's filePath property.
    const QString filePath = dlg.property("filePath").toString();
    if (filePath.isEmpty())
        return;

    loadFileIntoEngine(filePath, dlg.importOptions());
}

void MainWindow::loadFileIntoEngine(const QString& filePath,
                                     const CSVImportOptions& options)
{
    m_progressOverlay->showProgress(tr("Importing %1...").arg(QFileInfo(filePath).fileName()));

    m_currentFilePath = filePath;
    const QString tableName = QStringLiteral("csvforge_data");

    auto* worker = new ImportWorker(m_engine.get(), filePath, tableName,
                                    options);
    connect(worker, &ImportWorker::importComplete, this,
            [this](const CSVImportService::ImportResult& result) {
                onImportComplete(result.success,
                                 result.success ? result.tableName : result.error);
            });
    m_taskDispatcher->submit(worker);
}

void MainWindow::onImportComplete(bool success, const QString& result)
{
    m_progressOverlay->hideProgress();

    if (!success) {
        QMessageBox::critical(this, tr("Import Error"), result);
        return;
    }

    m_currentTableName = result;
    updateUIAfterLoad();
}

void MainWindow::updateUIAfterLoad()
{
    const auto schema = m_engine->getTableSchema(m_currentTableName);
    const qint64 totalRows = m_engine->getRowCount(m_currentTableName);

    // Create / replace table model
    if (m_tableModel) {
        m_tableModel->deleteLater();
    }
    m_tableModel = new TableModel(m_engine.get(), this);
    m_tableModel->setTableName(m_currentTableName);
    m_tableModel->setSchema(schema);
    m_tableModel->setTotalRowCount(totalRows);
    m_tableModel->setFilteredRowCount(totalRows);
    m_tableModel->setFilterState(m_filterState);
    m_tableModel->setSortState(m_sortState);

    m_tableView->setTableModel(m_tableModel);

    // Configure filter panel
    m_filterPanel->setEngine(m_engine.get());
    m_filterPanel->setTableName(m_currentTableName);
    m_filterPanel->setSchema(schema);
    m_filterPanel->setFilterState(m_filterState);

    // Configure search bar column list
    QStringList colNames;
    colNames.reserve(static_cast<int>(schema.size()));
    for (const auto& col : schema) {
        colNames.append(col.name);
    }
    m_searchBar->setColumns(colNames);

    // Enable UI
    m_toolbarManager->setFileLoaded(true);
    m_menuManager->setFileLoaded(true);

    updateStatusBar();
    updateWindowTitle();
    addToRecentFiles(m_currentFilePath);
    m_menuManager->updateRecentFiles(m_recentFiles);
}

void MainWindow::onExportData()
{
    if (!m_tableModel)
        return;

    ExportDialog dlg(m_tableModel->schema(), this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    auto options = dlg.exportOptions();
    options.tableName = m_currentTableName;
    options.whereClause = m_filterState->toWhereClause();

    m_progressOverlay->showProgress(tr("Exporting..."));

    auto* worker = new ExportWorker(m_engine.get(), options);
    connect(worker, &ExportWorker::exportComplete, this,
            [this](const ExportService::ExportResult& result) {
                onExportComplete(result.success,
                                 result.success
                                     ? tr("Exported %1 rows").arg(result.rowsExported)
                                     : result.error);
            });
    m_taskDispatcher->submit(worker);
}

void MainWindow::onExportComplete(bool success, const QString& result)
{
    m_progressOverlay->hideProgress();
    if (success) {
        m_statusBarManager->setMessage(result);
    } else {
        QMessageBox::warning(this, tr("Export Error"), result);
    }
}

void MainWindow::onSaveSession()
{
    if (m_sessionFilePath.isEmpty()) {
        onSaveSessionAs();
        return;
    }
    saveSessionState(m_sessionFilePath);
}

void MainWindow::onSaveSessionAs()
{
    const QString filter = tr("CSV Forge Session (*%1)")
                               .arg(QLatin1String(constants::SESSION_FILE_EXTENSION));
    const QString path =
        QFileDialog::getSaveFileName(this, tr("Save Session"), QString(), filter);
    if (path.isEmpty())
        return;

    m_sessionFilePath = path;
    saveSessionState(m_sessionFilePath);
}

void MainWindow::onLoadSession()
{
    const QString filter = tr("CSV Forge Session (*%1)")
                               .arg(QLatin1String(constants::SESSION_FILE_EXTENSION));
    const QString path =
        QFileDialog::getOpenFileName(this, tr("Load Session"), QString(), filter);
    if (path.isEmpty())
        return;

    loadSessionState(path);
}

void MainWindow::onCloseFile()
{
    if (!confirmUnsavedChanges())
        return;

    if (m_tableModel) {
        m_tableModel->deleteLater();
        m_tableModel = nullptr;
    }

    m_filterState->clearFilter();
    m_sortState->clearSort();
    m_searchBar->clear();
    m_profilePanel->clear();
    m_chipBar->updateChips();

    m_currentFilePath.clear();
    m_currentTableName.clear();
    m_sessionFilePath.clear();
    m_editableMode = false;

    m_toolbarManager->setFileLoaded(false);
    m_menuManager->setFileLoaded(false);
    m_statusBarManager->clear();
    updateWindowTitle();
}

void MainWindow::onRecentFileSelected(const QString& path)
{
    openFile(path);
}

void MainWindow::onShowRecentFiles()
{
    RecentFilesDialog dlg(m_recentFiles, this);
    connect(&dlg, &RecentFilesDialog::filesChanged,
            this, [this](const QStringList& remaining) {
                m_recentFiles = remaining;
                m_menuManager->updateRecentFiles(m_recentFiles);
            });

    if (dlg.exec() == QDialog::Accepted) {
        const QString sel = dlg.selectedFile();
        if (!sel.isEmpty())
            openFile(sel);
    }
}

// ===========================================================================
// Edit operations
// ===========================================================================

void MainWindow::onUndo()
{
    if (!m_tableModel || !m_editableMode)
        return;
    m_tableModel->editStore()->undo();
    m_tableModel->refresh();
}

void MainWindow::onRedo()
{
    if (!m_tableModel || !m_editableMode)
        return;
    m_tableModel->editStore()->redo();
    m_tableModel->refresh();
}

void MainWindow::onCopy()
{
    // DataTableView inherits QAbstractItemView which doesn't have a built-in
    // copy. Clipboard handling will be implemented in a future iteration.
    Q_UNUSED(m_tableView);
}

void MainWindow::onPaste()
{
    // Paste support will be implemented in a future iteration.
    Q_UNUSED(m_tableView);
}

void MainWindow::onDeleteSelection()
{
    if (!m_tableModel || !m_editableMode)
        return;
    // Placeholder for row/cell deletion logic
}

void MainWindow::onSelectAll()
{
    if (m_tableView)
        m_tableView->selectAll();
}

void MainWindow::onAddRow()
{
    if (!m_tableModel || !m_editableMode)
        return;
    // Placeholder: insert row via edit delta store
}

void MainWindow::onAddColumn()
{
    if (!m_tableModel || !m_editableMode)
        return;
    // Placeholder: add column via dialog
}

void MainWindow::onDuplicateRows()
{
    if (!m_tableModel || !m_editableMode)
        return;
    // Placeholder: duplicate selected rows
}

void MainWindow::onFillDown()
{
    if (!m_tableModel || !m_editableMode)
        return;
    // Placeholder: fill selected cells down
}

void MainWindow::onClearCells()
{
    if (!m_tableModel || !m_editableMode)
        return;
    // Placeholder: clear selected cells
}

void MainWindow::onToggleEditMode()
{
    m_editableMode = !m_editableMode;
    if (m_tableModel)
        m_tableModel->setEditable(m_editableMode);
    m_toolbarManager->setEditActionsEnabled(m_editableMode);
    m_menuManager->setEditActionsEnabled(m_editableMode);
    m_statusBarManager->setMode(m_editableMode);
}

// ===========================================================================
// View operations
// ===========================================================================

void MainWindow::onToggleFilterPanel()
{
    m_filterPanel->setVisible(!m_filterPanel->isVisible());
}

void MainWindow::onToggleProfilePanel()
{
    m_profilePanel->setVisible(!m_profilePanel->isVisible());
}

void MainWindow::onToggleSearchBar()
{
    const bool show = !m_searchBar->isVisible();
    m_searchBar->setVisible(show);
    if (show)
        m_searchBar->focus();
}

void MainWindow::onShowColumnManager()
{
    if (!m_tableModel)
        return;

    ColumnManagerDialog dlg(m_tableModel->schema(), this);
    if (dlg.exec() == QDialog::Accepted) {
        m_tableModel->setSchema(dlg.getSchema());
        m_tableModel->refresh();
        updateStatusBar();
    }
}

// ===========================================================================
// Data operations
// ===========================================================================

void MainWindow::onApplyFilter()
{
    if (!m_tableModel)
        return;

    FilterBuilderDialog dlg(m_tableModel->schema(),
                            m_filterState->currentFilter(), this);
    if (dlg.exec() == QDialog::Accepted) {
        m_filterState->setFilter(dlg.filterGroup());
    }
}

void MainWindow::onClearFilters()
{
    m_filterState->clearFilter();
}

void MainWindow::onShowColumnStats()
{
    if (!m_tableModel)
        return;

    const auto& schema = m_tableModel->schema();
    if (schema.empty())
        return;

    const QString col = schema.front().name;
    onRequestColumnStats(col);
}

void MainWindow::onRequestColumnStats(const QString& columnName)
{
    if (!m_tableModel)
        return;

    m_profilePanel->setLoading(true);
    m_profilePanel->show();

    auto* worker = new StatsWorker(m_engine.get(), m_currentTableName,
                                   columnName, m_filterState->toWhereClause());
    connect(worker, &StatsWorker::statsReady, this, &MainWindow::onStatsReady);
    m_taskDispatcher->submit(worker);
}

void MainWindow::onStatsReady(const ColumnStats& stats)
{
    m_profilePanel->showStats(stats);
}

void MainWindow::onShowPivotTable()
{
    if (!m_tableModel)
        return;

    PivotTableDialog dlg(m_tableModel->schema(), m_engine.get(), this);
    if (dlg.exec() == QDialog::Accepted) {
        auto result = m_pivotService->executePivot(dlg.getConfig());
        if (!result.success) {
            QMessageBox::warning(this, tr("Pivot Error"), result.error);
        } else {
            m_statusBarManager->setMessage(
                tr("Pivot table created: %1").arg(result.resultTableName));
        }
    }
}

void MainWindow::onShowLookupWizard()
{
    if (!m_tableModel)
        return;

    LookupWizardDialog dlg(m_currentTableName, m_tableModel->schema(),
                           m_engine.get(), this);
    if (dlg.exec() == QDialog::Accepted) {
        auto result = m_lookupService->executeLookup(dlg.getConfig());
        if (!result.success) {
            QMessageBox::warning(this, tr("Lookup Error"), result.error);
        } else {
            m_statusBarManager->setMessage(
                tr("Lookup complete: %1 matched, %2 unmatched")
                    .arg(result.matchedRows)
                    .arg(result.unmatchedRows));
        }
    }
}

void MainWindow::onRefreshData()
{
    if (!m_tableModel)
        return;

    m_tableModel->invalidateCache();
    m_tableModel->refresh();
    updateStatusBar();
}

// ===========================================================================
// Sort
// ===========================================================================

void MainWindow::onRequestSort(const QString& columnName)
{
    m_sortState->toggleSort(columnName);
}

// ===========================================================================
// Search
// ===========================================================================

void MainWindow::onSearchRequested(const QString& text, const QString& column,
                                    bool caseSensitive, bool regex)
{
    if (!m_tableModel || text.isEmpty()) {
        m_searchResults.clear();
        m_currentSearchIndex = -1;
        m_searchBar->setMatchCount(0);
        return;
    }

    // Build a SQL query to find matching rows
    const QString whereClause = m_filterState->toWhereClause();
    QString colExpr = column.isEmpty()
                          ? QStringLiteral("*")
                          : quoteName(column);

    Q_UNUSED(caseSensitive);
    Q_UNUSED(regex);

    // Use LIKE-based search for simplicity; regex support can be added later
    QString searchSql =
        QStringLiteral("SELECT rowid, * FROM %1").arg(quoteName(m_currentTableName));
    if (!whereClause.isEmpty()) {
        searchSql += QStringLiteral(" WHERE ") + whereClause;
    }

    // Results will be populated asynchronously via QueryWorker
    m_searchResults.clear();
    m_currentSearchIndex = -1;
    m_searchBar->setMatchCount(0);

    Q_UNUSED(colExpr);
    Q_UNUSED(searchSql);
    // TODO: implement full search via QueryWorker
}

void MainWindow::onNextMatch()
{
    if (m_searchResults.isEmpty())
        return;
    m_currentSearchIndex =
        (m_currentSearchIndex + 1) % m_searchResults.size();
    m_searchBar->setCurrentMatch(m_currentSearchIndex + 1);
    // TODO: scroll table view to the match row
}

void MainWindow::onPreviousMatch()
{
    if (m_searchResults.isEmpty())
        return;
    m_currentSearchIndex =
        (m_currentSearchIndex - 1 + m_searchResults.size()) % m_searchResults.size();
    m_searchBar->setCurrentMatch(m_currentSearchIndex + 1);
    // TODO: scroll table view to the match row
}

// ===========================================================================
// Filter chips
// ===========================================================================

void MainWindow::onFilterRemoved(int index)
{
    m_filterState->removeRule(index);
}

void MainWindow::onAllFiltersCleared()
{
    m_filterState->clearFilter();
}

// ===========================================================================
// Scroll-driven data fetch
// ===========================================================================

void MainWindow::onScrolledToRow(qint64 row)
{
    fetchPageForRow(row);
}

void MainWindow::fetchPageForRow(qint64 row)
{
    if (!m_tableModel)
        return;

    const int pageSize = constants::DEFAULT_PAGE_SIZE;
    const qint64 pageStart = (row / pageSize) * pageSize;

    const QString orderBy = m_sortState->toOrderByClause();
    const QString where = m_filterState->toWhereClause();

    QString sql = QStringLiteral("SELECT * FROM %1").arg(quoteName(m_currentTableName));
    if (!where.isEmpty())
        sql += QStringLiteral(" WHERE ") + where;
    if (!orderBy.isEmpty())
        sql += QStringLiteral(" ORDER BY ") + orderBy;
    sql += QStringLiteral(" LIMIT %1 OFFSET %2").arg(pageSize).arg(pageStart);

    auto* worker = new QueryWorker(m_engine.get(), sql);
    connect(worker, &QueryWorker::dataReady, this, &MainWindow::onPageFetched);
    m_taskDispatcher->submit(worker);
}

void MainWindow::onPageFetched(qint64 startRow,
                                const QVector<QVector<QVariant>>& rows)
{
    if (m_tableModel) {
        m_tableModel->onPageFetched(startRow, rows);
    }
}

// ===========================================================================
// Task progress
// ===========================================================================

void MainWindow::onTaskProgress(const QString& /*taskName*/, int percent,
                                 const QString& message)
{
    m_progressOverlay->setProgress(percent, message);
}

void MainWindow::onTaskFinished(const QString& taskName, bool success,
                                 const QString& result)
{
    Q_UNUSED(taskName);
    if (!success && !result.isEmpty()) {
        m_statusBarManager->setMessage(tr("Task failed: %1").arg(result), 5000);
    }
}

// ===========================================================================
// State changes
// ===========================================================================

void MainWindow::onFilterChanged()
{
    if (!m_tableModel)
        return;

    const QString where = m_filterState->toWhereClause();
    const qint64 filtered =
        m_engine->getFilteredRowCount(m_currentTableName, where);
    m_tableModel->setFilteredRowCount(filtered);
    m_tableModel->invalidateCache();
    m_tableModel->refresh();

    m_chipBar->updateChips();
    updateStatusBar();
}

void MainWindow::onSortChanged()
{
    if (!m_tableModel)
        return;

    m_tableModel->invalidateCache();
    m_tableModel->refresh();
    m_statusBarManager->setSortState(m_sortState->toDisplayString());
}

void MainWindow::onDirtyStateChanged(bool dirty)
{
    m_statusBarManager->setDirtyState(dirty);
    updateWindowTitle();
}

void MainWindow::onSelectionChanged(int rowCount)
{
    m_statusBarManager->setSelectedRows(rowCount);
}

// ===========================================================================
// Preferences & About
// ===========================================================================

void MainWindow::onShowPreferences()
{
    PreferencesDialog dlg(this);
    dlg.exec();
}

void MainWindow::onShowAbout()
{
    QMessageBox::about(
        this, tr("About %1").arg(QLatin1String(constants::APP_NAME)),
        tr("<h3>%1 v%2</h3>"
           "<p>A lightweight, high-performance CSV viewer and editor "
           "powered by DuckDB.</p>")
            .arg(QLatin1String(constants::APP_NAME),
                 QLatin1String(constants::APP_VERSION)));
}

// ===========================================================================
// Status bar / window title helpers
// ===========================================================================

void MainWindow::updateStatusBar()
{
    if (!m_tableModel) {
        m_statusBarManager->clear();
        return;
    }

    QFileInfo fi(m_currentFilePath);
    m_statusBarManager->setFileName(fi.fileName());
    m_statusBarManager->setTotalRows(m_tableModel->totalRowCount());
    m_statusBarManager->setFilteredRows(m_tableModel->filteredRowCount());
    m_statusBarManager->setSelectedRows(m_tableView->selectedRowCount());
    m_statusBarManager->setVisibleColumns(
        m_tableView->visibleColumnCount(),
        static_cast<int>(m_tableModel->schema().size()));
    m_statusBarManager->setMode(m_editableMode);
    m_statusBarManager->setSortState(m_sortState->toDisplayString());
}

void MainWindow::updateWindowTitle()
{
    QString title = QLatin1String(constants::APP_NAME);
    if (!m_currentFilePath.isEmpty()) {
        title = QStringLiteral("%1 – %2").arg(QFileInfo(m_currentFilePath).fileName(),
                                              QLatin1String(constants::APP_NAME));
    }
    if (m_tableModel && m_tableModel->editStore()->isDirty()) {
        title.prepend(QStringLiteral("● "));
    }
    setWindowTitle(title);
}

// ===========================================================================
// Recent files
// ===========================================================================

void MainWindow::addToRecentFiles(const QString& filePath)
{
    m_recentFiles.removeAll(filePath);
    m_recentFiles.prepend(filePath);
    while (m_recentFiles.size() > constants::MAX_RECENT_FILES) {
        m_recentFiles.removeLast();
    }
}

// ===========================================================================
// Session persistence
// ===========================================================================

void MainWindow::saveSessionState(const QString& path)
{
    SessionState state;
    state.sourceFilePath = m_currentFilePath;
    state.sessionFilePath = path;
    state.editableMode = m_editableMode;
    state.filters = m_filterState->currentFilter();
    state.sorts = m_sortState->columns();
    state.hiddenColumns = {};
    state.recentFiles = m_recentFiles;
    state.windowGeometry = saveGeometry();
    state.windowState = saveState();

    if (!state.saveToFile(path)) {
        QMessageBox::warning(this, tr("Save Error"),
                             tr("Could not save session to %1").arg(path));
    } else {
        m_sessionFilePath = path;
        m_statusBarManager->setMessage(tr("Session saved"), 3000);
    }
}

void MainWindow::loadSessionState(const QString& path)
{
    auto opt = SessionState::loadFromFile(path);
    if (!opt.has_value()) {
        QMessageBox::warning(this, tr("Load Error"),
                             tr("Could not load session from %1").arg(path));
        return;
    }

    const auto& state = opt.value();

    // Restore window geometry
    if (!state.windowGeometry.isEmpty())
        restoreGeometry(state.windowGeometry);
    if (!state.windowState.isEmpty())
        restoreState(state.windowState);

    m_editableMode = state.editableMode;
    m_recentFiles  = state.recentFiles;

    // Reload file
    if (!state.sourceFilePath.isEmpty() && QFileInfo::exists(state.sourceFilePath)) {
        CSVImportOptions defaultOpts;
        loadFileIntoEngine(state.sourceFilePath, defaultOpts);
    }

    // Restore filter / sort after file is loaded
    m_filterState->setFilter(state.filters);
    for (const auto& sc : state.sorts) {
        m_sortState->addSort(sc.columnName, sc.ascending);
    }

    m_sessionFilePath = path;
    m_menuManager->updateRecentFiles(m_recentFiles);
}

bool MainWindow::confirmUnsavedChanges()
{
    if (!m_tableModel || !m_tableModel->editStore()->isDirty())
        return true;

    const auto btn = QMessageBox::question(
        this, tr("Unsaved Changes"),
        tr("You have unsaved changes. Do you want to save the session before "
           "closing?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);

    if (btn == QMessageBox::Save) {
        onSaveSession();
        return true;
    }
    return btn == QMessageBox::Discard;
}

// ===========================================================================
// Settings persistence (QSettings)
// ===========================================================================

void MainWindow::loadSettings()
{
    QSettings settings(QLatin1String(constants::APP_NAME),
                       QLatin1String(constants::APP_NAME));

    const QByteArray geo = settings.value(QStringLiteral("geometry")).toByteArray();
    if (!geo.isEmpty())
        restoreGeometry(geo);

    const QByteArray state = settings.value(QStringLiteral("windowState")).toByteArray();
    if (!state.isEmpty())
        restoreState(state);

    m_recentFiles =
        settings.value(QStringLiteral("recentFiles")).toStringList();
    m_menuManager->updateRecentFiles(m_recentFiles);
}

void MainWindow::saveSettings()
{
    QSettings settings(QLatin1String(constants::APP_NAME),
                       QLatin1String(constants::APP_NAME));
    settings.setValue(QStringLiteral("geometry"), saveGeometry());
    settings.setValue(QStringLiteral("windowState"), saveState());
    settings.setValue(QStringLiteral("recentFiles"), m_recentFiles);
}

// ===========================================================================
// Drag & drop
// ===========================================================================

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (!event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }
    for (const auto& url : event->mimeData()->urls()) {
        if (isSupportedFile(url.toLocalFile())) {
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    const auto urls = event->mimeData()->urls();
    if (urls.isEmpty())
        return;

    const QString filePath = urls.first().toLocalFile();
    if (!filePath.isEmpty() && isSupportedFile(filePath)) {
        openFile(filePath);
    }
}

// ===========================================================================
// Close event
// ===========================================================================

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (!confirmUnsavedChanges()) {
        event->ignore();
        return;
    }
    saveSettings();
    event->accept();
}

} // namespace csvforge
