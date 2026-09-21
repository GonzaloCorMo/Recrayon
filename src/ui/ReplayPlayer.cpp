#include "ui/ReplayPlayer.h"

#include "core/Document.h"

#include <algorithm>

namespace recrayon {

namespace {
constexpr int kFrameIntervalMs = 16;
/// The finished drawing stays for a moment before the replay ends.
constexpr qint64 kHoldMs = 600;
} // namespace

ReplayPlayer::ReplayPlayer(QObject* parent) : QObject(parent) {
    m_timer.setInterval(kFrameIntervalMs);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &ReplayPlayer::tick);
}

bool ReplayPlayer::start(const QList<const Document*>& pages, qreal speed) {
    stop();
    m_timelines.clear();
    m_durationMs = 0;
    for (const Document* page : pages) {
        if (!page || page->isEmpty() || m_timelines.contains(page)) {
            continue;
        }
        ReplayTimeline timeline(page->items(), speed);
        m_durationMs = std::max(m_durationMs, timeline.durationMs());
        m_timelines.insert(page, std::move(timeline));
    }
    if (m_timelines.isEmpty()) {
        return false;
    }
    m_now = 0;
    m_clock.start();
    m_timer.start();
    emit activeChanged(true);
    emit frame();
    return true;
}

void ReplayPlayer::stop() {
    if (!m_timer.isActive()) {
        return;
    }
    m_timer.stop();
    m_timelines.clear();
    emit activeChanged(false);
    emit frame();
}

void ReplayPlayer::tick() {
    m_now = m_clock.elapsed();
    if (m_now >= m_durationMs + kHoldMs) {
        stop();
        return;
    }
    emit frame();
}

void ReplayPlayer::paint(QPainter& painter, const Document* page, const QRectF& area) const {
    const auto it = m_timelines.constFind(page);
    if (it != m_timelines.cend()) {
        it->paint(painter, m_now, area);
    }
}

} // namespace recrayon
