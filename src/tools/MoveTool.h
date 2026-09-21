#pragma once

#include "core/Item.h"
#include "tools/Tool.h"

namespace recrayon {

/// Drags the top-most item under the pointer.
///
/// The item moves live during the drag (Document::translateItem, not undoable); on release a
/// single undo step is recorded for the whole displacement. Cancelling moves it back.
class MoveTool final : public Tool {
public:
    void begin(const QPointF& pos, const ToolContext& context) override;
    void update(const QPointF& pos, const ToolContext& context) override;
    void end(const QPointF& pos, const ToolContext& context) override;
    void cancel() override;

    /// Item currently being dragged (null when the press did not hit anything).
    [[nodiscard]] const Item* target() const noexcept { return m_item.get(); }

private:
    void reset();

    Document* m_document = nullptr;
    ItemPtr m_item;
    QPointF m_lastPos;
    QPointF m_totalDelta;
};

} // namespace recrayon
