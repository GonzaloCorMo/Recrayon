#include "tools/PenTool.h"

#include "core/Document.h"

namespace recrayon {

void PenTool::begin(const QPointF& pos, const ToolContext& context) {
    m_item = std::make_shared<FreehandItem>(context.style, m_ending);
    m_item->addPoint(pos);
}

void PenTool::update(const QPointF& pos, const ToolContext& /*context*/) {
    if (m_item) {
        m_item->addPoint(pos);
    }
}

void PenTool::end(const QPointF& pos, const ToolContext& context) {
    if (!m_item) {
        return;
    }
    m_item->addPoint(pos);
    context.document->addItem(std::move(m_item));
    m_item.reset();
}

void PenTool::cancel() {
    m_item.reset();
}

} // namespace recrayon
