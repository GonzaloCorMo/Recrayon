#pragma once

#include "core/Item.h"

#include <QRectF>

#include <vector>

class QPainter;

namespace recrayon {

/// Schedule for replaying a page: items are redrawn one after another in creation order, each
/// taking time proportional to how much there is to draw (Item::drawLength()), clamped so tiny
/// marks stay visible and long strokes do not drag on.
///
/// Holds shared pointers to the items, so the replay is unaffected by edits made meanwhile.
class ReplayTimeline {
public:
    /// Normal speed: stroke pixels drawn per second.
    static constexpr qreal kPixelsPerSecond = 900.0;
    static constexpr qint64 kMinItemMs = 150;
    static constexpr qint64 kMaxItemMs = 2500;
    static constexpr qint64 kPauseMs = 120; ///< between two items

    ReplayTimeline() = default;
    /// @p speed multiplies the drawing speed (0.5 = half as fast, 2 = twice as fast).
    ReplayTimeline(std::vector<ItemPtr> items, qreal speed);

    [[nodiscard]] bool isEmpty() const noexcept { return m_items.empty(); }
    [[nodiscard]] qint64 durationMs() const noexcept { return m_durationMs; }
    [[nodiscard]] const std::vector<ItemPtr>& items() const noexcept { return m_items; }

    /// How far item @p index is drawn at @p timeMs: 0 = not started, 1 = complete.
    [[nodiscard]] qreal progress(std::size_t index, qint64 timeMs) const;

    /// Paints every started item at @p timeMs; @p area limits work to one overlay's screen.
    void paint(QPainter& painter, qint64 timeMs, const QRectF& area) const;

private:
    struct Span {
        qint64 start;
        qint64 end;
    };

    std::vector<ItemPtr> m_items;
    std::vector<Span> m_spans;
    qint64 m_durationMs = 0;
};

} // namespace recrayon
