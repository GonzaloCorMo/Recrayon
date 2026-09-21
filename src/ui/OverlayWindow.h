#pragma once

#include <QColor>
#include <QPixmap>
#include <QPointF>
#include <QPointer>
#include <QTimer>
#include <QWidget>

#include <optional>

class QScreen;

namespace recrayon {

class Document;
class PointerHighlight;
class RegionPicker;
class ReplayPlayer;
class TextEditBox;
class ToolController;
struct TextRequest;

/// Frameless, translucent, always-on-top window covering exactly one screen.
///
/// In draw mode it receives pointer input and forwards it (in document coordinates) to the
/// ToolController. In interact mode it is transparent for input, so clicks reach the
/// applications underneath while the annotations stay visible.
///
/// Committed items are rendered into a pixmap cache that is rebuilt only when the document
/// changes; the in-progress preview item is painted on top every frame.
class OverlayWindow final : public QWidget {
    Q_OBJECT

public:
    OverlayWindow(QScreen* screen, Document& document, ToolController& tools,
                  PointerHighlight& pointerHighlight, RegionPicker& picker, ReplayPlayer& replay);

    [[nodiscard]] QScreen* targetScreen() const noexcept { return m_screen; }

    [[nodiscard]] bool isDrawingEnabled() const noexcept { return m_drawing; }
    void setDrawingEnabled(bool enabled);

    /// While picking a capture area the overlay takes the pointer (like draw mode) and routes
    /// it to the RegionPicker instead of the drawing tools.
    void setPickingEnabled(bool enabled);

    /// Page painted by this overlay (desktop annotations or the whiteboard).
    void setDocument(Document& document);
    [[nodiscard]] const Document& document() const noexcept { return *m_document; }

    /// Opaque background (whiteboard) or none (transparent overlay over the desktop).
    void setBackground(const std::optional<QColor>& color);

    /// Shows the in-place text editor for a text tool. The request anchor is in document
    /// coordinates and must be on this overlay's screen.
    void beginTextInput(const TextRequest& request, const QFont& font, const QColor& color);
    void endTextInput();

    /// The draw-mode frame is hidden while taking screenshots or recording.
    void setFrameVisible(bool visible);

    /// While recording, repaint an invisible pixel at the video frame rate. Screen capture
    /// backends only deliver frames when the screen content changes, so a static screen would
    /// otherwise produce a video that is only a few frames long. 0 turns it off.
    void setCaptureHeartbeat(int framesPerSecond);

    /// Brings this overlay to the front and gives it the keyboard (for Esc and tool keys).
    void takeFocus();

signals:
    void escapePressed();
    /// The overlay moved to the top of the window stack: the toolbar must go above it again.
    void raised();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void syncGeometry();
    void applyInputMode();
    void rebuildCache();
    void onDocumentChanged(const QRectF& dirtyRect);
    void onPreviewChanged(const QRectF& dirtyRect);
    void updateCursor();
    void paintModeHint(QPainter& painter);
    [[nodiscard]] bool acceptsInput() const noexcept { return m_drawing || m_picking; }

    [[nodiscard]] QPointF toDocument(const QPointF& local) const { return local + m_origin; }
    [[nodiscard]] QRect toLocal(const QRectF& documentRect) const;

    QScreen* m_screen;
    Document* m_document;
    ToolController& m_tools;
    PointerHighlight& m_pointerHighlight;
    RegionPicker& m_picker;
    ReplayPlayer& m_replay;
    QPointF m_origin; // top-left of the screen in virtual-desktop coordinates
    QPixmap m_cache;
    bool m_cacheDirty = true;
    bool m_drawing = false;
    bool m_pressed = false;
    bool m_picking = false;
    std::optional<QColor> m_background;
    QPointer<TextEditBox> m_textEdit;
    bool m_frameVisible = true;
    QTimer m_heartbeat;
    bool m_heartbeatPhase = false;
};

} // namespace recrayon
