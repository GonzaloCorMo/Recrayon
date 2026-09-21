#pragma once

#include "core/Item.h"

#include <QPainterPath>

namespace recrayon {

enum class ShapeKind { Line, Arrow, Rectangle, Ellipse };

/// Geometric shape defined by two corner/end points (drag start and drag end).
class ShapeItem final : public Item {
public:
    ShapeItem(ShapeKind kind, StrokeStyle style, const QPointF& start, const QPointF& end);

    [[nodiscard]] ShapeKind kind() const noexcept { return m_kind; }
    [[nodiscard]] QPointF start() const noexcept { return m_start; }
    [[nodiscard]] QPointF end() const noexcept { return m_end; }
    void setEnd(const QPointF& end) noexcept { m_end = end; }

    /// True when the drag was too short to be intentional (e.g. a plain click).
    [[nodiscard]] bool isDegenerate() const noexcept;

    void paint(QPainter& painter) const override;
    [[nodiscard]] QRectF boundingRect() const override;
    [[nodiscard]] bool hitTest(const QPointF& point, qreal tolerance) const override;
    void translate(const QPointF& delta) override;
    void paintPartial(QPainter& painter, qreal progress) const override;
    [[nodiscard]] qreal drawLength() const override;

private:
    /// Centre line of the stroke; paint, bounds and hit-testing all derive from it.
    [[nodiscard]] QPainterPath outline() const;

    ShapeKind m_kind;
    QPointF m_start;
    QPointF m_end;
};

} // namespace recrayon
