#include "ui/PointerHighlight.h"

#include <QCursor>
#include <QPainter>
#include <QPainterPath>

#include <utility>

namespace recrayon {

namespace {

constexpr int kPollIntervalMs = 16; // ~60 Hz

constexpr qreal kSpotlightRadius = 140.0;
const QColor kSpotlightDim(0, 0, 0, 150);
const QColor kSpotlightRing(255, 255, 255, 110);

constexpr qreal kHaloRadius = 30.0;
const QColor kHaloFill(255, 235, 59, 80);
const QColor kHaloRing(255, 235, 59, 200);

constexpr qreal kRingWidth = 2.5;

QRectF circleRect(const QPointF& center, qreal radius) {
    return {center.x() - radius, center.y() - radius, 2 * radius, 2 * radius};
}

} // namespace

PointerHighlight::PointerHighlight(QObject* parent) : QObject(parent) {
    m_timer.setInterval(kPollIntervalMs);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &PointerHighlight::poll);
}

void PointerHighlight::setMode(PointerHighlightMode mode) {
    if (mode == m_mode) {
        return;
    }
    m_mode = mode;
    m_position = QCursor::pos();
    updateTimer();
    emit modeChanged(mode);
    emit appearanceChanged();
}

void PointerHighlight::setCursorProvider(CursorProvider provider) {
    m_cursorProvider = std::move(provider);
}

void PointerHighlight::setCursorReplicaEnabled(bool enabled) {
    if (enabled == m_replicaEnabled) {
        return;
    }
    m_replicaEnabled = enabled;
    m_position = QCursor::pos();
    m_cursor = enabled && m_cursorProvider ? m_cursorProvider() : CursorImage{};
    updateTimer();
    emit appearanceChanged();
}

bool PointerHighlight::isActive() const noexcept {
    return m_mode != PointerHighlightMode::Off || m_replicaEnabled;
}

void PointerHighlight::updateTimer() {
    if (isActive()) {
        m_timer.start();
    } else {
        m_timer.stop();
    }
}

void PointerHighlight::poll() {
    const QPointF position = QCursor::pos();
    CursorImage cursor = m_replicaEnabled && m_cursorProvider ? m_cursorProvider() : CursorImage{};
    const bool cursorChanged =
        cursor.visible != m_cursor.visible || cursor.image.cacheKey() != m_cursor.image.cacheKey();
    if (position == m_position && !cursorChanged) {
        return;
    }
    const QRectF before = dirtyRect();
    m_position = position;
    m_cursor = std::move(cursor);
    emit moved(before.united(dirtyRect()));
}

QRectF PointerHighlight::dirtyRect() const {
    QRectF rect;
    if (m_mode != PointerHighlightMode::Off) {
        const qreal radius =
            m_mode == PointerHighlightMode::Spotlight ? kSpotlightRadius : kHaloRadius;
        rect = circleRect(m_position, radius + kRingWidth);
    }
    if (m_replicaEnabled && m_cursor.visible) {
        rect = rect.united(
            QRectF(m_position - m_cursor.hotspot, m_cursor.image.deviceIndependentSize()));
    }
    return rect;
}

void PointerHighlight::paint(QPainter& painter, const QRectF& area) const {
    switch (m_mode) {
    case PointerHighlightMode::Off:
        break;
    case PointerHighlightMode::Spotlight: {
        QPainterPath dim;
        dim.setFillRule(Qt::OddEvenFill);
        dim.addRect(area);
        dim.addEllipse(m_position, kSpotlightRadius, kSpotlightRadius);
        painter.fillPath(dim, kSpotlightDim);
        painter.setPen(QPen(kSpotlightRing, kRingWidth));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(m_position, kSpotlightRadius, kSpotlightRadius);
        break;
    }
    case PointerHighlightMode::Halo:
        painter.setPen(QPen(kHaloRing, kRingWidth));
        painter.setBrush(kHaloFill);
        painter.drawEllipse(m_position, kHaloRadius, kHaloRadius);
        break;
    }

    if (m_replicaEnabled && m_cursor.visible && !m_cursor.image.isNull()) {
        painter.drawImage(m_position - m_cursor.hotspot, m_cursor.image);
    }
}

} // namespace recrayon
