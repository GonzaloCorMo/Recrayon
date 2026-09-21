#include "ui/RecordingIndicator.h"

#include "ui/Icons.h"

#include <QCursor>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QTime>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWindow>

namespace recrayon {

namespace {

constexpr int kBlinkMs = 500;
constexpr int kMargin = 10;
constexpr int kDotRadius = 6;
constexpr int kDotColumn = 22; ///< space reserved on the left for the blinking dot
constexpr int kScreenMargin = 16;
constexpr qreal kCornerRadius = 10.0;

const QColor kRecordRed(0xE5, 0x39, 0x35);
const QColor kPausedAmber(0xFB, 0xC0, 0x2D);

constexpr auto kStyleSheet = R"(
QLabel#time {
    color: #E8EAED;
    font-size: 15px;
    font-weight: bold;
}
QLabel#target {
    color: rgba(232, 234, 237, 160);
    font-size: 10px;
}
QToolButton {
    border: none;
    border-radius: 6px;
    background: transparent;
}
QToolButton:hover {
    background: rgba(255, 255, 255, 26);
}
)";

QString elapsedText(qint64 milliseconds) {
    const QTime time = QTime(0, 0).addMSecs(static_cast<int>(milliseconds));
    return time.hour() > 0 ? time.toString(QStringLiteral("h:mm:ss"))
                           : time.toString(QStringLiteral("mm:ss"));
}

} // namespace

RecordingIndicator::RecordingIndicator(QWidget* parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                          Qt::WindowDoesNotAcceptFocus | Qt::NoDropShadowWindowHint) {
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowTitle(tr("Recording"));
    setStyleSheet(QString::fromLatin1(kStyleSheet));

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(kMargin + kDotColumn, kMargin, kMargin, kMargin);
    root->setSpacing(10);
    root->setSizeConstraint(QLayout::SetFixedSize);

    auto* text = new QVBoxLayout;
    text->setContentsMargins(0, 0, 0, 0);
    text->setSpacing(0);
    m_time = new QLabel(elapsedText(0), this);
    m_time->setObjectName(QStringLiteral("time"));
    m_target = new QLabel(this);
    m_target->setObjectName(QStringLiteral("target"));
    text->addWidget(m_time);
    text->addWidget(m_target);
    root->addLayout(text);

    m_pause = new QToolButton(this);
    m_pause->setIconSize(QSize(20, 20));
    m_pause->setFixedSize(30, 30);
    m_pause->setCursor(Qt::ArrowCursor);
    connect(m_pause, &QToolButton::clicked, this, [this] { emit pauseRequested(!m_paused); });
    root->addWidget(m_pause);

    auto* stop = new QToolButton(this);
    stop->setIcon(icons::icon(icons::IconId::Stop));
    stop->setIconSize(QSize(20, 20));
    stop->setFixedSize(30, 30);
    stop->setToolTip(tr("Stop recording"));
    stop->setAccessibleName(tr("Stop recording"));
    stop->setCursor(Qt::ArrowCursor);
    connect(stop, &QToolButton::clicked, this, &RecordingIndicator::stopRequested);
    root->addWidget(stop);

    m_timer.setInterval(kBlinkMs);
    connect(&m_timer, &QTimer::timeout, this, &RecordingIndicator::tick);
    hide();
}

void RecordingIndicator::start(const QString& target) {
    m_targetText = target;
    m_paused = false;
    m_recordedMs = 0;
    m_blinkOn = true;
    m_clock.start();
    m_timer.start();
    updateTexts();
    adjustSize();
    placeOnScreenUnderCursor();
    show();
    raise();
}

void RecordingIndicator::stop() {
    m_timer.stop();
    hide();
}

void RecordingIndicator::setPaused(bool paused) {
    if (paused == m_paused) {
        return;
    }
    if (paused) {
        m_recordedMs += m_clock.elapsed();
    } else {
        m_clock.restart();
    }
    m_paused = paused;
    m_blinkOn = true;
    updateTexts();
    update();
}

qint64 RecordingIndicator::elapsedMs() const {
    return m_recordedMs + (m_paused ? 0 : m_clock.elapsed());
}

void RecordingIndicator::updateTexts() {
    m_time->setText(elapsedText(elapsedMs()));
    m_target->setText(m_paused ? tr("Paused — %1").arg(m_targetText) : m_targetText);
    m_pause->setIcon(icons::icon(m_paused ? icons::IconId::Record : icons::IconId::Pause));
    const QString action = m_paused ? tr("Resume recording") : tr("Pause recording");
    m_pause->setToolTip(action);
    m_pause->setAccessibleName(action);
    adjustSize();
}

void RecordingIndicator::placeOnScreenUnderCursor() {
    if (m_moved) {
        return; // the user chose where it goes
    }
    const QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen) {
        return;
    }
    const QRect area = screen->availableGeometry();
    // Top center: out of the way of most content, and hard to miss.
    move(area.center().x() - width() / 2, area.top() + kScreenMargin);
}

void RecordingIndicator::tick() {
    m_blinkOn = m_paused || !m_blinkOn; // steady while paused
    m_time->setText(elapsedText(elapsedMs()));
    update();
}

void RecordingIndicator::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(QPen(QColor(255, 255, 255, 40), 1));
    painter.setBrush(QColor(32, 33, 36, 235));
    painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), kCornerRadius,
                            kCornerRadius);

    QColor dot = m_paused ? kPausedAmber : kRecordRed;
    if (!m_blinkOn) {
        dot.setAlpha(70);
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(dot);
    painter.drawEllipse(QPointF(kMargin + kDotRadius, height() / 2.0), kDotRadius, kDotRadius);
}

void RecordingIndicator::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && windowHandle()) {
        m_moved = true; // from now on it stays where the user puts it
        windowHandle()->startSystemMove();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

} // namespace recrayon
