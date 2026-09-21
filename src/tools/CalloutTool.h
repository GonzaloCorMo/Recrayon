#pragma once

#include "core/ShapeItem.h"
#include "tools/Tool.h"

#include <memory>

namespace recrayon {

/// Arrow followed by text: drag to draw the arrow (head at the release point); on release the
/// tool asks for a label that will sit at the arrow's tail. Committing text creates one
/// CalloutItem; empty text or cancelling keeps just the arrow the user already drew.
class CalloutTool final : public Tool {
public:
    void begin(const QPointF& pos, const ToolContext& context) override;
    void update(const QPointF& pos, const ToolContext& context) override;
    void end(const QPointF& pos, const ToolContext& context) override;
    void cancel() override;
    [[nodiscard]] const Item* preview() const override { return m_arrow.get(); }

    [[nodiscard]] std::optional<TextRequest> pendingText() const override;
    void commitText(const QString& text, const ToolContext& context) override;
    void cancelText(const ToolContext& context) override;

private:
    std::shared_ptr<ShapeItem> m_arrow;
    bool m_pending = false;
};

} // namespace recrayon
