#pragma once

#include <QColor>
#include <QPen>

namespace recrayon {

/// Visual attributes of a stroke. Captured when a gesture starts, so changing the current
/// color or width never alters items that were already drawn.
struct StrokeStyle {
    QColor color{0xE5, 0x39, 0x35};
    qreal width = 4.0;

    [[nodiscard]] QPen toPen() const {
        return QPen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    }

    friend bool operator==(const StrokeStyle&, const StrokeStyle&) = default;
};

} // namespace recrayon
