#pragma once

#include <QMainWindow>
#include <QPair>
#include <QSplitter>
#include <QStringList>
#include <QVector>
#include <memory>

namespace csvforge {

class DuckDBEngine;
class TableModel;
class FilterState;
class SortState;
class TaskDispatcher;
class DataTableView;
class FilterPanel;
class SearchBar;
class ActiveFilterChipBar;
class ColumnProfilePanel;
class ProgressOverlay;
class ToolbarManager;
class MenuManager;
class StatusBarManager;
class StatsService;
class ExportService;
class PivotService;
class LookupService;
class CSVImportService;
struct CSVImportOptions;
struct ColumnStats;
struct SessionState;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void openFile(const QString& filePath);

protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    // File operations
    void onOpenFile();
    void onImportFile();
    void onExportData();
    void onSaveSession();
    void onSaveSessionAs();
    void onLoadSession();
    void onCloseFile();
    void onRecentFileSelected(const QString& path);
    void onShowRecentFiles();

    // Edit operations
    void onUndo();
    void onRedo();
    void onCopy();
    void onPaste();
    void onDeleteSelection();
    void onSelectAll();
    void onAddRow();
    void onAddColumn();
    void onDuplicateRows();
    void onFillDown();
    void onClearCells();
    void onToggleEditMode();

    // View operations
    void onToggleFilterPanel();
    void onToggleProfilePanel();
    void onToggleSearchBar();
    void onShowColumnManager();

    // Data operations
    void onApplyFilter();
    void onClearFilters();
    void onShowColumnStats();
    void onRequestColumnStats(const QString& columnName);
    void onShowPivotTable();
    void onShowLookupWizard();
    void onRefreshData();

    // Sort
    void onRequestSort(const QString& columnName);

    // Search
    void onSearchRequested(const QString& text, const QString& column,
                           bool caseSensitive, bool regex);
    void onNextMatch();
    void onPreviousMatch();

    // Filter chips
    void onFilterRemoved(int index);
    void onAllFiltersCleared();

    // Worker results
    void onImportComplete(bool success, const QString& result);
    void onStatsReady(const ColumnStats& stats);
    void onExportComplete(bool success, const QString& result);

    // Scroll-driven data fetch
    void onScrolledToRow(qint64 row);
    void onPageFetched(qint64 startRow, const QVector<QVector<QVariant>>& rows);

    // Task progress
    void onTaskProgress(const QString& taskName, int percent,
                        const QString& message);
    void onTaskFinished(const QString& taskName, bool success,
                        const QString& result);

    // State changes
    void onFilterChanged();
    void onSortChanged();
    void onDirtyStateChanged(bool dirty);
    void onSelectionChanged(int rowCount);

    // Preferences
    void onShowPreferences();
    void onShowAbout();

private:
    // Core engine
    std::unique_ptr<DuckDBEngine> m_engine;
    std::unique_ptr<TaskDispatcher> m_taskDispatcher;

    // Services
    std::unique_ptr<StatsService> m_statsService;
    std::unique_ptr<ExportService> m_exportService;
    std::unique_ptr<PivotService> m_pivotService;
    std::unique_ptr<LookupService> m_lookupService;
    std::unique_ptr<CSVImportService> m_importService;

    // Data model
    TableModel* m_tableModel = nullptr;
    FilterState* m_filterState = nullptr;
    SortState* m_sortState = nullptr;

    // UI managers
    ToolbarManager* m_toolbarManager = nullptr;
    MenuManager* m_menuManager = nullptr;
    StatusBarManager* m_statusBarManager = nullptr;

    // UI widgets
    DataTableView* m_tableView = nullptr;
    FilterPanel* m_filterPanel = nullptr;
    SearchBar* m_searchBar = nullptr;
    ActiveFilterChipBar* m_chipBar = nullptr;
    ColumnProfilePanel* m_profilePanel = nullptr;
    ProgressOverlay* m_progressOverlay = nullptr;

    // Layout
    QSplitter* m_mainSplitter = nullptr;

    // State
    QString m_currentFilePath;
    QString m_currentTableName;
    QString m_sessionFilePath;
    bool m_editableMode = false;
    QStringList m_recentFiles;

    // Search state
    QVector<QPair<qint64, int>> m_searchResults;
    int m_currentSearchIndex = -1;

    // Setup
    void setupEngine();
    void setupUI();
    void setupCentralWidget();
    void setupConnections();
    void setupDragDrop();

    // Helpers
    void loadFileIntoEngine(const QString& filePath, const CSVImportOptions& options);
    void updateUIAfterLoad();
    void updateStatusBar();
    void updateWindowTitle();
    void addToRecentFiles(const QString& filePath);
    void saveSessionState(const QString& path);
    void loadSessionState(const QString& path);
    bool confirmUnsavedChanges();
    void fetchPageForRow(qint64 row);

    // Settings persistence
    void loadSettings();
    void saveSettings();
};

} // namespace csvforge
