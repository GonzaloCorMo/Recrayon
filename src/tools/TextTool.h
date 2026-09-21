#pragma once

#include "tools/Tool.h"

namespace recrayon {

/// Click to place text: the text box's top-left corner goes where the user clicked. Empty text
/// adds nothing.
class TextTool final : public Tool {
public:
    void begin(const QPointF& pos, const ToolContext& context) override;
    void update(const QPointF& pos, const ToolContext& context) override;
    void end(const QPointF& pos, const ToolContext& context) override;
    void cancel() override;

    [[nodiscard]] std::optional<TextRequest> pendingText() const override;
    void commitText(const QString& text, const ToolContext& context) override;
    void cancelText(const ToolContext& context) override;

private:
    QPointF m_anchor;
    bool m_pending = false;
};

} // namespace recrayon
