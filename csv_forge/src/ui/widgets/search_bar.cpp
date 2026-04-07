#include "search_bar.h"
#include "utils/constants.h"

#include <QHBoxLayout>
#include <QOverload>

namespace csvforge {

SearchBar::SearchBar(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void SearchBar::setupUI()
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(4);

    auto* searchIcon = new QLabel(QStringLiteral("\xF0\x9F\x94\x8D"), this); // magnifying glass
    layout->addWidget(searchIcon);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search..."));
    m_searchEdit->setClearButtonEnabled(false);
    layout->addWidget(m_searchEdit, 1);

    m_columnSelector = new QComboBox(this);
    m_columnSelector->addItem(tr("All Columns"));
    layout->addWidget(m_columnSelector);

    m_caseSensitiveCheck = new QCheckBox(tr("Aa"), this);
    m_caseSensitiveCheck->setToolTip(tr("Case Sensitive"));
    layout->addWidget(m_caseSensitiveCheck);

    m_regexCheck = new QCheckBox(QStringLiteral(".*"), this);
    m_regexCheck->setToolTip(tr("Use Regular Expression"));
    layout->addWidget(m_regexCheck);

    m_prevBtn = new QPushButton(QStringLiteral("\u25C0"), this);
    m_prevBtn->setFixedWidth(28);
    m_prevBtn->setToolTip(tr("Previous Match"));
    layout->addWidget(m_prevBtn);

    m_nextBtn = new QPushButton(QStringLiteral("\u25B6"), this);
    m_nextBtn->setFixedWidth(28);
    m_nextBtn->setToolTip(tr("Next Match"));
    layout->addWidget(m_nextBtn);

    m_matchLabel = new QLabel(QStringLiteral("0/0"), this);
    m_matchLabel->setMinimumWidth(50);
    m_matchLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_matchLabel);

    m_clearBtn = new QPushButton(QStringLiteral("\u2715"), this);
    m_clearBtn->setFixedWidth(28);
    m_clearBtn->setToolTip(tr("Clear Search"));
    layout->addWidget(m_clearBtn);

    // Debounce timer
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(constants::SEARCH_DEBOUNCE_MS);
    connect(m_debounceTimer, &QTimer::timeout, this, &SearchBar::onSearchTextChanged);

    // Connections
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this]() {
        m_debounceTimer->start();
    });

    connect(m_nextBtn, &QPushButton::clicked, this, &SearchBar::nextMatch);
    connect(m_prevBtn, &QPushButton::clicked, this, &SearchBar::previousMatch);

    connect(m_clearBtn, &QPushButton::clicked, this, [this]() {
        m_searchEdit->clear();
        m_matchCount = 0;
        m_currentMatch = 0;
        updateMatchLabel();
        emit searchCleared();
    });

    connect(m_caseSensitiveCheck, &QCheckBox::toggled, this, [this]() {
        if (!m_searchEdit->text().isEmpty()) {
            m_debounceTimer->start();
        }
    });

    connect(m_regexCheck, &QCheckBox::toggled, this, [this]() {
        if (!m_searchEdit->text().isEmpty()) {
            m_debounceTimer->start();
        }
    });

    connect(m_columnSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        if (!m_searchEdit->text().isEmpty()) {
            m_debounceTimer->start();
        }
    });
}

void SearchBar::setColumns(const QStringList& columns)
{
    m_columnSelector->clear();
    m_columnSelector->addItem(tr("All Columns"));
    m_columnSelector->addItems(columns);
}

QString SearchBar::searchText() const
{
    return m_searchEdit->text();
}

QString SearchBar::searchColumn() const
{
    if (m_columnSelector->currentIndex() == 0) {
        return QString(); // "All Columns"
    }
    return m_columnSelector->currentText();
}

bool SearchBar::caseSensitive() const
{
    return m_caseSensitiveCheck->isChecked();
}

bool SearchBar::useRegex() const
{
    return m_regexCheck->isChecked();
}

int SearchBar::matchCount() const
{
    return m_matchCount;
}

int SearchBar::currentMatch() const
{
    return m_currentMatch;
}

void SearchBar::setMatchCount(int count)
{
    m_matchCount = count;
    updateMatchLabel();
}

void SearchBar::setCurrentMatch(int current)
{
    m_currentMatch = current;
    updateMatchLabel();
}

void SearchBar::clear()
{
    m_searchEdit->clear();
    m_matchCount = 0;
    m_currentMatch = 0;
    updateMatchLabel();
}

void SearchBar::focus()
{
    m_searchEdit->setFocus();
    m_searchEdit->selectAll();
}

void SearchBar::onSearchTextChanged()
{
    const QString text = m_searchEdit->text();
    if (text.isEmpty()) {
        m_matchCount = 0;
        m_currentMatch = 0;
        updateMatchLabel();
        emit searchCleared();
        return;
    }
    emit searchRequested(text, searchColumn(), caseSensitive(), useRegex());
}

void SearchBar::updateMatchLabel()
{
    if (m_matchCount == 0) {
        m_matchLabel->setText(QStringLiteral("0/0"));
    } else {
        m_matchLabel->setText(QStringLiteral("%1/%2")
                                 .arg(m_currentMatch)
                                 .arg(m_matchCount));
    }
}

} // namespace csvforge
