#include "active_filter_chip_bar.h"
#include "data/model/filter_state.h"

namespace csvforge {

ActiveFilterChipBar::ActiveFilterChipBar(QWidget* parent)
    : QWidget(parent)
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(4, 2, 4, 2);
    m_layout->setSpacing(4);

    m_clearAllBtn = new QPushButton(tr("Clear All"), this);
    m_clearAllBtn->setStyleSheet(
        QStringLiteral("QPushButton { color: #cc0000; border: none; font-weight: bold; }"));
    m_clearAllBtn->setVisible(false);
    m_layout->addWidget(m_clearAllBtn);

    m_layout->addStretch();

    connect(m_clearAllBtn, &QPushButton::clicked, this, [this]() {
        emit allFiltersCleared();
    });
}

void ActiveFilterChipBar::setFilterState(FilterState* state)
{
    m_filterState = state;
    if (m_filterState) {
        connect(m_filterState, &FilterState::filterChanged,
                this, &ActiveFilterChipBar::updateChips);
        updateChips();
    }
}

void ActiveFilterChipBar::updateChips()
{
    clearChips();

    if (!m_filterState || !m_filterState->hasActiveFilter()) {
        m_clearAllBtn->setVisible(false);
        setVisible(false);
        return;
    }

    const auto& filter = m_filterState->currentFilter();
    for (int i = 0; i < filter.rules.size(); ++i) {
        const auto& rule = filter.rules[i];
        addChip(rule.toDisplayString(), i);
    }

    m_clearAllBtn->setVisible(true);
    setVisible(true);
}

void ActiveFilterChipBar::clearChips()
{
    for (auto* chip : m_chips) {
        m_layout->removeWidget(chip);
        delete chip;
    }
    m_chips.clear();
}

void ActiveFilterChipBar::addChip(const QString& text, int index)
{
    auto* chip = new QPushButton(text + QStringLiteral(" \u2715"), this);
    chip->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: #e0e7ff;"
        "  border: 1px solid #93a3d4;"
        "  border-radius: 10px;"
        "  padding: 2px 8px;"
        "  font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #c7d2fe;"
        "}"));

    connect(chip, &QPushButton::clicked, this, [this, index]() {
        emit filterRemoved(index);
    });

    // Insert after the clear button (before the stretch)
    int insertPos = m_layout->count() - 1; // before stretch
    if (m_clearAllBtn->isVisible()) {
        insertPos = m_layout->indexOf(m_clearAllBtn) + 1;
    }
    m_layout->insertWidget(insertPos, chip);
    m_chips.append(chip);
}

} // namespace csvforge
