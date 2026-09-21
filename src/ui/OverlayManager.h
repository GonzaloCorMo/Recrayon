#pragma once

#include <QColor>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QScreen>

#include <memory>
#include <optional>
#include <vector>

class QAction;
class QScreen;

namespace recrayon {

class Document;
class OverlayWindow;
class PointerHighlight;
class RegionPicker;
class ReplayPlayer;
class ToolController;

enum class InteractionMode {
    Interact, ///< overlays are click-through; annotations visible, apps usable
    Draw,     ///< overlays capture the pointer and draw
};

/// Keeps one OverlayWindow per screen (following hot-plugged monitors) and owns the global
/// interaction mode and annotation visibility.
class OverlayManager final : public QObject {
    Q_OBJECT

public:
    OverlayManager(Document& document, ToolController& tools, PointerHighlight& pointerHighlight,
                   RegionPicker& picker, ReplayPlayer& replay, QObject* parent = nullptr);
    ~OverlayManager() override;

    [[nodiscard]] InteractionMode mode() const noexcept { return m_mode; }
    void setMode(InteractionMode mode);

    [[nodiscard]] bool annotationsVisible() const noexcept { return m_visible; }
    /// Hiding the annotations also leaves draw mode.
    void setAnnotationsVisible(bool visible);

    /// Actions (with keyboard shortcuts) that should work while an overlay has focus.
    void setSharedActions(const QList<QAction*>& actions);

    /// Shows or hides the draw-mode frame (a UI hint that must not end up in captures).
    void setDrawModeFrameVisible(bool visible);

    /// Routes the pointer on every screen to the RegionPicker (leaving draw mode). Annotations
    /// are shown for the duration if they were hidden, and hidden again afterwards.
    void setPicking(bool picking);
    [[nodiscard]] bool isPicking() const noexcept { return m_picking; }

    /// Shows the whiteboard @p page with an opaque @p color on @p screen, or on every screen
    /// when @p screen is null. The other screens keep showing the desktop annotations.
    void showWhiteboard(Document& page, const QColor& color, QScreen* screen);
    void setWhiteboardColor(const QColor& color);
    void hideWhiteboard();

    /// Gives the keyboard to the overlay under the pointer, so Esc and the tool keys work even
    /// when the click came from the toolbar.
    void focusOverlayUnderCursor();

    /// Pages currently shown by at least one overlay (for replays).
    [[nodiscard]] QList<const Document*> visiblePages() const;

    /// See OverlayWindow::setCaptureHeartbeat().
    void setCaptureHeartbeat(int framesPerSecond);

    /// Shows the overlays for the first time. Before this call nothing is on screen.
    void showOverlays();

signals:
    void modeChanged(recrayon::InteractionMode mode);
    /// An overlay came to the front; the toolbar must be raised above it again.
    void overlayRaised();
    void visibilityChanged(bool visible);

private:
    void addOverlay(QScreen* screen);
    void removeOverlay(QScreen* screen);
    [[nodiscard]] OverlayWindow* overlayUnderCursor() const;
    [[nodiscard]] OverlayWindow* overlayAt(const QPointF& documentPos) const;
    /// Gives @p overlay the page and background it should show.
    void applyPage(OverlayWindow& overlay) const;

    Document& m_desktopDocument;
    ToolController& m_tools;
    PointerHighlight& m_pointerHighlight;
    RegionPicker& m_picker;
    ReplayPlayer& m_replay;
    std::vector<std::unique_ptr<OverlayWindow>> m_overlays;
    QList<QAction*> m_sharedActions;
    InteractionMode m_mode = InteractionMode::Interact;
    bool m_visible = true;
    bool m_shown = false;
    bool m_frameVisible = true;
    int m_heartbeat = 0; ///< frames per second, 0 = off
    bool m_picking = false;
    struct Whiteboard {
        Document* page = nullptr;
        QColor color;
        QPointer<QScreen> screen; ///< null = every screen
        bool allScreens = true;
    };
    std::optional<Whiteboard> m_whiteboard;
    bool m_visibleBeforePicking = true;
};

} // namespace recrayon
