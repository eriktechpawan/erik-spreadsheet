#pragma once

#include <QLabel>
#include <QObject>
#include <QStatusBar>

namespace csvforge {

class StatusBarManager : public QObject {
    Q_OBJECT
public:
    explicit StatusBarManager(QStatusBar* statusBar, QObject* parent = nullptr);

    void setFileName(const QString& name);
    void setTotalRows(qint64 count);
    void setFilteredRows(qint64 count);
    void setSelectedRows(int count);
    void setVisibleColumns(int count, int total);
    void setMode(bool editable);
    void setSortState(const QString& sortDescription);
    void setDirtyState(bool dirty);
    void setMessage(const QString& message, int timeoutMs = 3000);
    void clear();

private:
    QStatusBar* m_statusBar;
    QLabel* m_fileLabel = nullptr;
    QLabel* m_totalRowsLabel = nullptr;
    QLabel* m_filteredRowsLabel = nullptr;
    QLabel* m_selectedRowsLabel = nullptr;
    QLabel* m_visibleColsLabel = nullptr;
    QLabel* m_modeLabel = nullptr;
    QLabel* m_sortLabel = nullptr;
    QLabel* m_dirtyLabel = nullptr;

    void setupStatusBar();
    QLabel* createStatusLabel(const QString& text);
};

} // namespace csvforge
