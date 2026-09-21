#include "tools/MoveTool.h"

#include "core/Document.h"

namespace recrayon {

namespace {
/// Extra reach so thin strokes are easy to grab.
constexpr qreal kGrabTolerance = 6.0;
} // namespace

void MoveTool::begin(const QPointF& pos, const ToolContext& context) {
    reset();
    m_document = context.document;
    m_lastPos = pos;

    // Items are stored back to front: search from the end to grab what is visually on top.
    const auto& items = m_document->items();
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
        if ((*it)->hitTest(pos, kGrabTolerance)) {
            m_item = *it;
            break;
        }
    }
}

void MoveTool::update(const QPointF& pos, const ToolContext& /*context*/) {
    if (!m_item) {
        return;
    }
    const QPointF delta = pos - m_lastPos;
    if (delta.isNull()) {
        return;
    }
    m_document->translateItem(*m_item, delta);
    m_totalDelta += delta;
    m_lastPos = pos;
}

void MoveTool::end(const QPointF& pos, const ToolContext& context) {
    update(pos, context);
    if (m_item) {
        m_document->recordMove(m_item, m_totalDelta);
    }
    reset();
}

void MoveTool::cancel() {
    if (m_item && !m_totalDelta.isNull()) {
        m_document->translateItem(*m_item, -m_totalDelta);
    }
    reset();
}

void MoveTool::reset() {
    m_document = nullptr;
    m_item.reset();
    m_totalDelta = {};
}

} // namespace recrayon
