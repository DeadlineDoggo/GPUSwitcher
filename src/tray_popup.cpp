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
#include <QToolButton>
#include <QLineEdit>
#include <QScrollArea>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QComboBox>
#include <QProgressBar>

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
    lay->setContentsMargins(0,0,0,0);
    lay->setSpacing(0);

    // Header bar
    QWidget *header = new QWidget(this);
    header->setObjectName("popupHeader");
    header->setStyleSheet("#popupHeader { border-top-left-radius:10px; border-top-right-radius:10px; padding:10px 12px; }");
    QHBoxLayout *hhead = new QHBoxLayout(header);
    hhead->setContentsMargins(6,6,6,6);
    hhead->setSpacing(8);

    QPushButton *backBtn = new QPushButton(header);
    backBtn->setFlat(true);
    backBtn->setIcon(QIcon::fromTheme("go-previous"));
    backBtn->setToolTip("Back");
    backBtn->setFixedSize(28,28);
    connect(backBtn, &QPushButton::clicked, this, &TrayPopup::hide);
    hhead->addWidget(backBtn);

    QLabel *title = new QLabel("GPU Switcher", header);
    title->setStyleSheet("font-size:14px; font-weight:700;");
    hhead->addWidget(title);

    hhead->addStretch(1);
    // QPushButton *cfg = new QPushButton(header);
    // cfg->setFlat(true);
    // cfg->setIcon(QIcon::fromTheme("settings"));
    // cfg->setFixedSize(28,28);
    // hhead->addWidget(cfg);

    lay->addWidget(header);

    // Toolbar row (toggles / GPU quick actions)
    // QWidget *toolbar = new QWidget(this);
    // QHBoxLayout *ht = new QHBoxLayout(toolbar);
    // ht->setContentsMargins(12,8,12,8);
    // ht->setSpacing(8);

    // QToolButton *bInt = new QToolButton(toolbar);
    // bInt->setIcon(QIcon::fromTheme("chip"));
    // bInt->setText("Integrated");
    // bInt->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    // connect(bInt, &QToolButton::clicked, this, &TrayPopup::integratedRequested);
    // ht->addWidget(bInt);

    // QToolButton *bHyb = new QToolButton(toolbar);
    // bHyb->setIcon(QIcon::fromTheme("computer"));
    // bHyb->setText("Hybrid");
    // bHyb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    // connect(bHyb, &QToolButton::clicked, this, &TrayPopup::hybridRequested);
    // ht->addWidget(bHyb);

    // QToolButton *bNvd = new QToolButton(toolbar);
    // bNvd->setIcon(QIcon::fromTheme("nvidia"));
    // bNvd->setText("Nvidia");
    // bNvd->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    // connect(bNvd, &QToolButton::clicked, this, &TrayPopup::nvidiaRequested);
    // ht->addWidget(bNvd);

    // ht->addStretch(1);
    // lay->addWidget(toolbar);

    // Status area: big icon, refresh button, dropdown
    QWidget *statusArea = new QWidget(this);
    QVBoxLayout *sa = new QVBoxLayout(statusArea);
    sa->setContentsMargins(12,12,12,6);
    sa->setSpacing(8);

    m_iconLabel = new QLabel(statusArea);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setFixedSize(88,88);
    sa->addWidget(m_iconLabel, 0, Qt::AlignHCenter);

    QHBoxLayout *centerRow = new QHBoxLayout();
    centerRow->setSpacing(8);
    centerRow->setContentsMargins(0,0,0,0);

    QToolButton *refresh = new QToolButton(statusArea);
    refresh->setIcon(QIcon::fromTheme("view-refresh"));
    refresh->setToolTip("Refresh status");
    centerRow->addWidget(refresh, 0, Qt::AlignHCenter);

    m_modeCombo = new QComboBox(statusArea);
    m_modeCombo->addItems({"Integrated","Hybrid","Nvidia"});
    centerRow->addWidget(m_modeCombo, 0, Qt::AlignHCenter);

    sa->addLayout(centerRow);
    lay->addWidget(statusArea);

    // Separator
    QFrame *sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    lay->addWidget(sep);

    // Scrollable content area
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    QWidget *content = new QWidget(scroll);
    QVBoxLayout *cv = new QVBoxLayout(content);
    cv->setContentsMargins(12,12,12,12);
    cv->setSpacing(12);

    // Instead of network rows, show only small content area under status area
    content->setLayout(cv);
    // placeholder info row
    QLabel *info = new QLabel("Manage GPU switching and status", content);
    cv->addWidget(info);
    cv->addStretch(1);

    scroll->setWidget(content);
    scroll->setFixedSize(360, 160);
    lay->addWidget(scroll);

    // Busy/progress indicator at bottom
    m_progress = new QProgressBar(this);
    m_progress->setMinimum(0);
    m_progress->setMaximum(0); // indeterminate
    m_progress->setVisible(false);
    m_progress->setFixedHeight(10);
    lay->addWidget(m_progress);

    // Wire up controls
    connect(refresh, &QToolButton::clicked, this, &TrayPopup::refreshRequested);
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::activated), this, [this](int index){
        if (index == 0) emit integratedRequested();
        else if (index == 1) emit hybridRequested();
        else if (index == 2) emit nvidiaRequested();
    });

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

void TrayPopup::setModeIcon(const QIcon &icon)
{
    if (!m_iconLabel) return;
    QPixmap pm = icon.pixmap(72,72);
    if (pm.isNull()) pm = icon.pixmap(48,48);
    m_iconLabel->setPixmap(pm);
}

void TrayPopup::setMode(const QString &mode)
{
    if (!m_modeCombo) return;
    if (mode == "integrated") m_modeCombo->setCurrentIndex(0);
    else if (mode == "hybrid") m_modeCombo->setCurrentIndex(1);
    else if (mode == "nvidia") m_modeCombo->setCurrentIndex(2);
}

void TrayPopup::setBusy(bool busy)
{
    if (!m_progress) return;
    m_progress->setVisible(busy);
}
