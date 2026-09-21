#pragma once

#include "core/Item.h"
#include "core/ShapeItem.h"
#include "core/TextItem.h"

namespace recrayon {

/// An arrow with a text label at its tail ("callout"): the arrow points at something, the
/// label explains it. The label sits on the side of the tail opposite to the tip
/// (geometry::calloutLabelAlignment()), so it never covers the arrow. Moves and erases as one.
class CalloutItem final : public Item {
public:
    /// Distance between the arrow's tail and the label.
    static constexpr qreal kLabelGap = 8.0;

    CalloutItem(const StrokeStyle& style, const QFont& font, const QPointF& tail,
                const QPointF& tip, const QString& text);

    [[nodiscard]] const ShapeItem& arrow() const noexcept { return m_arrow; }
    [[nodiscard]] const TextItem& label() const noexcept { return m_label; }

    void paint(QPainter& painter) const override;
    [[nodiscard]] QRectF boundingRect() const override;
    [[nodiscard]] bool hitTest(const QPointF& point, qreal tolerance) const override;
    void translate(const QPointF& delta) override;
    /// Draws the arrow first, then types the label.
    void paintPartial(QPainter& painter, qreal progress) const override;
    [[nodiscard]] qreal drawLength() const override;

private:
    ShapeItem m_arrow;
    TextItem m_label;
};

} // namespace recrayon
