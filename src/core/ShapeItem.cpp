#include "core/ShapeItem.h"

#include "core/Geometry.h"

#include <QLineF>
#include <QPainter>
#include <QPainterPathStroker>
#include <QPolygonF>

namespace recrayon {

namespace {
constexpr qreal kMinShapeSize = 2.0;
} // namespace

ShapeItem::ShapeItem(ShapeKind kind, StrokeStyle style, const QPointF& start, const QPointF& end)
    : Item(std::move(style)), m_kind(kind), m_start(start), m_end(end) {}

bool ShapeItem::isDegenerate() const noexcept {
    return QLineF(m_start, m_end).length() < kMinShapeSize;
}

QPainterPath ShapeItem::outline() const {
    QPainterPath path;
    switch (m_kind) {
    case ShapeKind::Line:
        path.moveTo(m_start);
        path.lineTo(m_end);
        break;
    case ShapeKind::Arrow: {
        path.moveTo(m_start);
        path.lineTo(m_end);
        const qreal shaftLength = QLineF(m_start, m_end).length();
        if (shaftLength >= 1.0) {
            const auto head = geometry::arrowHead(
                m_end, m_start, geometry::arrowHeadLength(style().width, shaftLength));
            path.moveTo(head.left);
            path.lineTo(m_end);
            path.lineTo(head.right);
        }
        break;
    }
    case ShapeKind::Rectangle:
        path.addRect(QRectF(m_start, m_end).normalized());
        break;
    case ShapeKind::Ellipse:
        path.addEllipse(QRectF(m_start, m_end).normalized());
        break;
    }
    return path;
}

void ShapeItem::paint(QPainter& painter) const {
    painter.setPen(style().toPen());
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(outline());
}

QRectF ShapeItem::boundingRect() const {
    const qreal margin = style().width / 2.0 + 1.0;
    return outline().boundingRect().adjusted(-margin, -margin, margin, margin);
}

void ShapeItem::paintPartial(QPainter& painter, qreal progress) const {
    if (progress >= 1.0) {
        paint(painter);
        return;
    }
    if (progress <= 0.0) {
        return;
    }
    if (m_kind == ShapeKind::Line || m_kind == ShapeKind::Arrow) {
        // Grows from its start; an arrow carries its head on the moving end.
        const ShapeItem partial(m_kind, style(), m_start, QLineF(m_start, m_end).pointAt(progress));
        partial.paint(painter);
        return;
    }
    // Rectangles and ellipses are traced along their outline.
    constexpr int kSteps = 160;
    const QPainterPath path = outline();
    QPolygonF traced;
    const int reached = static_cast<int>(progress * kSteps);
    for (int i = 0; i <= reached; ++i) {
        traced << path.pointAtPercent(static_cast<qreal>(i) / kSteps);
    }
    traced << path.pointAtPercent(progress);
    painter.setPen(style().toPen());
    painter.setBrush(Qt::NoBrush);
    painter.drawPolyline(traced);
}

qreal ShapeItem::drawLength() const {
    return m_kind == ShapeKind::Line || m_kind == ShapeKind::Arrow ? QLineF(m_start, m_end).length()
                                                                   : outline().length();
}

void ShapeItem::translate(const QPointF& delta) {
    m_start += delta;
    m_end += delta;
}

bool ShapeItem::hitTest(const QPointF& point, qreal tolerance) const {
    QPainterPathStroker stroker;
    stroker.setWidth(style().width + 2.0 * tolerance);
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);
    return stroker.createStroke(outline()).contains(point);
}

} // namespace recrayon
