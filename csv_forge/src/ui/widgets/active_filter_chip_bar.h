#pragma once
#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVector>

namespace csvforge {

class FilterState;

class ActiveFilterChipBar : public QWidget {
    Q_OBJECT
public:
    explicit ActiveFilterChipBar(QWidget* parent = nullptr);
    void setFilterState(FilterState* state);
    void updateChips();

signals:
    void filterRemoved(int index);
    void allFiltersCleared();

private:
    FilterState* m_filterState = nullptr;
    QHBoxLayout* m_layout = nullptr;
    QVector<QPushButton*> m_chips;
    QPushButton* m_clearAllBtn = nullptr;

    void clearChips();
    void addChip(const QString& text, int index);
};

} // namespace csvforge
