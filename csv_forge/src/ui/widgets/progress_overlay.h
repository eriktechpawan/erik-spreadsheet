#pragma once
#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>

namespace csvforge {

class ProgressOverlay : public QWidget {
    Q_OBJECT
public:
    explicit ProgressOverlay(QWidget* parent = nullptr);

    void showProgress(const QString& message);
    void setProgress(int percent, const QString& message);
    void hideProgress();
    bool isCancellable() const;
    void setCancellable(bool cancellable);

signals:
    void cancelled();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QLabel* m_messageLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QPushButton* m_cancelBtn = nullptr;
    bool m_cancellable = true;
    bool m_active = false;

    void setupUI();
};

} // namespace csvforge
