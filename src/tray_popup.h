#pragma once

#include <QWidget>

class QLabel;
class QComboBox;
class QProgressBar;
class QTextEdit;
class QLineEdit;
class QSocketNotifier;
class QIcon;
class QString;

class TrayPopup : public QWidget
{
    Q_OBJECT
public:
    explicit TrayPopup(QWidget *parent = nullptr);
    void setStatusText(const QString &text);
    void showAt(const QPoint &globalPos);
    void setModeIcon(const QIcon &icon);
    void setBusy(bool busy);
    void setMode(const QString &mode);
    void startStdoutCapture();

signals:
    void integratedRequested();
    void hybridRequested();
    void nvidiaRequested();
    void refreshRequested();
    void quitRequested();

private:
    QLabel *m_label = nullptr;
    QLabel *m_iconLabel = nullptr;
    QComboBox *m_modeCombo = nullptr;
    QProgressBar *m_progress = nullptr;
    QTextEdit *m_console = nullptr;
    QLineEdit *m_consoleInput = nullptr;
    int m_stdoutFd = -1;
    QSocketNotifier *m_stdoutNotifier = nullptr;

    void trimConsole(int maxLines);
    void handleStdout();
};
