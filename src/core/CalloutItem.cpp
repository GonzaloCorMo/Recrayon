#include "core/CalloutItem.h"

#include "core/Geometry.h"

#include <algorithm>

namespace recrayon {

CalloutItem::CalloutItem(const StrokeStyle& style, const QFont& font, const QPointF& tail,
                         const QPointF& tip, const QString& text)
    : Item(style), m_arrow(ShapeKind::Arrow, style, tail, tip),
      m_label(style, font, tail, geometry::calloutLabelAlignment(tail, tip), kLabelGap, text) {}

void CalloutItem::paint(QPainter& painter) const {
    m_arrow.paint(painter);
    m_label.paint(painter);
}

QRectF CalloutItem::boundingRect() const {
    return m_arrow.boundingRect().united(m_label.boundingRect());
}

bool CalloutItem::hitTest(const QPointF& point, qreal tolerance) const {
    return m_arrow.hitTest(point, tolerance) || m_label.hitTest(point, tolerance);
}

void CalloutItem::paintPartial(QPainter& painter, qreal progress) const {
    const qreal arrowShare = m_arrow.drawLength() / drawLength();
    m_arrow.paintPartial(painter, arrowShare > 0 ? progress / arrowShare : 1.0);
    if (progress > arrowShare) {
        m_label.paintPartial(painter, (progress - arrowShare) / (1.0 - arrowShare));
    }
}

qreal CalloutItem::drawLength() const {
    return std::max(1.0, m_arrow.drawLength() + m_label.drawLength());
}

void CalloutItem::translate(const QPointF& delta) {
    m_arrow.translate(delta);
    m_label.translate(delta);
}

} // namespace recrayon
