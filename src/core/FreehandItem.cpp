#include "core/FreehandItem.h"

#include <QPainter>

#include <algorithm>
#include <limits>

namespace recrayon {

namespace {
/// Samples closer than this (in logical pixels) add noise, not detail.
constexpr qreal kMinPointDistance = 1.5;

/// The arrow direction is taken from a point this fraction of the head length back along the
/// stroke, so the last jittery samples of a gesture do not make the head point sideways.
constexpr qreal kArrowLookBackFactor = 0.8;
} // namespace

FreehandItem::FreehandItem(StrokeStyle style, Ending ending)
    : Item(std::move(style)), m_ending(ending) {}

bool FreehandItem::addPoint(const QPointF& point) {
    if (!m_points.isEmpty() && QLineF(m_points.last(), point).length() < kMinPointDistance) {
        return false;
    }

    m_points.append(point);
    const qsizetype count = m_points.size();

    if (count == 1) {
        m_path.moveTo(point);
        m_min = point;
        m_max = point;
        return true;
    }

    m_min = QPointF(std::min(m_min.x(), point.x()), std::min(m_min.y(), point.y()));
    m_max = QPointF(std::max(m_max.x(), point.x()), std::max(m_max.y(), point.y()));

    // Curve through the previous sample towards the midpoint of the last two. The tail from that
    // midpoint to the newest sample is added at paint time, so the stroke follows the cursor.
    if (count >= 3) {
        const QPointF& control = m_points.at(count - 2);
        m_path.quadTo(control, geometry::midpoint(control, point));
    }
    return true;
}

std::optional<geometry::ArrowHead> FreehandItem::arrowHead() const {
    if (m_ending != Ending::Arrow || m_points.size() < 2) {
        return std::nullopt;
    }
    const QPointF tip = m_points.last();
    const qreal headLength =
        geometry::arrowHeadLength(style().width, std::numeric_limits<qreal>::max());

    QPointF from = m_points.first();
    for (qsizetype i = m_points.size() - 2; i >= 0; --i) {
        if (QLineF(tip, m_points.at(i)).length() >= headLength * kArrowLookBackFactor) {
            from = m_points.at(i);
            break;
        }
    }
    if (QLineF(tip, from).length() < 1.0) {
        return std::nullopt;
    }
    return geometry::arrowHead(tip, from, headLength);
}

void FreehandItem::paint(QPainter& painter) const {
    if (m_points.isEmpty()) {
        return;
    }

    painter.setPen(style().toPen());
    painter.setBrush(Qt::NoBrush);

    if (m_points.size() == 1) {
        painter.drawPoint(m_points.first());
        return;
    }

    // Single path (instead of path + separate tail line) so translucent strokes have no
    // darker overlap where the pieces would meet.
    QPainterPath path = m_path;
    path.lineTo(m_points.last());
    if (const auto head = arrowHead()) {
        path.moveTo(head->left);
        path.lineTo(m_points.last());
        path.lineTo(head->right);
    }
    painter.drawPath(path);
}

void FreehandItem::paintPartial(QPainter& painter, qreal progress) const {
    if (progress >= 1.0) {
        paint(painter);
        return;
    }
    if (progress <= 0.0 || m_points.isEmpty()) {
        return;
    }
    // Replay the stroke as it was drawn: the same samples up to the reached length, the last
    // one interpolated. A free arrow shows its head at the moving tip.
    const qreal target = drawLength() * progress;
    FreehandItem partial(style(), m_ending);
    partial.addPoint(m_points.first());
    qreal travelled = 0.0;
    for (qsizetype i = 1; i < m_points.size(); ++i) {
        const QLineF segment(m_points.at(i - 1), m_points.at(i));
        if (travelled + segment.length() >= target) {
            partial.addPoint(segment.pointAt((target - travelled) / segment.length()));
            break;
        }
        travelled += segment.length();
        partial.addPoint(m_points.at(i));
    }
    partial.paint(painter);
}

qreal FreehandItem::drawLength() const {
    qreal length = 0.0;
    for (qsizetype i = 1; i < m_points.size(); ++i) {
        length += QLineF(m_points.at(i - 1), m_points.at(i)).length();
    }
    return length;
}

QRectF FreehandItem::boundingRect() const {
    if (m_points.isEmpty()) {
        return {};
    }
    QPointF min = m_min;
    QPointF max = m_max;
    if (const auto head = arrowHead()) {
        for (const QPointF& p : {head->left, head->right}) {
            min = QPointF(std::min(min.x(), p.x()), std::min(min.y(), p.y()));
            max = QPointF(std::max(max.x(), p.x()), std::max(max.y(), p.y()));
        }
    }
    const qreal margin = style().width / 2.0 + 1.0;
    return QRectF(min, max).adjusted(-margin, -margin, margin, margin);
}

bool FreehandItem::hitTest(const QPointF& point, qreal tolerance) const {
    if (m_points.isEmpty() ||
        !boundingRect().adjusted(-tolerance, -tolerance, tolerance, tolerance).contains(point)) {
        return false;
    }

    const qreal reach = tolerance + style().width / 2.0;
    if (m_points.size() == 1) {
        return QLineF(point, m_points.first()).length() <= reach;
    }

    for (qsizetype i = 1; i < m_points.size(); ++i) {
        if (geometry::distanceToSegment(point, m_points.at(i - 1), m_points.at(i)) <= reach) {
            return true;
        }
    }
    if (const auto head = arrowHead()) {
        const QPointF tip = m_points.last();
        return geometry::distanceToSegment(point, head->left, tip) <= reach ||
               geometry::distanceToSegment(point, head->right, tip) <= reach;
    }
    return false;
}

void FreehandItem::translate(const QPointF& delta) {
    for (QPointF& p : m_points) {
        p += delta;
    }
    m_path.translate(delta);
    m_min += delta;
    m_max += delta;
}

} // namespace recrayon
