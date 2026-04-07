#pragma once

#include <QObject>
#include <QString>
#include <vector>

namespace csvforge {

struct SortColumn {
    QString columnName;
    bool ascending = true;
};

class SortState : public QObject {
    Q_OBJECT
public:
    explicit SortState(QObject* parent = nullptr);

    // Current sort
    const std::vector<SortColumn>& columns() const;
    void setSort(const QString& columnName, bool ascending = true);
    void addSort(const QString& columnName, bool ascending = true);
    void removeSort(const QString& columnName);
    void clearSort();
    bool hasActiveSort() const;

    // Toggle sort on a column (none -> asc -> desc -> none)
    void toggleSort(const QString& columnName);

    // Generate SQL ORDER BY clause
    QString toOrderByClause() const;

    // Display
    QString toDisplayString() const;

signals:
    void sortChanged();

private:
    std::vector<SortColumn> m_columns;
};

} // namespace csvforge
