#pragma once

#include <QLineF>
#include <QPointF>
#include <QRectF>
#include <QSize>

#include <algorithm>
#include <cmath>

namespace recrayon::geometry {

/// Shortest distance from @p point to the segment [@p a, @p b].
[[nodiscard]] inline qreal distanceToSegment(const QPointF& point, const QPointF& a,
                                             const QPointF& b) noexcept {
    const QPointF ab = b - a;
    const qreal lengthSquared = QPointF::dotProduct(ab, ab);
    if (qFuzzyIsNull(lengthSquared)) {
        return QLineF(point, a).length();
    }
    const qreal t = std::clamp(QPointF::dotProduct(point - a, ab) / lengthSquared, 0.0, 1.0);
    return QLineF(point, a + t * ab).length();
}

[[nodiscard]] inline QPointF midpoint(const QPointF& a, const QPointF& b) noexcept {
    return (a + b) / 2.0;
}

/// The two barb end points of an arrow head drawn at @p tip.
struct ArrowHead {
    QPointF left;
    QPointF right;
};

/// Arrow head at @p tip for a shaft arriving from @p from.
[[nodiscard]] inline ArrowHead arrowHead(const QPointF& tip, const QPointF& from, qreal length,
                                         qreal angleDegrees = 28.0) {
    const QLineF backwards(tip, from);
    QLineF left = backwards;
    left.setLength(length);
    left.setAngle(backwards.angle() + angleDegrees);
    QLineF right = backwards;
    right.setLength(length);
    right.setAngle(backwards.angle() - angleDegrees);
    return {left.p2(), right.p2()};
}

/// Largest rectangle with the aspect ratio of @p content that fits in @p box, centered
/// (letterboxing). Returns @p box when @p content is empty.
[[nodiscard]] inline QRectF fitCentered(const QSizeF& content, const QRectF& box) {
    if (content.isEmpty()) {
        return box;
    }
    const qreal scale = std::min(box.width() / content.width(), box.height() / content.height());
    const QSizeF size = content * scale;
    return {box.topLeft() +
                QPointF((box.width() - size.width()) / 2.0, (box.height() - size.height()) / 2.0),
            size};
}

/// Box of @p size placed next to @p anchor. @p alignment names the side of the box that faces
/// the anchor: AlignLeft puts the box's left edge @p gap to the right of the anchor, AlignRight
/// its right edge @p gap to the left, AlignTop / AlignBottom likewise vertically and the
/// *Center flags center it on that axis.
[[nodiscard]] inline QRectF alignedBox(const QPointF& anchor, const QSizeF& size,
                                       Qt::Alignment alignment, qreal gap = 0.0) {
    qreal x = anchor.x() - size.width() / 2.0;
    if (alignment & Qt::AlignLeft) {
        x = anchor.x() + gap;
    } else if (alignment & Qt::AlignRight) {
        x = anchor.x() - gap - size.width();
    }
    qreal y = anchor.y() - size.height() / 2.0;
    if (alignment & Qt::AlignTop) {
        y = anchor.y() + gap;
    } else if (alignment & Qt::AlignBottom) {
        y = anchor.y() - gap - size.height();
    }
    return {QPointF(x, y), size};
}

/// Where a callout's label goes relative to the arrow's tail: on the side opposite to the tip,
/// along the dominant direction, so the text never overlaps the arrow.
[[nodiscard]] inline Qt::Alignment calloutLabelAlignment(const QPointF& tail, const QPointF& tip) {
    const QPointF away = tail - tip;
    if (std::abs(away.x()) >= std::abs(away.y())) {
        return (away.x() >= 0 ? Qt::AlignLeft : Qt::AlignRight) | Qt::AlignVCenter;
    }
    return (away.y() >= 0 ? Qt::AlignTop : Qt::AlignBottom) | Qt::AlignHCenter;
}

/// Frame size a video encoder accepts: at most @p maxWidth wide (scaled proportionally), both
/// dimensions even (YUV 4:2:0 subsampling) and at least 16 px.
[[nodiscard]] inline QSize evenVideoSize(const QSize& size, int maxWidth = 3840) {
    if (size.isEmpty()) {
        return {16, 16};
    }
    QSizeF scaled(size);
    if (scaled.width() > maxWidth) {
        scaled *= maxWidth / scaled.width();
    }
    const auto even = [](qreal value) {
        return std::max(16, qRound(value) & ~1);
    };
    return {even(scaled.width()), even(scaled.height())};
}

/// Head length that scales with the stroke but never exceeds the @p available shaft length.
[[nodiscard]] inline qreal arrowHeadLength(qreal strokeWidth, qreal available) noexcept {
    constexpr qreal kMinLength = 12.0;
    constexpr qreal kWidthFactor = 4.0;
    return std::min(std::max(kMinLength, strokeWidth * kWidthFactor), available);
}

} // namespace recrayon::geometry
