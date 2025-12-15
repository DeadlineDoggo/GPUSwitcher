#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QIcon>
#include <QString>
class QMenu;
class QWidget;
class TrayPopup;

class GPUTray : public QObject
{
    Q_OBJECT
public:
    explicit GPUTray(QObject *parent = nullptr);
    ~GPUTray() override = default;

private slots:
    void updateStatus();
    void setIntegrated();
    void setHybrid();
    void setNvidia();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    // iconActivated removed — let Qt handle activation/menu natively

private:
    QIcon iconForMode(const QString &mode) const;
    QString currentModeFromEnvy() const;

    QSystemTrayIcon *m_tray = nullptr;
    QMenu *m_menu = nullptr;
    QWidget *m_menuParent = nullptr;
    TrayPopup *m_popup = nullptr;
};
