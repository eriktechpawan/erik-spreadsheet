#pragma once
#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QProgressBar>
#include "data/duckdb/stats_service.h"
#include "data/types/filter_rule.h"

namespace csvforge {

class ColumnProfilePanel : public QWidget {
    Q_OBJECT
public:
    explicit ColumnProfilePanel(QWidget* parent = nullptr);

    void showStats(const ColumnStats& stats);
    void clear();
    void setLoading(bool loading);

signals:
    void filterRequested(const QString& columnName, FilterOperator op, const QVariant& value);

private:
    QLabel* m_columnNameLabel = nullptr;
    QLabel* m_typeLabel = nullptr;
    QLabel* m_totalRowsLabel = nullptr;
    QLabel* m_filteredRowsLabel = nullptr;
    QLabel* m_nullCountLabel = nullptr;
    QLabel* m_uniqueCountLabel = nullptr;
    QLabel* m_minLabel = nullptr;
    QLabel* m_maxLabel = nullptr;
    QLabel* m_sumLabel = nullptr;
    QLabel* m_avgLabel = nullptr;
    QLabel* m_medianLabel = nullptr;
    QTableWidget* m_topValuesTable = nullptr;
    QLabel* m_selectionSumLabel = nullptr;
    QLabel* m_selectionAvgLabel = nullptr;
    QProgressBar* m_loadingBar = nullptr;

    void setupUI();
};

} // namespace csvforge
