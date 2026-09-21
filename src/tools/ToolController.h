#pragma once

#include "core/StrokeStyle.h"
#include "tools/Tool.h"
#include "tools/ToolKind.h"

#include <QColor>
#include <QFont>
#include <QObject>
#include <QRectF>

#include <array>
#include <memory>

namespace recrayon {

class Document;
class Item;

/// Owns the tool instances and the current drawing settings, and routes pointer input from the
/// overlays to the active tool. Overlay windows only forward events; all drawing logic lives here
/// and in the Tool implementations.
///
/// Text tools: after the gesture the controller emits textRequested(); the UI shows an editor
/// and answers with commitText() or cancelText().
class ToolController final : public QObject {
    Q_OBJECT

public:
    explicit ToolController(Document& document, QObject* parent = nullptr);
    ~ToolController() override;

    /// Document new items go to (desktop annotations or the whiteboard page). Overlays set it
    /// to their own page before each gesture. Switching cancels a gesture or pending text.
    void setDocument(Document& document);
    [[nodiscard]] Document& document() const noexcept { return *m_document; }

    [[nodiscard]] ToolKind currentTool() const noexcept { return m_current; }
    /// Switching tools cancels a gesture in progress.
    void setCurrentTool(ToolKind kind);

    [[nodiscard]] QColor color() const { return m_color; }
    void setColor(const QColor& color);

    [[nodiscard]] qreal width() const noexcept { return m_width; }
    /// Base stroke width in logical pixels, clamped to [kMinWidth, kMaxWidth].
    void setWidth(qreal width);

    [[nodiscard]] QFont textFont() const { return m_font; }
    void setTextFont(const QFont& font);

    /// Style the current tool applies to a new gesture (e.g. the highlighter is wider and
    /// translucent; the eraser uses the width as its erase diameter).
    [[nodiscard]] StrokeStyle effectiveStyle() const;

    // ---- Pointer input, in document coordinates ----
    void press(const QPointF& pos);
    void move(const QPointF& pos);
    void release(const QPointF& pos);
    /// Cancels the gesture. Pending text is kept: see commitPendingText().
    void cancel();

    // ---- Text input answer from the UI ----
    [[nodiscard]] bool isAwaitingText() const noexcept { return m_awaitingText; }
    void commitText(const QString& text);
    void cancelText();
    /// Ends pending text keeping what was typed: asks the editor (textCommitRequested) and, if
    /// nobody answers, closes the input. Used before switching tools, pages, replays...
    void commitPendingText();

    [[nodiscard]] bool isGestureActive() const noexcept { return m_gestureActive; }
    [[nodiscard]] const Item* preview() const;

    static constexpr qreal kMinWidth = 1.0;
    static constexpr qreal kMaxWidth = 64.0;

signals:
    void toolChanged(recrayon::ToolKind kind);
    /// A gesture of @p kind changed the document (an element was placed, erased or moved).
    /// Clicks that add nothing and empty texts do not count.
    void gestureCommitted(recrayon::ToolKind kind);
    /// The page being edited changed (e.g. the user started drawing on the whiteboard screen).
    void documentChanged(recrayon::Document& document);
    void colorChanged(const QColor& color);
    void widthChanged(qreal width);
    /// The preview item changed; @p dirtyRect (document coordinates) needs repainting.
    void previewChanged(const QRectF& dirtyRect);
    /// A text tool needs text: show an editor at @p request with @p font and @p color.
    void textRequested(const recrayon::TextRequest& request, const QFont& font,
                       const QColor& color);
    /// The controller needs the pending text now: the editor must call commitText() at once.
    void textCommitRequested();
    /// The pending text was committed or cancelled (possibly not by the editor itself): close it.
    void textInputClosed();

private:
    [[nodiscard]] Tool& currentToolImpl() const;
    [[nodiscard]] QRectF previewRect() const;
    void notifyPreviewChanged(const QRectF& before);
    void finishText(bool commit, const QString& text);
    void notifyIfCommitted();

    Document* m_document;
    std::array<std::unique_ptr<Tool>, kToolCount> m_tools;
    ToolKind m_current = ToolKind::Pen;
    QColor m_color{0xE5, 0x39, 0x35};
    qreal m_width = 4.0;
    QFont m_font;
    ToolContext m_context;
    bool m_gestureActive = false;
    bool m_awaitingText = false;
    int m_undoIndexAtPress = 0;
};

} // namespace recrayon
