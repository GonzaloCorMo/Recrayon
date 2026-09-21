#pragma once

#include "core/ReplayTimeline.h"

#include <QElapsedTimer>
#include <QHash>
#include <QList>
#include <QObject>
#include <QRectF>
#include <QTimer>

class QPainter;

namespace recrayon {

class Document;

/// Plays back the drawing of one or more pages from scratch (see ReplayTimeline). While it is
/// active the overlays paint the replay instead of their finished annotations. Pages replay in
/// parallel (e.g. the whiteboard screen and the desktop screen at the same time).
class ReplayPlayer final : public QObject {
    Q_OBJECT

public:
    explicit ReplayPlayer(QObject* parent = nullptr);

    [[nodiscard]] bool isActive() const noexcept { return m_timer.isActive(); }

    /// Starts replaying the current items of @p pages. Returns false (and does nothing) when
    /// there is nothing to replay.
    bool start(const QList<const Document*>& pages, qreal speed);
    void stop();

    /// Paints @p page as it looks at the current replay time. Painter in document coordinates.
    void paint(QPainter& painter, const Document* page, const QRectF& area) const;

signals:
    void activeChanged(bool active);
    /// Time advanced: overlays repaint.
    void frame();

private:
    void tick();

    QHash<const Document*, ReplayTimeline> m_timelines;
    QTimer m_timer;
    QElapsedTimer m_clock;
    qint64 m_now = 0;
    qint64 m_durationMs = 0;
};

} // namespace recrayon
