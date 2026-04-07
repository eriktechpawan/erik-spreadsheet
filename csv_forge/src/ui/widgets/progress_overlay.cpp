#include "progress_overlay.h"

#include <QFrame>
#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>

namespace csvforge {

ProgressOverlay::ProgressOverlay(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    hide();
}

void ProgressOverlay::setupUI()
{
    setAttribute(Qt::WA_TransparentForMouseEvents, false);

    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setAlignment(Qt::AlignCenter);

    auto* centerFrame = new QFrame(this);
    centerFrame->setFixedSize(320, 140);
    centerFrame->setStyleSheet(QStringLiteral(
        "QFrame {"
        "  background-color: #ffffff;"
        "  border: 1px solid #ccc;"
        "  border-radius: 8px;"
        "}"));

    auto* frameLayout = new QVBoxLayout(centerFrame);
    frameLayout->setContentsMargins(20, 16, 20, 16);
    frameLayout->setSpacing(12);

    m_messageLabel = new QLabel(this);
    QFont msgFont = m_messageLabel->font();
    msgFont.setBold(true);
    m_messageLabel->setFont(msgFont);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setWordWrap(true);
    frameLayout->addWidget(m_messageLabel);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    frameLayout->addWidget(m_progressBar);

    m_cancelBtn = new QPushButton(tr("Cancel"), this);
    m_cancelBtn->setFixedWidth(80);
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addStretch();
    frameLayout->addLayout(btnLayout);

    outerLayout->addWidget(centerFrame);

    connect(m_cancelBtn, &QPushButton::clicked, this, &ProgressOverlay::cancelled);
}

void ProgressOverlay::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    if (m_active) {
        QPainter painter(this);
        painter.fillRect(rect(), QColor(0, 0, 0, 128));
    }
}

void ProgressOverlay::showProgress(const QString& message)
{
    m_active = true;
    m_messageLabel->setText(message);
    m_progressBar->setValue(0);
    if (parentWidget()) {
        resize(parentWidget()->size());
    }
    show();
    raise();
}

void ProgressOverlay::setProgress(int percent, const QString& message)
{
    m_progressBar->setValue(percent);
    if (!message.isEmpty()) {
        m_messageLabel->setText(message);
    }
}

void ProgressOverlay::hideProgress()
{
    m_active = false;
    hide();
}

bool ProgressOverlay::isCancellable() const
{
    return m_cancellable;
}

void ProgressOverlay::setCancellable(bool cancellable)
{
    m_cancellable = cancellable;
    m_cancelBtn->setVisible(cancellable);
}

} // namespace csvforge
