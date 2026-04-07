#include "data/model/sort_state.h"

#include <algorithm>

namespace csvforge {

SortState::SortState(QObject* parent)
    : QObject(parent)
{
}

const std::vector<SortColumn>& SortState::columns() const
{
    return m_columns;
}

void SortState::setSort(const QString& columnName, bool ascending)
{
    m_columns.clear();
    m_columns.push_back({columnName, ascending});
    emit sortChanged();
}

void SortState::addSort(const QString& columnName, bool ascending)
{
    // If column already in sort list, update direction
    auto it = std::find_if(m_columns.begin(), m_columns.end(),
                           [&columnName](const SortColumn& sc) {
                               return sc.columnName == columnName;
                           });
    if (it != m_columns.end()) {
        it->ascending = ascending;
    } else {
        m_columns.push_back({columnName, ascending});
    }
    emit sortChanged();
}

void SortState::removeSort(const QString& columnName)
{
    auto it = std::find_if(m_columns.begin(), m_columns.end(),
                           [&columnName](const SortColumn& sc) {
                               return sc.columnName == columnName;
                           });
    if (it != m_columns.end()) {
        m_columns.erase(it);
        emit sortChanged();
    }
}

void SortState::clearSort()
{
    if (!m_columns.empty()) {
        m_columns.clear();
        emit sortChanged();
    }
}

bool SortState::hasActiveSort() const
{
    return !m_columns.empty();
}

void SortState::toggleSort(const QString& columnName)
{
    auto it = std::find_if(m_columns.begin(), m_columns.end(),
                           [&columnName](const SortColumn& sc) {
                               return sc.columnName == columnName;
                           });
    if (it == m_columns.end()) {
        // Not sorted -> ascending
        m_columns.push_back({columnName, true});
    } else if (it->ascending) {
        // Ascending -> descending
        it->ascending = false;
    } else {
        // Descending -> remove (no sort)
        m_columns.erase(it);
    }
    emit sortChanged();
}

QString SortState::toOrderByClause() const
{
    if (m_columns.empty()) {
        return {};
    }

    QStringList parts;
    parts.reserve(static_cast<int>(m_columns.size()));
    for (const auto& col : m_columns) {
        parts.append(QStringLiteral("\"%1\" %2")
                         .arg(col.columnName, col.ascending ? QStringLiteral("ASC")
                                                            : QStringLiteral("DESC")));
    }
    return parts.join(QStringLiteral(", "));
}

QString SortState::toDisplayString() const
{
    if (m_columns.empty()) {
        return QStringLiteral("No sort");
    }

    QStringList parts;
    parts.reserve(static_cast<int>(m_columns.size()));
    for (const auto& col : m_columns) {
        const QChar arrow = col.ascending ? QChar(0x25B2) : QChar(0x25BC); // ▲ ▼
        parts.append(QStringLiteral("%1 %2").arg(col.columnName, arrow));
    }
    return parts.join(QStringLiteral(", "));
}

} // namespace csvforge
