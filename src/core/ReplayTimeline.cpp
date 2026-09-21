#include "core/ReplayTimeline.h"

#include <QPainter>

#include <algorithm>

namespace recrayon {

ReplayTimeline::ReplayTimeline(std::vector<ItemPtr> items, qreal speed)
    : m_items(std::move(items)) {
    const qreal pixelsPerMs = kPixelsPerSecond * std::max(speed, 0.05) / 1000.0;
    qint64 time = 0;
    m_spans.reserve(m_items.size());
    for (const ItemPtr& item : m_items) {
        const auto duration = std::clamp(static_cast<qint64>(item->drawLength() / pixelsPerMs),
                                         kMinItemMs, kMaxItemMs);
        m_spans.push_back({time, time + duration});
        time += duration + kPauseMs;
    }
    m_durationMs = m_spans.empty() ? 0 : m_spans.back().end;
}

qreal ReplayTimeline::progress(std::size_t index, qint64 timeMs) const {
    const Span& span = m_spans.at(index);
    if (timeMs <= span.start) {
        return 0.0;
    }
    if (timeMs >= span.end) {
        return 1.0;
    }
    const qreal linear =
        static_cast<qreal>(timeMs - span.start) / static_cast<qreal>(span.end - span.start);
    // Ease in and out, like a hand that accelerates and settles.
    return linear * linear * (3.0 - 2.0 * linear);
}

void ReplayTimeline::paint(QPainter& painter, qint64 timeMs, const QRectF& area) const {
    for (std::size_t i = 0; i < m_items.size(); ++i) {
        if (m_spans[i].start >= timeMs) {
            break; // items are in time order: nothing after this has started
        }
        const ItemPtr& item = m_items[i];
        if (item->boundingRect().intersects(area)) {
            item->paintPartial(painter, progress(i, timeMs));
        }
    }
}

} // namespace recrayon
