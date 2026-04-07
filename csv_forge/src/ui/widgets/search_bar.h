#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QTimer>

namespace csvforge {

class SearchBar : public QWidget {
    Q_OBJECT
public:
    explicit SearchBar(QWidget* parent = nullptr);

    void setColumns(const QStringList& columns);
    QString searchText() const;
    QString searchColumn() const;
    bool caseSensitive() const;
    bool useRegex() const;
    int matchCount() const;
    int currentMatch() const;
    void setMatchCount(int count);
    void setCurrentMatch(int current);
    void clear();
    void focus();

signals:
    void searchRequested(const QString& text, const QString& column, bool caseSensitive, bool regex);
    void nextMatch();
    void previousMatch();
    void searchCleared();

private:
    QLineEdit* m_searchEdit = nullptr;
    QComboBox* m_columnSelector = nullptr;
    QCheckBox* m_caseSensitiveCheck = nullptr;
    QCheckBox* m_regexCheck = nullptr;
    QPushButton* m_nextBtn = nullptr;
    QPushButton* m_prevBtn = nullptr;
    QPushButton* m_clearBtn = nullptr;
    QLabel* m_matchLabel = nullptr;
    QTimer* m_debounceTimer = nullptr;
    int m_matchCount = 0;
    int m_currentMatch = 0;

    void setupUI();
    void onSearchTextChanged();
    void updateMatchLabel();
};

} // namespace csvforge
