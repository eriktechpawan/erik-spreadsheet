#pragma once

#include "data/duckdb/stats_service.h"
#include "data/types/column_schema.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>

namespace csvforge {

// ---------------------------------------------------------------------------
// ColumnStatsDialog – read-only display of column statistics
// ---------------------------------------------------------------------------

class ColumnStatsDialog : public QDialog {
    Q_OBJECT

public:
    explicit ColumnStatsDialog(const ColumnStats& stats,
                               QWidget* parent = nullptr);
    ~ColumnStatsDialog() override = default;

private slots:
    void onCopyStats();

private:
    void buildUI();
    QGroupBox* buildSummarySection();
    QGroupBox* buildTopValuesSection();
    QGroupBox* buildSelectionSection();
    QString statsAsText() const;

    ColumnStats m_stats;

    QLabel*       m_columnNameLabel = nullptr;
    QLabel*       m_typeLabel       = nullptr;
    QTableWidget* m_topValuesTable  = nullptr;
    QGroupBox*    m_selectionGroup  = nullptr;

    // Summary labels
    QLabel* m_totalRowsLabel    = nullptr;
    QLabel* m_filteredRowsLabel = nullptr;
    QLabel* m_nullCountLabel    = nullptr;
    QLabel* m_nonNullCountLabel = nullptr;
    QLabel* m_uniqueCountLabel  = nullptr;
    QLabel* m_minLabel          = nullptr;
    QLabel* m_maxLabel          = nullptr;
    QLabel* m_sumLabel          = nullptr;
    QLabel* m_averageLabel      = nullptr;
    QLabel* m_medianLabel       = nullptr;

    // Selection labels
    QLabel* m_selCountLabel = nullptr;
    QLabel* m_selSumLabel   = nullptr;
    QLabel* m_selAvgLabel   = nullptr;

    QPushButton*      m_copyBtn   = nullptr;
    QDialogButtonBox* m_buttonBox = nullptr;
};

} // namespace csvforge
