#include "ui/RegionPicker.h"

#include "ui/OverlayHint.h"

#include <QCursor>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QLineF>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>

#include <algorithm>
#include <utility>

namespace recrayon {

namespace {

constexpr qreal kDragThreshold = 4.0;
const QColor kDim(0, 0, 0, 110);
const QColor kAccent(0x8A, 0xB4, 0xF8);
constexpr qreal kLabelGap = 6.0;
} // namespace

RegionPicker::RegionPicker(QObject* parent) : QObject(parent) {}

void RegionPicker::setWindowLookup(WindowLookup lookup) {
    m_windowLookup = std::move(lookup);
}

void RegionPicker::setNativeSize(NativeSize nativeSize) {
    m_nativeSize = std::move(nativeSize);
}

void RegionPicker::start() {
    m_active = true;
    m_pressed = false;
    m_currentPos = QCursor::pos();
    updateHover(m_currentPos);
    emit changed();
}

void RegionPicker::cancel() {
    if (!m_active) {
        return;
    }
    m_active = false;
    m_pressed = false;
    m_hover.reset();
    emit changed();
    emit canceled();
}

bool RegionPicker::isDragging() const {
    return m_pressed && QLineF(m_pressPos, m_currentPos).length() >= kDragThreshold;
}

void RegionPicker::press(const QPointF& pos) {
    if (!m_active) {
        return;
    }
    m_pressed = true;
    m_pressPos = pos;
    m_currentPos = pos;
    emit changed();
}

void RegionPicker::move(const QPointF& pos) {
    if (!m_active) {
        return;
    }
    m_currentPos = pos;
    if (!m_pressed) {
        updateHover(pos);
    }
    emit changed();
}

void RegionPicker::release(const QPointF& pos) {
    if (!m_active || !m_pressed) {
        return;
    }
    m_currentPos = pos;
    Selection selection;
    if (isDragging()) {
        selection.logicalRect = QRectF(m_pressPos, m_currentPos).normalized();
    } else if (m_hover) {
        selection = {m_hover->logicalRect, m_hover->id};
    } else if (const QScreen* screen = QGuiApplication::screenAt(pos.toPoint())) {
        selection.logicalRect = screen->geometry(); // clicked the desktop: whole screen
    } else {
        cancel();
        return;
    }
    m_active = false;
    m_pressed = false;
    m_hover.reset();
    emit changed();
    emit finished(selection);
}

void RegionPicker::updateHover(const QPointF& pos) {
    m_hover = m_windowLookup ? m_windowLookup(pos) : std::nullopt;
}

QRectF RegionPicker::highlightedRect() const {
    if (isDragging()) {
        return QRectF(m_pressPos, m_currentPos).normalized();
    }
    if (!m_pressed && m_hover) {
        return m_hover->logicalRect;
    }
    return {};
}

void RegionPicker::paint(QPainter& painter, const QRectF& area) const {
    if (!m_active) {
        return;
    }
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, false);

    const QRectF highlight = highlightedRect();
    QPainterPath dim;
    dim.setFillRule(Qt::OddEvenFill);
    dim.addRect(area);
    if (!highlight.isEmpty()) {
        dim.addRect(highlight);
    }
    painter.fillPath(dim, kDim);

    if (!highlight.isEmpty()) {
        painter.setPen(QPen(kAccent, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(highlight);

        const QSize nativeSize = m_nativeSize ? m_nativeSize(highlight) : highlight.size().toSize();
        QString label = QStringLiteral("%1 × %2").arg(nativeSize.width()).arg(nativeSize.height());
        if (!isDragging() && m_hover && !m_hover->title.isEmpty()) {
            label = QStringLiteral("%1 — %2").arg(m_hover->title.left(60), label);
        }
        hints::paintLabel(painter,
                          highlight.topLeft() - QPointF(0, hints::labelHeight(painter) + kLabelGap),
                          label, area);
    }

    const QString hint = tr("Drag to select an area  ·  Click a window  ·  Click the desktop for "
                            "the whole screen  ·  Esc to cancel");
    hints::paintBanner(painter, area, hint);

    painter.restore();
}

} // namespace recrayon
