#include "gpu_tray.h"

#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QCursor>
#include <QProcess>
#include <QTimer>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QCoreApplication>
#include <QWidget>
#include "tray_popup.h"

GPUTray::GPUTray(QObject *parent)
    : QObject(parent)
{
    m_tray = new QSystemTrayIcon(this);

    // Create an invisible widget to serve as the menu's parent on Wayland
    // This helps with the transient parent requirement. Use 1x1 so it's not 0x0.
    m_menuParent = new QWidget(nullptr, Qt::Tool | Qt::FramelessWindowHint);
    m_menuParent->setAttribute(Qt::WA_TranslucentBackground);
    m_menuParent->setFixedSize(1, 1);

    m_menu = new QMenu(m_menuParent);

    QAction *statusAct = new QAction("Current GPU: unknown", m_menu);
    statusAct->setDisabled(true);
    m_menu->addAction(statusAct);
    m_menu->addSeparator();

    QMenu *switchMenu = m_menu->addMenu("Switch GPU");
    QAction *intAct = switchMenu->addAction("Integrated");
    QAction *hybAct = switchMenu->addAction("Hybrid");
    QAction *nvdAct = switchMenu->addAction("Nvidia");

    m_menu->addSeparator();
    QAction *quitAct = m_menu->addAction("Quit");

    m_tray->setContextMenu(m_menu);

    // Warn if system tray not available
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::warning(nullptr, "System Tray", "No system tray is available on this system.");
    }

    QString currentMode = currentModeFromEnvy();
    m_tray->setIcon(iconForMode(currentMode));
    m_tray->setToolTip(QString("Current GPU: %1").arg(currentMode));
    m_tray->show();

    // Connect all actions including the missing integrated one!
    connect(intAct, &QAction::triggered, this, &GPUTray::setIntegrated);
    connect(hybAct, &QAction::triggered, this, &GPUTray::setHybrid);
    connect(nvdAct, &QAction::triggered, this, &GPUTray::setNvidia);
    connect(quitAct, &QAction::triggered, qApp, &QCoreApplication::quit);

    // Show the menu on left-click (ActivationReason::Trigger)
    connect(m_tray, &QSystemTrayIcon::activated, this, &GPUTray::onTrayActivated);

    // Update status initially and periodically
    QTimer::singleShot(0, this, &GPUTray::updateStatus);
    QTimer *periodic = new QTimer(this);
    periodic->setInterval(5000);
    connect(periodic, &QTimer::timeout, this, &GPUTray::updateStatus);
    periodic->start();
}

QString GPUTray::currentModeFromEnvy() const
{
    QProcess p;
    p.start("bash", QStringList() << "-lc" << "envycontrol --query 2>/dev/null || true");
    p.waitForFinished(1500);
    QString out = QString::fromLocal8Bit(p.readAllStandardOutput()).toLower();

    if (out.contains("nvidia"))
        return "nvidia";
    if (out.contains("integrated") || out.contains("intel"))
        return "integrated";
    if (out.contains("hybrid") || out.contains("switchable"))
        return "hybrid";

    // Fallback: try checking nvidia-smi presence
    p.start("bash", QStringList() << "-lc" << "command -v nvidia-smi >/dev/null && echo nvidia || echo integrated");
    p.waitForFinished(500);
    out = QString::fromLocal8Bit(p.readAllStandardOutput()).toLower();
    if (out.contains("nvidia"))
        return "nvidia";
    return "integrated";
}

QIcon GPUTray::iconForMode(const QString &mode) const
{
    QIcon icon;
    if (mode == "integrated") {
        icon = QIcon(":/icons/integrated.png");
        if (icon.isNull())
            icon = QIcon::fromTheme("video-display");
    } else if (mode == "hybrid") {
        icon = QIcon(":/icons/hybrid.png");
        if (icon.isNull())
            icon = QIcon::fromTheme("computer");
    } else if (mode == "nvidia") {
        icon = QIcon(":/icons/nvidia.png");
        if (icon.isNull())
            icon = QIcon::fromTheme("applications-graphics");
    } else {
        icon = QIcon::fromTheme("video-display");
    }
    
    if (icon.isNull()) {
        icon = QIcon::fromTheme("preferences-system");
    }
    
    return icon;
}

void GPUTray::updateStatus()
{
    QString mode = currentModeFromEnvy();
    QString statusText = QString("Current GPU: %1").arg(mode);

    if (m_tray && m_tray->contextMenu()) {
        QAction *first = m_tray->contextMenu()->actions().isEmpty() ? nullptr : m_tray->contextMenu()->actions().first();
        if (first)
            first->setText(statusText);
    }

    if (m_tray) {
        m_tray->setIcon(iconForMode(mode));
        m_tray->setToolTip(statusText);
    }
}

void GPUTray::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        // Toggle existing popup if visible
        if (m_popup && m_popup->isVisible()) {
            m_popup->hide();
            return;
        }

        // Create popup widget class (TrayPopup) and reuse when toggling
        if (!m_popup) {
            m_popup = new TrayPopup(m_menuParent);
            connect(m_popup, &TrayPopup::integratedRequested, this, &GPUTray::setIntegrated);
            connect(m_popup, &TrayPopup::hybridRequested, this, &GPUTray::setHybrid);
            connect(m_popup, &TrayPopup::nvidiaRequested, this, &GPUTray::setNvidia);
            connect(m_popup, &TrayPopup::quitRequested, qApp, &QCoreApplication::quit);
        }

        m_popup->setStatusText(QString("Current GPU: %1").arg(currentModeFromEnvy()));
        QPoint pos = QCursor::pos();
        m_popup->showAt(pos);
    }
}

static void runEnvyCommand(const QString &cmd)
{
    QString shellCmd = QString("bash -lc '%1' &").arg(cmd);
    QProcess::startDetached("bash", QStringList() << "-lc" << shellCmd);
}

void GPUTray::setIntegrated()
{
    runEnvyCommand("sudo envycontrol -s integrated");
    if (m_tray) m_tray->showMessage("GPU Switch", "Requested switch to Integrated. You may be asked for your password.");
    QTimer::singleShot(2000, this, &GPUTray::updateStatus);
}

void GPUTray::setHybrid()
{
    runEnvyCommand("sudo envycontrol -s hybrid --rtd3 2");
    if (m_tray) m_tray->showMessage("GPU Switch", "Requested switch to Hybrid. You may be asked for your password.");
    QTimer::singleShot(2000, this, &GPUTray::updateStatus);
}

void GPUTray::setNvidia()
{
    runEnvyCommand("sudo envycontrol -s nvidia");
    if (m_tray) m_tray->showMessage("GPU Switch", "Requested switch to Nvidia. You may be asked for your password.");
    QTimer::singleShot(2000, this, &GPUTray::updateStatus);
}