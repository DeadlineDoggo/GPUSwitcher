#include "tray_popup.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QPalette>
#include <QColor>
#include <QPropertyAnimation>
#include <QScreen>
#include <QGuiApplication>

TrayPopup::TrayPopup(QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::Popup)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlag(Qt::WindowStaysOnTopHint, true);
    setAttribute(Qt::WA_ShowWithoutActivating);

    // Use palette to pick background color similar to Plasma panels
    QPalette pal = this->palette();
    QColor bg = pal.color(QPalette::Window);
    QColor text = pal.color(QPalette::WindowText);
    QString bg_rgba = QString("rgba(%1,%2,%3,%4)").arg(bg.red()).arg(bg.green()).arg(bg.blue()).arg(240);
    QString txt_rgba = QString("rgba(%1,%2,%3,%4)").arg(text.red()).arg(text.green()).arg(text.blue()).arg(255);

    // Rounded background and padding to resemble Plasma popup
    setStyleSheet(QString("background: %1; border-radius:10px; color: %2;")
                  .arg(bg_rgba).arg(txt_rgba));

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(22);
    shadow->setOffset(0,8);
    shadow->setColor(QColor(0,0,0,140));
    setGraphicsEffect(shadow);

    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(12,12,12,12);
    lay->setSpacing(8);

    m_label = new QLabel("", this);
    m_label->setStyleSheet("font-weight:600;");
    lay->addWidget(m_label);

    QPushButton *bInt = new QPushButton("Integrated", this);
    connect(bInt, &QPushButton::clicked, this, &TrayPopup::integratedRequested);
    lay->addWidget(bInt);

    QPushButton *bHyb = new QPushButton("Hybrid", this);
    connect(bHyb, &QPushButton::clicked, this, &TrayPopup::hybridRequested);
    lay->addWidget(bHyb);

    QPushButton *bNvd = new QPushButton("Nvidia", this);
    connect(bNvd, &QPushButton::clicked, this, &TrayPopup::nvidiaRequested);
    lay->addWidget(bNvd);

    lay->addSpacing(6);
    QPushButton *bQuit = new QPushButton("Quit", this);
    connect(bQuit, &QPushButton::clicked, this, &TrayPopup::quitRequested);
    lay->addWidget(bQuit);

    setLayout(lay);
}

void TrayPopup::showAt(const QPoint &globalPos)
{
    // Compute available geometry from the screen under the cursor
    QScreen *screen = QGuiApplication::screenAt(globalPos);
    QRect avail = screen ? screen->availableGeometry() : QGuiApplication::primaryScreen()->availableGeometry();

    adjustSize();
    QSize s = size();

    // Prefer placing above the cursor if there's more space above, otherwise below
    int x = globalPos.x() - s.width()/2;
    int y;
    const int margin = 8;

    if (globalPos.y() > avail.center().y())
        y = globalPos.y() - s.height() - margin; // above
    else
        y = globalPos.y() + margin; // below

    // Clamp within available geometry
    x = qMax(avail.left() + margin, qMin(x, avail.right() - s.width() - margin));
    y = qMax(avail.top() + margin, qMin(y, avail.bottom() - s.height() - margin));

    move(QPoint(x, y));

    // Fade-in animation
    setWindowOpacity(0.0);
    show();
    QPropertyAnimation *anim = new QPropertyAnimation(this, "windowOpacity", this);
    anim->setDuration(180);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::InOutQuad);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void TrayPopup::setStatusText(const QString &text)
{
    if (m_label)
        m_label->setText(text);
}
