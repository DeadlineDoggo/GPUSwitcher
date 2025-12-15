#pragma once

#include <QWidget>

class QLabel;

class TrayPopup : public QWidget
{
    Q_OBJECT
public:
    explicit TrayPopup(QWidget *parent = nullptr);
    void setStatusText(const QString &text);
    void showAt(const QPoint &globalPos);

signals:
    void integratedRequested();
    void hybridRequested();
    void nvidiaRequested();
    void quitRequested();

private:
    QLabel *m_label = nullptr;
};
