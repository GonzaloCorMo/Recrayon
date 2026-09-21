#pragma once

#include "core/StrokeStyle.h"

#include <QFont>
#include <QPointF>
#include <QString>

#include <optional>

namespace recrayon {

class Document;
class Item;

/// Everything a tool needs during one gesture. Built by ToolController on press.
struct ToolContext {
    Document* document = nullptr;
    StrokeStyle style;
    QFont font; ///< for tools that create text
};

/// Where the text a tool asks for goes: a box placed next to @p anchor as described by
/// geometry::alignedBox().
struct TextRequest {
    QPointF anchor;
    Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignTop;
    qreal gap = 0.0;
};

/// Strategy interface: turns a press / move / release gesture into document changes.
///
/// Tools never touch widgets. Positions are in document coordinates (virtual desktop,
/// logical pixels). A gesture is always begin() → update()* → end() or cancel().
///
/// Text tools are two-phase: after end() they report pendingText() and wait until the UI calls
/// commitText() (or cancelText()) with what the user typed.
class Tool {
public:
    Tool() = default;
    virtual ~Tool() = default;

    Tool(const Tool&) = delete;
    Tool& operator=(const Tool&) = delete;
    Tool(Tool&&) = delete;
    Tool& operator=(Tool&&) = delete;

    virtual void begin(const QPointF& pos, const ToolContext& context) = 0;
    virtual void update(const QPointF& pos, const ToolContext& context) = 0;
    virtual void end(const QPointF& pos, const ToolContext& context) = 0;

    /// Aborts the gesture (and any pending text). Nothing uncommitted must be committed.
    virtual void cancel() = 0;

    /// Item under construction, painted on top of the document. Null when there is none.
    [[nodiscard]] virtual const Item* preview() const { return nullptr; }

    /// Set after end() when the tool now needs text from the user.
    [[nodiscard]] virtual std::optional<TextRequest> pendingText() const { return std::nullopt; }
    virtual void commitText(const QString& /*text*/, const ToolContext& /*context*/) {}
    /// The user dismissed the text input (Esc). The tool keeps whatever still makes sense.
    virtual void cancelText(const ToolContext& /*context*/) {}
};

} // namespace recrayon
