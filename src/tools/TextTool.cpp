#include "tools/TextTool.h"

#include "core/Document.h"
#include "core/TextItem.h"

#include <memory>

namespace recrayon {

void TextTool::begin(const QPointF& pos, const ToolContext& /*context*/) {
    m_anchor = pos;
    m_pending = false;
}

void TextTool::update(const QPointF& /*pos*/, const ToolContext& /*context*/) {}

void TextTool::end(const QPointF& /*pos*/, const ToolContext& /*context*/) {
    m_pending = true;
}

void TextTool::cancel() {
    m_pending = false;
}

std::optional<TextRequest> TextTool::pendingText() const {
    if (!m_pending) {
        return std::nullopt;
    }
    return TextRequest{m_anchor, Qt::AlignLeft | Qt::AlignTop, 0.0};
}

void TextTool::commitText(const QString& text, const ToolContext& context) {
    if (!m_pending) {
        return;
    }
    m_pending = false;
    if (!text.trimmed().isEmpty()) {
        context.document->addItem(std::make_shared<TextItem>(
            context.style, context.font, m_anchor, Qt::AlignLeft | Qt::AlignTop, 0.0, text));
    }
}

void TextTool::cancelText(const ToolContext& /*context*/) {
    m_pending = false;
}

} // namespace recrayon
