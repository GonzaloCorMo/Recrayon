#pragma once

#include <QImage>
#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QTimer>

#include <functional>

class QPainter;

namespace recrayon {

enum class PointerHighlightMode {
    Off,
    Spotlight, ///< darken everything except a circle around the pointer
    Halo,      ///< translucent circle around the pointer
};

/// Tracks the global pointer position and paints pointer-related decorations on the overlays:
/// a highlight (spotlight / halo) and, optionally, a replica of the real mouse cursor.
///
/// The replica exists for screen recordings: Qt's screen capture leaves the cursor out, but it
/// does capture the overlays. The replica sits exactly under the real cursor, so on screen it is
/// hidden by it.
///
/// Works in both interaction modes: in Interact mode the overlays receive no mouse events, so
/// the position is polled from QCursor::pos() at display rate while anything is shown.
class PointerHighlight final : public QObject {
    Q_OBJECT

public:
    /// Picture of the system cursor; image has its devicePixelRatio set, hotspot is logical.
    struct CursorImage {
        QImage image;
        QPointF hotspot;
        bool visible = false;
    };
    using CursorProvider = std::function<CursorImage()>;

    explicit PointerHighlight(QObject* parent = nullptr);

    [[nodiscard]] PointerHighlightMode mode() const noexcept { return m_mode; }
    void setMode(PointerHighlightMode mode);

    /// Source of the cursor picture (platform specific, injected by the application).
    void setCursorProvider(CursorProvider provider);
    [[nodiscard]] bool isCursorReplicaEnabled() const noexcept { return m_replicaEnabled; }
    void setCursorReplicaEnabled(bool enabled);

    [[nodiscard]] QPointF position() const noexcept { return m_position; }

    /// True when paint() would draw anything.
    [[nodiscard]] bool isActive() const noexcept;

    /// Paints the highlight and the cursor replica. The painter uses document coordinates;
    /// @p area is the part of the document covered by the calling overlay.
    void paint(QPainter& painter, const QRectF& area) const;

signals:
    void modeChanged(recrayon::PointerHighlightMode mode);
    /// Something moved or changed; @p dirtyRect (document coordinates) needs repainting.
    void moved(const QRectF& dirtyRect);
    /// Visibility of all decorations changed; overlays repaint entirely.
    void appearanceChanged();

private:
    void poll();
    void updateTimer();
    [[nodiscard]] QRectF dirtyRect() const;

    QTimer m_timer;
    QPointF m_position;
    PointerHighlightMode m_mode = PointerHighlightMode::Off;
    CursorProvider m_cursorProvider;
    CursorImage m_cursor;
    bool m_replicaEnabled = false;
};

} // namespace recrayon
