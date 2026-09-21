#include "tools/ShapeTool.h"

#include "core/Document.h"

namespace recrayon {

void ShapeTool::begin(const QPointF& pos, const ToolContext& context) {
    m_item = std::make_shared<ShapeItem>(m_kind, context.style, pos, pos);
}

void ShapeTool::update(const QPointF& pos, const ToolContext& /*context*/) {
    if (m_item) {
        m_item->setEnd(pos);
    }
}

void ShapeTool::end(const QPointF& pos, const ToolContext& context) {
    if (!m_item) {
        return;
    }
    m_item->setEnd(pos);
    if (!m_item->isDegenerate()) {
        context.document->addItem(std::move(m_item));
    }
    m_item.reset();
}

void ShapeTool::cancel() {
    m_item.reset();
}

} // namespace recrayon
