#include "tools/CalloutTool.h"

#include "core/CalloutItem.h"
#include "core/Document.h"
#include "core/Geometry.h"

namespace recrayon {

void CalloutTool::begin(const QPointF& pos, const ToolContext& context) {
    m_arrow = std::make_shared<ShapeItem>(ShapeKind::Arrow, context.style, pos, pos);
    m_pending = false;
}

void CalloutTool::update(const QPointF& pos, const ToolContext& /*context*/) {
    if (m_arrow && !m_pending) {
        m_arrow->setEnd(pos);
    }
}

void CalloutTool::end(const QPointF& pos, const ToolContext& /*context*/) {
    if (!m_arrow) {
        return;
    }
    m_arrow->setEnd(pos);
    if (m_arrow->isDegenerate()) {
        m_arrow.reset(); // a plain click is not an arrow
        return;
    }
    m_pending = true;
}

void CalloutTool::cancel() {
    m_arrow.reset();
    m_pending = false;
}

std::optional<TextRequest> CalloutTool::pendingText() const {
    if (!m_pending || !m_arrow) {
        return std::nullopt;
    }
    return TextRequest{m_arrow->start(),
                       geometry::calloutLabelAlignment(m_arrow->start(), m_arrow->end()),
                       CalloutItem::kLabelGap};
}

void CalloutTool::commitText(const QString& text, const ToolContext& context) {
    if (!m_pending || !m_arrow) {
        return;
    }
    if (text.trimmed().isEmpty()) {
        context.document->addItem(std::move(m_arrow));
    } else {
        context.document->addItem(std::make_shared<CalloutItem>(
            m_arrow->style(), context.font, m_arrow->start(), m_arrow->end(), text));
    }
    cancel();
}

void CalloutTool::cancelText(const ToolContext& context) {
    commitText(QString(), context); // keep the arrow the user already drew
}

} // namespace recrayon
