#include "ui/dialogs/preferences_dialog.h"

#include <QFormLayout>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

namespace csvforge {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

PreferencesDialog::PreferencesDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Preferences"));
    resize(500, 400);
    buildUI();
    loadSettings();
}

// ---------------------------------------------------------------------------
// Public getters
// ---------------------------------------------------------------------------

QString PreferencesDialog::defaultDelimiter() const
{
    return m_delimiterCombo->currentData().toString();
}

QString PreferencesDialog::defaultEncoding() const
{
    return m_encodingCombo->currentData().toString();
}

int PreferencesDialog::maxRecentFiles() const { return m_maxRecentSpin->value(); }
bool PreferencesDialog::autoDetectFormat() const { return m_autoDetectCheck->isChecked(); }
bool PreferencesDialog::ignoreImportErrors() const { return m_ignoreErrorCheck->isChecked(); }

int PreferencesDialog::fontSize() const { return m_fontSizeSpin->value(); }
bool PreferencesDialog::alternatingRowColors() const { return m_altRowColorCheck->isChecked(); }
bool PreferencesDialog::showGridLines() const { return m_gridLinesCheck->isChecked(); }
bool PreferencesDialog::showRowNumbers() const { return m_rowNumberCheck->isChecked(); }

int PreferencesDialog::rowCachePageSize() const { return m_pageSizeSpin->value(); }
int PreferencesDialog::cacheBufferPages() const { return m_cachePagesSpin->value(); }
int PreferencesDialog::maxDistinctValues() const { return m_maxDistinctSpin->value(); }
int PreferencesDialog::filterDebounceMs() const { return m_filterDebounceSpin->value(); }

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void PreferencesDialog::onApply()
{
    saveSettings();
    emit applied();
}

void PreferencesDialog::onAccept()
{
    saveSettings();
    accept();
}

// ---------------------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------------------

void PreferencesDialog::buildUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    m_tabs = new QTabWidget(this);
    m_tabs->addTab(buildGeneralTab(),     QStringLiteral("General"));
    m_tabs->addTab(buildDisplayTab(),     QStringLiteral("Display"));
    m_tabs->addTab(buildPerformanceTab(), QStringLiteral("Performance"));
    mainLayout->addWidget(m_tabs, /*stretch=*/1);

    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply,
        this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &PreferencesDialog::onAccept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &PreferencesDialog::onApply);
    mainLayout->addWidget(m_buttonBox);
}

QWidget* PreferencesDialog::buildGeneralTab()
{
    auto* widget = new QWidget(this);
    auto* layout = new QFormLayout(widget);

    m_delimiterCombo = new QComboBox(widget);
    m_delimiterCombo->addItem(QStringLiteral("Comma (,)"),   QStringLiteral(","));
    m_delimiterCombo->addItem(QStringLiteral("Tab (\\t)"),   QStringLiteral("\t"));
    m_delimiterCombo->addItem(QStringLiteral("Semicolon (;)"), QStringLiteral(";"));
    m_delimiterCombo->addItem(QStringLiteral("Pipe (|)"),    QStringLiteral("|"));
    layout->addRow(QStringLiteral("Default Delimiter:"), m_delimiterCombo);

    m_encodingCombo = new QComboBox(widget);
    m_encodingCombo->addItem(QStringLiteral("UTF-8"),   QStringLiteral("utf-8"));
    m_encodingCombo->addItem(QStringLiteral("Latin-1"), QStringLiteral("latin-1"));
    m_encodingCombo->addItem(QStringLiteral("ASCII"),   QStringLiteral("ascii"));
    layout->addRow(QStringLiteral("Default Encoding:"), m_encodingCombo);

    m_maxRecentSpin = new QSpinBox(widget);
    m_maxRecentSpin->setRange(5, 50);
    m_maxRecentSpin->setValue(20);
    layout->addRow(QStringLiteral("Max Recent Files:"), m_maxRecentSpin);

    m_autoDetectCheck = new QCheckBox(QStringLiteral("Auto-detect format"), widget);
    m_autoDetectCheck->setChecked(true);
    layout->addRow(QString(), m_autoDetectCheck);

    m_ignoreErrorCheck = new QCheckBox(QStringLiteral("Ignore errors on import"), widget);
    layout->addRow(QString(), m_ignoreErrorCheck);

    return widget;
}

QWidget* PreferencesDialog::buildDisplayTab()
{
    auto* widget = new QWidget(this);
    auto* layout = new QFormLayout(widget);

    m_fontSizeSpin = new QSpinBox(widget);
    m_fontSizeSpin->setRange(8, 24);
    m_fontSizeSpin->setValue(13);
    layout->addRow(QStringLiteral("Font Size:"), m_fontSizeSpin);

    m_altRowColorCheck = new QCheckBox(QStringLiteral("Show alternating row colors"), widget);
    m_altRowColorCheck->setChecked(true);
    layout->addRow(QString(), m_altRowColorCheck);

    m_gridLinesCheck = new QCheckBox(QStringLiteral("Show grid lines"), widget);
    m_gridLinesCheck->setChecked(true);
    layout->addRow(QString(), m_gridLinesCheck);

    m_rowNumberCheck = new QCheckBox(QStringLiteral("Row number column"), widget);
    m_rowNumberCheck->setChecked(true);
    layout->addRow(QString(), m_rowNumberCheck);

    return widget;
}

QWidget* PreferencesDialog::buildPerformanceTab()
{
    auto* widget = new QWidget(this);
    auto* layout = new QFormLayout(widget);

    m_pageSizeSpin = new QSpinBox(widget);
    m_pageSizeSpin->setRange(1000, 100000);
    m_pageSizeSpin->setSingleStep(1000);
    m_pageSizeSpin->setValue(10000);
    layout->addRow(QStringLiteral("Row Cache Page Size:"), m_pageSizeSpin);

    m_cachePagesSpin = new QSpinBox(widget);
    m_cachePagesSpin->setRange(1, 10);
    m_cachePagesSpin->setValue(3);
    layout->addRow(QStringLiteral("Cache Buffer Pages:"), m_cachePagesSpin);

    m_maxDistinctSpin = new QSpinBox(widget);
    m_maxDistinctSpin->setRange(100, 50000);
    m_maxDistinctSpin->setSingleStep(100);
    m_maxDistinctSpin->setValue(5000);
    layout->addRow(QStringLiteral("Max Distinct Values:"), m_maxDistinctSpin);

    m_filterDebounceSpin = new QSpinBox(widget);
    m_filterDebounceSpin->setRange(100, 2000);
    m_filterDebounceSpin->setSingleStep(50);
    m_filterDebounceSpin->setValue(300);
    m_filterDebounceSpin->setSuffix(QStringLiteral(" ms"));
    layout->addRow(QStringLiteral("Filter Debounce:"), m_filterDebounceSpin);

    return widget;
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

void PreferencesDialog::loadSettings()
{
    QSettings s;
    s.beginGroup(QStringLiteral("Preferences"));

    // General
    const QString delim = s.value(QStringLiteral("delimiter"), QStringLiteral(",")).toString();
    for (int i = 0; i < m_delimiterCombo->count(); ++i) {
        if (m_delimiterCombo->itemData(i).toString() == delim) {
            m_delimiterCombo->setCurrentIndex(i);
            break;
        }
    }

    const QString enc = s.value(QStringLiteral("encoding"), QStringLiteral("utf-8")).toString();
    for (int i = 0; i < m_encodingCombo->count(); ++i) {
        if (m_encodingCombo->itemData(i).toString() == enc) {
            m_encodingCombo->setCurrentIndex(i);
            break;
        }
    }

    m_maxRecentSpin->setValue(s.value(QStringLiteral("maxRecent"), 20).toInt());
    m_autoDetectCheck->setChecked(s.value(QStringLiteral("autoDetect"), true).toBool());
    m_ignoreErrorCheck->setChecked(s.value(QStringLiteral("ignoreErrors"), false).toBool());

    // Display
    m_fontSizeSpin->setValue(s.value(QStringLiteral("fontSize"), 13).toInt());
    m_altRowColorCheck->setChecked(s.value(QStringLiteral("altRowColors"), true).toBool());
    m_gridLinesCheck->setChecked(s.value(QStringLiteral("gridLines"), true).toBool());
    m_rowNumberCheck->setChecked(s.value(QStringLiteral("rowNumbers"), true).toBool());

    // Performance
    m_pageSizeSpin->setValue(s.value(QStringLiteral("pageSize"), 10000).toInt());
    m_cachePagesSpin->setValue(s.value(QStringLiteral("cachePages"), 3).toInt());
    m_maxDistinctSpin->setValue(s.value(QStringLiteral("maxDistinct"), 5000).toInt());
    m_filterDebounceSpin->setValue(s.value(QStringLiteral("filterDebounce"), 300).toInt());

    s.endGroup();
}

void PreferencesDialog::saveSettings()
{
    QSettings s;
    s.beginGroup(QStringLiteral("Preferences"));

    s.setValue(QStringLiteral("delimiter"),      m_delimiterCombo->currentData().toString());
    s.setValue(QStringLiteral("encoding"),        m_encodingCombo->currentData().toString());
    s.setValue(QStringLiteral("maxRecent"),       m_maxRecentSpin->value());
    s.setValue(QStringLiteral("autoDetect"),      m_autoDetectCheck->isChecked());
    s.setValue(QStringLiteral("ignoreErrors"),    m_ignoreErrorCheck->isChecked());

    s.setValue(QStringLiteral("fontSize"),        m_fontSizeSpin->value());
    s.setValue(QStringLiteral("altRowColors"),    m_altRowColorCheck->isChecked());
    s.setValue(QStringLiteral("gridLines"),       m_gridLinesCheck->isChecked());
    s.setValue(QStringLiteral("rowNumbers"),      m_rowNumberCheck->isChecked());

    s.setValue(QStringLiteral("pageSize"),        m_pageSizeSpin->value());
    s.setValue(QStringLiteral("cachePages"),      m_cachePagesSpin->value());
    s.setValue(QStringLiteral("maxDistinct"),     m_maxDistinctSpin->value());
    s.setValue(QStringLiteral("filterDebounce"),  m_filterDebounceSpin->value());

    s.endGroup();
}

} // namespace csvforge
