#include "core/DesktopLayout.h"

#include <algorithm>
#include <limits>

namespace recrayon {

namespace {

/// Squared distance from @p point to @p rect (0 when inside).
qreal distanceSquared(const QRectF& rect, const QPointF& point) {
    const qreal dx = std::max({rect.left() - point.x(), 0.0, point.x() - rect.right()});
    const qreal dy = std::max({rect.top() - point.y(), 0.0, point.y() - rect.bottom()});
    return dx * dx + dy * dy;
}

template <typename RectOf>
qsizetype nearestIndex(const QList<ScreenMapping>& screens, const QPointF& point, RectOf rectOf) {
    qsizetype best = -1;
    qreal bestDistance = std::numeric_limits<qreal>::max();
    for (qsizetype i = 0; i < screens.size(); ++i) {
        const qreal distance = distanceSquared(QRectF(rectOf(screens.at(i))), point);
        if (distance < bestDistance) {
            bestDistance = distance;
            best = i;
        }
    }
    return best;
}

} // namespace

QRect DesktopLayout::nativeBounds() const {
    QRect bounds;
    for (const ScreenMapping& screen : m_screens) {
        bounds = bounds.united(screen.native);
    }
    return bounds;
}

qsizetype DesktopLayout::indexAtLogical(const QPointF& point) const {
    return nearestIndex(m_screens, point, [](const ScreenMapping& s) { return s.logical; });
}

qsizetype DesktopLayout::indexAtNative(const QPointF& point) const {
    return nearestIndex(m_screens, point, [](const ScreenMapping& s) { return s.native; });
}

QPointF DesktopLayout::toNative(const QPointF& logical) const {
    const qsizetype index = indexAtLogical(logical);
    if (index < 0) {
        return logical;
    }
    const ScreenMapping& screen = m_screens.at(index);
    return QPointF(screen.native.topLeft()) +
           (logical - QPointF(screen.logical.topLeft())) * screen.devicePixelRatio;
}

QPointF DesktopLayout::toLogical(const QPointF& native) const {
    const qsizetype index = indexAtNative(native);
    if (index < 0) {
        return native;
    }
    const ScreenMapping& screen = m_screens.at(index);
    return QPointF(screen.logical.topLeft()) +
           (native - QPointF(screen.native.topLeft())) / screen.devicePixelRatio;
}

QRect DesktopLayout::toNative(const QRectF& logical) const {
    const QRectF normalized = logical.normalized();
    return QRectF(toNative(normalized.topLeft()), toNative(normalized.bottomRight()))
        .normalized()
        .toAlignedRect();
}

QRectF DesktopLayout::toLogical(const QRect& native) const {
    const QRectF normalized = QRectF(native).normalized();
    return QRectF(toLogical(normalized.topLeft()), toLogical(normalized.bottomRight()))
        .normalized();
}

} // namespace recrayon
