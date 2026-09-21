#pragma once

#include "core/StrokeStyle.h"

#include <QPointF>
#include <QRectF>

#include <memory>
#include <utility>

class QPainter;

namespace recrayon {

/// Base class of everything that can be drawn on the document.
///
/// Coordinates are in document space: the virtual desktop in logical (device-independent) pixels,
/// so an item keeps its position no matter which overlay window paints it.
class Item {
public:
    explicit Item(StrokeStyle style) : m_style(std::move(style)) {}
    virtual ~Item() = default;

    Item(const Item&) = delete;
    Item& operator=(const Item&) = delete;
    Item(Item&&) = delete;
    Item& operator=(Item&&) = delete;

    /// Paints the item. The painter is already translated to document coordinates.
    virtual void paint(QPainter& painter) const = 0;

    /// Area touched by paint(), including the stroke width.
    [[nodiscard]] virtual QRectF boundingRect() const = 0;

    /// True when @p point lies on the stroke, allowing @p tolerance extra pixels.
    [[nodiscard]] virtual bool hitTest(const QPointF& point, qreal tolerance) const = 0;

    /// Paints the item as it looks @p progress (0..1) of the way through being drawn, for
    /// replays. The default fades the whole item in; subclasses trace their stroke instead.
    virtual void paintPartial(QPainter& painter, qreal progress) const;

    /// How much "drawing" the item represents (≈ stroke length in logical pixels). Replays give
    /// each item a duration proportional to it.
    [[nodiscard]] virtual qreal drawLength() const;

    /// Moves the item by @p delta. Only Document::translateItem() should call this, so that
    /// views are notified.
    virtual void translate(const QPointF& delta) = 0;

    [[nodiscard]] const StrokeStyle& style() const noexcept { return m_style; }

private:
    StrokeStyle m_style;
};

using ItemPtr = std::shared_ptr<Item>;

} // namespace recrayon
