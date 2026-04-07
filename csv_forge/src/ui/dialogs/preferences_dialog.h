#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QTabWidget>

namespace csvforge {

// ---------------------------------------------------------------------------
// PreferencesDialog – application-wide settings persisted via QSettings
// ---------------------------------------------------------------------------

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget* parent = nullptr);
    ~PreferencesDialog() override = default;

    // General
    QString defaultDelimiter() const;
    QString defaultEncoding() const;
    int     maxRecentFiles() const;
    bool    autoDetectFormat() const;
    bool    ignoreImportErrors() const;

    // Display
    int  fontSize() const;
    bool alternatingRowColors() const;
    bool showGridLines() const;
    bool showRowNumbers() const;

    // Performance
    int rowCachePageSize() const;
    int cacheBufferPages() const;
    int maxDistinctValues() const;
    int filterDebounceMs() const;

signals:
    void applied();

private slots:
    void onApply();
    void onAccept();

private:
    void buildUI();
    QWidget* buildGeneralTab();
    QWidget* buildDisplayTab();
    QWidget* buildPerformanceTab();
    void loadSettings();
    void saveSettings();

    QTabWidget* m_tabs = nullptr;

    // General
    QComboBox* m_delimiterCombo   = nullptr;
    QComboBox* m_encodingCombo    = nullptr;
    QSpinBox*  m_maxRecentSpin    = nullptr;
    QCheckBox* m_autoDetectCheck  = nullptr;
    QCheckBox* m_ignoreErrorCheck = nullptr;

    // Display
    QSpinBox*  m_fontSizeSpin     = nullptr;
    QCheckBox* m_altRowColorCheck = nullptr;
    QCheckBox* m_gridLinesCheck   = nullptr;
    QCheckBox* m_rowNumberCheck   = nullptr;

    // Performance
    QSpinBox* m_pageSizeSpin      = nullptr;
    QSpinBox* m_cachePagesSpin    = nullptr;
    QSpinBox* m_maxDistinctSpin   = nullptr;
    QSpinBox* m_filterDebounceSpin = nullptr;

    QDialogButtonBox* m_buttonBox = nullptr;
};

} // namespace csvforge
