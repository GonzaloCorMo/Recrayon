#include "tools/EraserTool.h"

#include "core/Document.h"

#include <QLineF>

#include <algorithm>
#include <cmath>
#include <vector>

namespace recrayon {

void EraserTool::begin(const QPointF& pos, const ToolContext& context) {
    m_document = context.document;
    m_macroOpen = false;
    m_lastPos = pos;
    eraseAt(pos, context);
}

void EraserTool::update(const QPointF& pos, const ToolContext& context) {
    // Sample along the segment so fast pointer moves do not skip thin strokes.
    const qreal step = std::max(1.0, context.style.width / 2.0);
    const QLineF segment(m_lastPos, pos);
    const int samples = std::max(1, static_cast<int>(std::ceil(segment.length() / step)));
    for (int i = 1; i <= samples; ++i) {
        eraseAt(segment.pointAt(static_cast<qreal>(i) / samples), context);
    }
    m_lastPos = pos;
}

void EraserTool::end(const QPointF& pos, const ToolContext& context) {
    update(pos, context);
    closeMacro();
}

void EraserTool::cancel() {
    // Items already erased stay erased (and undoable); we only close the history group.
    closeMacro();
}

void EraserTool::eraseAt(const QPointF& pos, const ToolContext& context) {
    const qreal radius = context.style.width / 2.0;
    std::vector<ItemPtr> hits;
    for (const ItemPtr& item : context.document->items()) {
        if (item->hitTest(pos, radius)) {
            hits.push_back(item);
        }
    }
    if (hits.empty()) {
        return;
    }
    if (!m_macroOpen) {
        context.document->undoStack()->beginMacro(Document::tr("Erase"));
        m_macroOpen = true;
    }
    context.document->removeItems(hits);
}

void EraserTool::closeMacro() {
    if (m_macroOpen && m_document) {
        m_document->undoStack()->endMacro();
    }
    m_macroOpen = false;
    m_document = nullptr;
}

} // namespace recrayon
