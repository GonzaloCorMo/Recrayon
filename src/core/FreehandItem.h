#pragma once

#include "core/Geometry.h"
#include "core/Item.h"

#include <QList>
#include <QPainterPath>

#include <optional>

namespace recrayon {

/// Free-form stroke (pen, highlighter, free arrow). Points are smoothed with quadratic Bézier
/// segments through the midpoints of consecutive samples; the path is built incrementally so
/// long strokes stay cheap while they are being drawn.
class FreehandItem final : public Item {
public:
    enum class Ending { None, Arrow };

    explicit FreehandItem(StrokeStyle style, Ending ending = Ending::None);

    /// Appends a sample. Returns false (and ignores it) when it is too close to the previous one.
    bool addPoint(const QPointF& point);

    [[nodiscard]] const QList<QPointF>& points() const noexcept { return m_points; }
    [[nodiscard]] Ending ending() const noexcept { return m_ending; }

    void paint(QPainter& painter) const override;
    [[nodiscard]] QRectF boundingRect() const override;
    [[nodiscard]] bool hitTest(const QPointF& point, qreal tolerance) const override;
    void translate(const QPointF& delta) override;
    void paintPartial(QPainter& painter, qreal progress) const override;
    [[nodiscard]] qreal drawLength() const override;

    /// Arrow head at the last point, if this stroke has one and is long enough to orient it.
    [[nodiscard]] std::optional<geometry::ArrowHead> arrowHead() const;

private:
    Ending m_ending;
    QList<QPointF> m_points;
    QPainterPath m_path;
    QPointF m_min;
    QPointF m_max;
};

} // namespace recrayon
