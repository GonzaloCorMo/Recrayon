#include "tools/ToolController.h"

#include "core/Document.h"
#include "core/Item.h"
#include "tools/CalloutTool.h"
#include "tools/EraserTool.h"
#include "tools/MoveTool.h"
#include "tools/PenTool.h"
#include "tools/ShapeTool.h"
#include "tools/TextTool.h"

#include <QUndoStack>

#include <algorithm>

namespace recrayon {

namespace {
constexpr int kHighlighterAlpha = 90;
constexpr qreal kHighlighterWidthFactor = 4.0;
constexpr qreal kEraserWidthFactor = 3.0;
constexpr qreal kMinEraserWidth = 12.0;
constexpr int kDefaultTextPixelSize = 28;
} // namespace

ToolController::ToolController(Document& document, QObject* parent)
    : QObject(parent), m_document(&document) {
    m_tools[toIndex(ToolKind::Pen)] = std::make_unique<PenTool>();
    m_tools[toIndex(ToolKind::Highlighter)] = std::make_unique<PenTool>();
    m_tools[toIndex(ToolKind::FreeArrow)] = std::make_unique<PenTool>(FreehandItem::Ending::Arrow);
    m_tools[toIndex(ToolKind::Line)] = std::make_unique<ShapeTool>(ShapeKind::Line);
    m_tools[toIndex(ToolKind::Arrow)] = std::make_unique<ShapeTool>(ShapeKind::Arrow);
    m_tools[toIndex(ToolKind::Rectangle)] = std::make_unique<ShapeTool>(ShapeKind::Rectangle);
    m_tools[toIndex(ToolKind::Ellipse)] = std::make_unique<ShapeTool>(ShapeKind::Ellipse);
    m_tools[toIndex(ToolKind::Eraser)] = std::make_unique<EraserTool>();
    m_tools[toIndex(ToolKind::Move)] = std::make_unique<MoveTool>();
    m_tools[toIndex(ToolKind::Text)] = std::make_unique<TextTool>();
    m_tools[toIndex(ToolKind::Callout)] = std::make_unique<CalloutTool>();
    Q_ASSERT(std::all_of(m_tools.cbegin(), m_tools.cend(),
                         [](const auto& tool) { return tool != nullptr; }));
    m_font.setPixelSize(kDefaultTextPixelSize);
}

ToolController::~ToolController() = default;

void ToolController::setDocument(Document& document) {
    if (&document == m_document) {
        return;
    }
    cancel();
    m_document = &document;
    emit documentChanged(document);
}

void ToolController::setCurrentTool(ToolKind kind) {
    if (kind == m_current) {
        return;
    }
    cancel();
    m_current = kind;
    emit toolChanged(kind);
}

void ToolController::setColor(const QColor& color) {
    if (!color.isValid() || color == m_color) {
        return;
    }
    m_color = color;
    emit colorChanged(color);
}

void ToolController::setWidth(qreal width) {
    const qreal clamped = std::clamp(width, kMinWidth, kMaxWidth);
    if (qFuzzyCompare(clamped, m_width)) {
        return;
    }
    m_width = clamped;
    emit widthChanged(clamped);
}

void ToolController::setTextFont(const QFont& font) {
    m_font = font;
}

StrokeStyle ToolController::effectiveStyle() const {
    StrokeStyle style{m_color, m_width};
    switch (m_current) {
    case ToolKind::Highlighter:
        style.color.setAlpha(kHighlighterAlpha);
        style.width = m_width * kHighlighterWidthFactor;
        break;
    case ToolKind::Eraser:
        style.width = std::max(m_width * kEraserWidthFactor, kMinEraserWidth);
        break;
    default:
        break;
    }
    return style;
}

void ToolController::press(const QPointF& pos) {
    // The UI commits its editor before new input; this only guards against a stale one.
    commitPendingText();
    if (m_gestureActive) {
        cancel();
    }
    const QRectF before = previewRect();
    m_context = ToolContext{m_document, effectiveStyle(), m_font};
    m_undoIndexAtPress = m_document->undoStack()->index();
    m_gestureActive = true;
    currentToolImpl().begin(pos, m_context);
    notifyPreviewChanged(before);
}

void ToolController::move(const QPointF& pos) {
    if (!m_gestureActive) {
        return;
    }
    const QRectF before = previewRect();
    currentToolImpl().update(pos, m_context);
    notifyPreviewChanged(before);
}

void ToolController::release(const QPointF& pos) {
    if (!m_gestureActive) {
        return;
    }
    const QRectF before = previewRect();
    Tool& tool = currentToolImpl();
    tool.end(pos, m_context);
    m_gestureActive = false;
    notifyPreviewChanged(before);

    if (const auto request = tool.pendingText()) {
        m_awaitingText = true;
        emit textRequested(*request, m_font, m_context.style.color);
        return;
    }
    notifyIfCommitted();
}

void ToolController::notifyIfCommitted() {
    if (m_context.document && m_context.document->undoStack()->index() != m_undoIndexAtPress) {
        emit gestureCommitted(m_current);
    }
}

void ToolController::cancel() {
    // Switching tools or pages, starting a replay or leaving draw mode must not lose what the
    // user typed: only Esc in the editor (cancelText()) discards it.
    commitPendingText();
    if (!m_gestureActive) {
        return;
    }
    const QRectF before = previewRect();
    currentToolImpl().cancel();
    m_gestureActive = false;
    notifyPreviewChanged(before);
}

void ToolController::commitText(const QString& text) {
    finishText(true, text);
}

void ToolController::cancelText() {
    finishText(false, {});
}

void ToolController::commitPendingText() {
    if (!m_awaitingText) {
        return;
    }
    emit textCommitRequested(); // the editor answers synchronously with commitText()
    if (m_awaitingText) {
        finishText(false, {}); // no editor listening
    }
}

void ToolController::finishText(bool commit, const QString& text) {
    if (!m_awaitingText) {
        return;
    }
    m_awaitingText = false;
    const QRectF before = previewRect();
    if (commit) {
        currentToolImpl().commitText(text, m_context);
    } else {
        currentToolImpl().cancelText(m_context);
    }
    notifyPreviewChanged(before);
    emit textInputClosed();
    notifyIfCommitted();
}

const Item* ToolController::preview() const {
    return currentToolImpl().preview();
}

Tool& ToolController::currentToolImpl() const {
    return *m_tools[toIndex(m_current)];
}

QRectF ToolController::previewRect() const {
    const Item* item = preview();
    return item ? item->boundingRect() : QRectF();
}

void ToolController::notifyPreviewChanged(const QRectF& before) {
    const QRectF dirty = before.united(previewRect());
    if (!dirty.isNull()) {
        emit previewChanged(dirty);
    }
}

} // namespace recrayon
