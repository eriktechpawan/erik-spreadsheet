#include "filter_panel.h"
#include "data/model/filter_state.h"
#include "data/duckdb/duckdb_engine.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QVBoxLayout>

namespace csvforge {

FilterPanel::FilterPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void FilterPanel::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // Title
    auto* titleLabel = new QLabel(tr("Filter"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // Filter rule form
    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(4);

    m_columnSelector = new QComboBox(this);
    formLayout->addRow(tr("Column:"), m_columnSelector);

    m_operatorSelector = new QComboBox(this);
    formLayout->addRow(tr("Operator:"), m_operatorSelector);

    m_valueEdit = new QLineEdit(this);
    m_valueEdit->setPlaceholderText(tr("Value..."));
    formLayout->addRow(tr("Value:"), m_valueEdit);

    m_secondValueEdit = new QLineEdit(this);
    m_secondValueEdit->setPlaceholderText(tr("Second value (for Between)..."));
    m_secondValueEdit->setVisible(false);
    formLayout->addRow(tr("Value 2:"), m_secondValueEdit);

    mainLayout->addLayout(formLayout);

    // Combination selector
    auto* combLayout = new QHBoxLayout();
    auto* combLabel = new QLabel(tr("Combine:"), this);
    m_combinationSelector = new QComboBox(this);
    m_combinationSelector->addItem(tr("AND"), static_cast<int>(FilterCombination::And));
    m_combinationSelector->addItem(tr("OR"), static_cast<int>(FilterCombination::Or));
    combLayout->addWidget(combLabel);
    combLayout->addWidget(m_combinationSelector);
    mainLayout->addLayout(combLayout);

    // Action buttons
    auto* btnLayout = new QHBoxLayout();
    m_addFilterBtn = new QPushButton(tr("Add Filter"), this);
    m_clearBtn = new QPushButton(tr("Clear"), this);
    btnLayout->addWidget(m_addFilterBtn);
    btnLayout->addWidget(m_clearBtn);
    mainLayout->addLayout(btnLayout);

    // Active filters list
    auto* activeLabel = new QLabel(tr("Active Filters"), this);
    QFont activeFont = activeLabel->font();
    activeFont.setBold(true);
    activeLabel->setFont(activeFont);
    mainLayout->addWidget(activeLabel);

    m_activeFiltersList = new QListWidget(this);
    m_activeFiltersList->setMaximumHeight(120);
    mainLayout->addWidget(m_activeFiltersList);

    // Presets section
    auto* presetLayout = new QHBoxLayout();
    m_presetSelector = new QComboBox(this);
    m_presetSelector->setPlaceholderText(tr("Load preset..."));
    m_savePresetBtn = new QPushButton(tr("Save"), this);
    presetLayout->addWidget(m_presetSelector, 1);
    presetLayout->addWidget(m_savePresetBtn);
    mainLayout->addLayout(presetLayout);

    // Quick filters
    m_quickFilterGroup = new QGroupBox(tr("Quick Filters"), this);
    auto* quickLayout = new QVBoxLayout(m_quickFilterGroup);
    quickLayout->setSpacing(2);

    auto* row1 = new QHBoxLayout();
    auto* hideZerosBtn = new QPushButton(tr("Hide Zeros"), this);
    auto* showZerosBtn = new QPushButton(tr("Show Zeros"), this);
    row1->addWidget(hideZerosBtn);
    row1->addWidget(showZerosBtn);
    quickLayout->addLayout(row1);

    auto* row2 = new QHBoxLayout();
    auto* hideBlanksBtn = new QPushButton(tr("Hide Blanks"), this);
    auto* showBlanksBtn = new QPushButton(tr("Show Blanks"), this);
    row2->addWidget(hideBlanksBtn);
    row2->addWidget(showBlanksBtn);
    quickLayout->addLayout(row2);

    auto* row3 = new QHBoxLayout();
    auto* aboveAvgBtn = new QPushButton(tr("Above Average"), this);
    auto* belowAvgBtn = new QPushButton(tr("Below Average"), this);
    row3->addWidget(aboveAvgBtn);
    row3->addWidget(belowAvgBtn);
    quickLayout->addLayout(row3);

    mainLayout->addWidget(m_quickFilterGroup);
    mainLayout->addStretch();

    // Connections
    connect(m_addFilterBtn, &QPushButton::clicked, this, &FilterPanel::onAddFilter);
    connect(m_clearBtn, &QPushButton::clicked, this, &FilterPanel::onClearFilters);
    connect(m_savePresetBtn, &QPushButton::clicked, this, &FilterPanel::onSavePreset);
    connect(m_presetSelector, &QComboBox::currentTextChanged, this, &FilterPanel::onLoadPreset);

    connect(m_columnSelector, &QComboBox::currentTextChanged, this, [this](const QString& colName) {
        for (const auto& col : m_schema) {
            if (col.name == colName) {
                populateOperators(col.type);
                break;
            }
        }
    });

    connect(m_operatorSelector, &QComboBox::currentIndexChanged, this, [this]() {
        FilterOperator op = selectedOperator();
        m_secondValueEdit->setVisible(op == FilterOperator::Between);
    });

    // Quick filter connections
    connect(hideZerosBtn, &QPushButton::clicked, this, [this]() {
        if (m_filterState) {
            m_filterState->applyQuickFilter(FilterOperator::NonZero);
        }
    });
    connect(showZerosBtn, &QPushButton::clicked, this, [this]() {
        if (m_filterState) {
            m_filterState->applyQuickFilter(FilterOperator::IsZero);
        }
    });
    connect(hideBlanksBtn, &QPushButton::clicked, this, [this]() {
        if (m_filterState) {
            m_filterState->applyQuickFilter(FilterOperator::IsNotEmpty);
        }
    });
    connect(showBlanksBtn, &QPushButton::clicked, this, [this]() {
        if (m_filterState) {
            m_filterState->applyQuickFilter(FilterOperator::IsEmpty);
        }
    });
    connect(aboveAvgBtn, &QPushButton::clicked, this, [this]() {
        if (m_filterState) {
            m_filterState->applyQuickFilter(FilterOperator::GreaterThan);
        }
    });
    connect(belowAvgBtn, &QPushButton::clicked, this, [this]() {
        if (m_filterState) {
            m_filterState->applyQuickFilter(FilterOperator::LessThan);
        }
    });
}

void FilterPanel::setFilterState(FilterState* state)
{
    m_filterState = state;
    if (m_filterState) {
        connect(m_filterState, &FilterState::filterChanged, this, [this]() {
            updateActiveFiltersList();
            emit filterChanged(m_filterState->currentFilter());
        });
        updateActiveFiltersList();

        // Populate presets
        m_presetSelector->clear();
        m_presetSelector->addItem(QString()); // empty placeholder
        for (const auto& name : m_filterState->presetNames()) {
            m_presetSelector->addItem(name);
        }
    }
}

void FilterPanel::setEngine(DuckDBEngine* engine)
{
    m_engine = engine;
}

void FilterPanel::setTableName(const QString& tableName)
{
    m_tableName = tableName;
}

void FilterPanel::setSchema(const ColumnSchemaList& schema)
{
    m_schema = schema;
    m_columnSelector->clear();
    for (const auto& col : schema) {
        m_columnSelector->addItem(col.name);
    }
    if (!schema.isEmpty()) {
        populateOperators(schema.first().type);
    }
}

void FilterPanel::showQuickFilterForColumn(const QString& columnName, const QVariant& value)
{
    // Set column selector to the given column
    int idx = m_columnSelector->findText(columnName);
    if (idx >= 0) {
        m_columnSelector->setCurrentIndex(idx);
    }
    if (value.isValid()) {
        m_valueEdit->setText(value.toString());
    }
}

void FilterPanel::populateOperators(ColumnType type)
{
    m_operatorSelector->clear();

    if (type == ColumnType::Text || type == ColumnType::Varchar) {
        m_operatorSelector->addItem(tr("Equals"), static_cast<int>(FilterOperator::Equals));
        m_operatorSelector->addItem(tr("Not Equals"), static_cast<int>(FilterOperator::NotEquals));
        m_operatorSelector->addItem(tr("Contains"), static_cast<int>(FilterOperator::Contains));
        m_operatorSelector->addItem(tr("Not Contains"), static_cast<int>(FilterOperator::NotContains));
        m_operatorSelector->addItem(tr("Starts With"), static_cast<int>(FilterOperator::StartsWith));
        m_operatorSelector->addItem(tr("Ends With"), static_cast<int>(FilterOperator::EndsWith));
        m_operatorSelector->addItem(tr("Is Empty"), static_cast<int>(FilterOperator::IsEmpty));
        m_operatorSelector->addItem(tr("Is Not Empty"), static_cast<int>(FilterOperator::IsNotEmpty));
        m_operatorSelector->addItem(tr("Is Null"), static_cast<int>(FilterOperator::IsNull));
        m_operatorSelector->addItem(tr("Is Not Null"), static_cast<int>(FilterOperator::IsNotNull));
    } else {
        // Numeric types (Integer, Double, BigInt, etc.)
        m_operatorSelector->addItem(tr("Equals"), static_cast<int>(FilterOperator::Equals));
        m_operatorSelector->addItem(tr("Not Equals"), static_cast<int>(FilterOperator::NotEquals));
        m_operatorSelector->addItem(tr("Greater Than"), static_cast<int>(FilterOperator::GreaterThan));
        m_operatorSelector->addItem(tr("Greater or Equal"), static_cast<int>(FilterOperator::GreaterEqual));
        m_operatorSelector->addItem(tr("Less Than"), static_cast<int>(FilterOperator::LessThan));
        m_operatorSelector->addItem(tr("Less or Equal"), static_cast<int>(FilterOperator::LessEqual));
        m_operatorSelector->addItem(tr("Between"), static_cast<int>(FilterOperator::Between));
        m_operatorSelector->addItem(tr("Is Zero"), static_cast<int>(FilterOperator::IsZero));
        m_operatorSelector->addItem(tr("Non-Zero"), static_cast<int>(FilterOperator::NonZero));
        m_operatorSelector->addItem(tr("Is Positive"), static_cast<int>(FilterOperator::IsPositive));
        m_operatorSelector->addItem(tr("Is Negative"), static_cast<int>(FilterOperator::IsNegative));
        m_operatorSelector->addItem(tr("Is Null"), static_cast<int>(FilterOperator::IsNull));
        m_operatorSelector->addItem(tr("Is Not Null"), static_cast<int>(FilterOperator::IsNotNull));
    }
}

void FilterPanel::onAddFilter()
{
    if (!m_filterState || m_columnSelector->currentText().isEmpty()) {
        return;
    }

    FilterRule rule;
    rule.columnName = m_columnSelector->currentText();
    rule.op = selectedOperator();
    rule.value = m_valueEdit->text();
    if (rule.op == FilterOperator::Between) {
        rule.secondValue = m_secondValueEdit->text();
    }

    m_filterState->addRule(rule);

    // Update combination
    auto combination = static_cast<FilterCombination>(
        m_combinationSelector->currentData().toInt());
    m_filterState->setCombination(combination);
}

void FilterPanel::onRemoveFilter()
{
    if (!m_filterState) {
        return;
    }
    int row = m_activeFiltersList->currentRow();
    if (row >= 0) {
        m_filterState->removeRule(row);
    }
}

void FilterPanel::onClearFilters()
{
    if (m_filterState) {
        m_filterState->clearFilter();
    }
    m_valueEdit->clear();
    m_secondValueEdit->clear();
    emit filterCleared();
}

void FilterPanel::onSavePreset()
{
    if (!m_filterState) {
        return;
    }
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Save Preset"),
                                         tr("Preset name:"), QLineEdit::Normal,
                                         QString(), &ok);
    if (ok && !name.isEmpty()) {
        m_filterState->savePreset(name);
        if (m_presetSelector->findText(name) < 0) {
            m_presetSelector->addItem(name);
        }
    }
}

void FilterPanel::onLoadPreset(const QString& name)
{
    if (!m_filterState || name.isEmpty()) {
        return;
    }
    m_filterState->loadPreset(name);
}

void FilterPanel::updateActiveFiltersList()
{
    m_activeFiltersList->clear();
    if (!m_filterState) {
        return;
    }

    const auto& filter = m_filterState->currentFilter();
    for (const auto& rule : filter.rules) {
        QString display = QStringLiteral("%1 %2 %3")
                              .arg(rule.columnName,
                                   QString::number(static_cast<int>(rule.op)),
                                   rule.value.toString());
        m_activeFiltersList->addItem(display);
    }
}

FilterOperator FilterPanel::selectedOperator() const
{
    return static_cast<FilterOperator>(
        m_operatorSelector->currentData().toInt());
}

} // namespace csvforge
