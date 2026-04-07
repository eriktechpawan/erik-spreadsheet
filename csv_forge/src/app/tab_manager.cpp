#include "app/tab_manager.h"
#include "data/duckdb/duckdb_engine.h"
#include "utils/logging.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>

namespace csvforge {

TabManager::TabManager(DuckDBEngine* engine, QObject* parent)
    : QObject(parent)
    , m_engine(engine)
{
}

TabManager::~TabManager() = default;

// ---------------------------------------------------------------------------
// Tab creation
// ---------------------------------------------------------------------------

int TabManager::createSourceTab(const QString& tableName, const QString& filePath)
{
    DatasetDescriptor desc;
    desc.id = generateUniqueId();
    desc.displayName = tableName;
    desc.type = DatasetType::Source;
    desc.tableName = tableName;
    desc.sourceFilePath = filePath;

    if (m_engine && m_engine->isInitialized()) {
        desc.rowCount = m_engine->getRowCount(tableName);
        auto schema = m_engine->getTableSchema(tableName);
        desc.columnCount = static_cast<int>(schema.size());
    }

    auto tab = std::make_unique<TabState>(this);
    tab->setDescriptor(desc);

    if (m_engine && m_engine->isInitialized()) {
        tab->setSchema(m_engine->getTableSchema(tableName));
    }

    const int idx = static_cast<int>(m_tabs.size());
    m_tabs.push_back(std::move(tab));
    connectTab(idx);
    emit tabCreated(idx);

    // Auto-activate first tab
    if (m_activeIndex < 0) {
        setActiveTab(idx);
    }

    logMsg(LogCategory::Session, LogLevel::Info,
           QStringLiteral("Created source tab '%1' (table=%2)")
               .arg(desc.displayName, tableName));
    return idx;
}

int TabManager::createDerivedTab(const DatasetDescriptor& desc)
{
    auto tab = std::make_unique<TabState>(this);

    DatasetDescriptor d = desc;
    if (d.id.isEmpty()) {
        d.id = generateUniqueId();
    }
    tab->setDescriptor(d);

    if (m_engine && m_engine->isInitialized() && !d.tableName.isEmpty()) {
        tab->setSchema(m_engine->getTableSchema(d.tableName));
        d.rowCount = m_engine->getRowCount(d.tableName);
        d.columnCount = static_cast<int>(tab->schema().size());
        tab->setDescriptor(d);
    }

    const int idx = static_cast<int>(m_tabs.size());
    m_tabs.push_back(std::move(tab));
    connectTab(idx);
    emit tabCreated(idx);

    setActiveTab(idx);

    logMsg(LogCategory::Session, LogLevel::Info,
           QStringLiteral("Created derived tab '%1' [%2]")
               .arg(d.displayName, d.typeBadge()));
    return idx;
}

// ---------------------------------------------------------------------------
// Close / duplicate
// ---------------------------------------------------------------------------

bool TabManager::closeTab(int index)
{
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) return false;

    // Drop the DuckDB table for derived datasets
    const auto& desc = m_tabs[static_cast<size_t>(index)]->descriptor();
    if (desc.type != DatasetType::Source && m_engine && !desc.tableName.isEmpty()) {
        m_engine->dropTable(desc.tableName);
    }

    m_tabs.erase(m_tabs.begin() + index);
    emit tabClosed(index);

    // Adjust active index
    if (m_tabs.empty()) {
        m_activeIndex = -1;
        emit activeTabChanged(-1);
    } else if (m_activeIndex >= static_cast<int>(m_tabs.size())) {
        setActiveTab(static_cast<int>(m_tabs.size()) - 1);
    } else if (m_activeIndex == index) {
        const int newIdx = (index > 0) ? index - 1 : 0;
        m_activeIndex = -1; // force change
        setActiveTab(newIdx);
    }
    return true;
}

int TabManager::duplicateTab(int index)
{
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) return -1;

    const auto* src = m_tabs[static_cast<size_t>(index)].get();
    DatasetDescriptor desc = src->descriptor();
    desc.id = generateUniqueId();
    desc.displayName = desc.displayName + QStringLiteral(" (Copy)");

    // For a true duplicate, we would CREATE TABLE AS SELECT from the source.
    // For now, duplicate shares the same table (read-only view).
    auto tab = std::make_unique<TabState>(this);
    tab->setDescriptor(desc);
    tab->setSchema(src->schema());
    tab->setHiddenColumns(src->hiddenColumns());
    tab->setColumnOrder(src->columnOrder());

    const int idx = static_cast<int>(m_tabs.size());
    m_tabs.push_back(std::move(tab));
    connectTab(idx);
    emit tabCreated(idx);
    setActiveTab(idx);
    return idx;
}

// ---------------------------------------------------------------------------
// Access
// ---------------------------------------------------------------------------

int TabManager::tabCount() const
{
    return static_cast<int>(m_tabs.size());
}

int TabManager::activeTabIndex() const { return m_activeIndex; }

void TabManager::setActiveTab(int index)
{
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) return;
    if (m_activeIndex == index) return;
    m_activeIndex = index;
    emit activeTabChanged(index);
}

TabState* TabManager::tabAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) return nullptr;
    return m_tabs[static_cast<size_t>(index)].get();
}

TabState* TabManager::activeTab() const
{
    return tabAt(m_activeIndex);
}

TabState* TabManager::tabById(const QString& datasetId) const
{
    for (const auto& tab : m_tabs) {
        if (tab->descriptor().id == datasetId) return tab.get();
    }
    return nullptr;
}

int TabManager::indexOfId(const QString& datasetId) const
{
    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
        if (m_tabs[static_cast<size_t>(i)]->descriptor().id == datasetId) return i;
    }
    return -1;
}

QStringList TabManager::openTableNames() const
{
    QStringList names;
    for (const auto& tab : m_tabs) {
        if (!tab->descriptor().tableName.isEmpty()) {
            names << tab->descriptor().tableName;
        }
    }
    return names;
}

std::vector<DatasetDescriptor> TabManager::allDescriptors() const
{
    std::vector<DatasetDescriptor> result;
    result.reserve(m_tabs.size());
    for (const auto& tab : m_tabs) {
        result.push_back(tab->descriptor());
    }
    return result;
}

// ---------------------------------------------------------------------------
// Rename
// ---------------------------------------------------------------------------

void TabManager::renameTab(int index, const QString& newName)
{
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) return;
    auto desc = m_tabs[static_cast<size_t>(index)]->descriptor();
    desc.displayName = newName;
    m_tabs[static_cast<size_t>(index)]->setDescriptor(desc);
    emit tabRenamed(index, newName);
}

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------

QJsonArray TabManager::toJson() const
{
    QJsonArray arr;
    for (const auto& tab : m_tabs) {
        arr.append(tab->toJson());
    }
    return arr;
}

void TabManager::fromJson(const QJsonArray& arr)
{
    // Clear existing
    while (!m_tabs.empty()) {
        closeTab(static_cast<int>(m_tabs.size()) - 1);
    }

    for (const auto& v : arr) {
        auto tab = TabState::fromJson(v.toObject(), this);
        const int idx = static_cast<int>(m_tabs.size());
        m_tabs.push_back(std::move(tab));
        connectTab(idx);
        emit tabCreated(idx);
    }

    if (!m_tabs.empty()) {
        setActiveTab(0);
    }
}

int TabManager::savedActiveIndex() const { return m_activeIndex; }

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

QString TabManager::generateUniqueId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void TabManager::connectTab(int index)
{
    auto* tab = m_tabs[static_cast<size_t>(index)].get();
    connect(tab, &TabState::dirtyChanged, this, [this, index](bool dirty) {
        emit tabDirtyChanged(index, dirty);
    });
}

} // namespace csvforge
