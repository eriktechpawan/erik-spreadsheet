#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>
#include <vector>

#include "data/model/dataset_descriptor.h"
#include "data/model/tab_state.h"

namespace csvforge {

class DuckDBEngine;

/// Manages the collection of open dataset tabs.
class TabManager : public QObject {
    Q_OBJECT
public:
    explicit TabManager(DuckDBEngine* engine, QObject* parent = nullptr);
    ~TabManager() override;

    // --- Tab lifecycle ---

    /// Create a new tab from a loaded DuckDB table. Returns tab index.
    int createSourceTab(const QString& tableName, const QString& filePath);

    /// Create a derived tab (pivot, lookup, etc.). Returns tab index.
    int createDerivedTab(const DatasetDescriptor& desc);

    /// Close a tab by index. Returns true if closed.
    bool closeTab(int index);

    /// Duplicate a tab (copies state, not data). Returns new tab index.
    int duplicateTab(int index);

    // --- Access ---

    int tabCount() const;
    int activeTabIndex() const;
    void setActiveTab(int index);

    TabState* tabAt(int index) const;
    TabState* activeTab() const;
    TabState* tabById(const QString& datasetId) const;
    int indexOfId(const QString& datasetId) const;

    /// All dataset descriptors, e.g., for lookup source selection.
    QStringList openTableNames() const;
    std::vector<DatasetDescriptor> allDescriptors() const;

    // --- Rename ---
    void renameTab(int index, const QString& newName);

    // --- Serialization ---
    QJsonArray toJson() const;
    void fromJson(const QJsonArray& arr);

    /// Index of active tab stored for serialization.
    int savedActiveIndex() const;

signals:
    void tabCreated(int index);
    void tabClosed(int index);
    void activeTabChanged(int index);
    void tabRenamed(int index, const QString& name);
    void tabDirtyChanged(int index, bool dirty);

private:
    DuckDBEngine* m_engine;
    std::vector<std::unique_ptr<TabState>> m_tabs;
    int m_activeIndex = -1;

    QString generateUniqueId() const;
    void connectTab(int index);
};

} // namespace csvforge
