#pragma once

#include "core/FreehandItem.h"
#include "tools/Tool.h"

#include <memory>

namespace recrayon {

/// Free-hand drawing. Used for the pen and the highlighter (they differ only in style) and for
/// the free arrow (a free-hand stroke that ends in an arrow head).
class PenTool final : public Tool {
public:
    explicit PenTool(FreehandItem::Ending ending = FreehandItem::Ending::None) noexcept
        : m_ending(ending) {}

    void begin(const QPointF& pos, const ToolContext& context) override;
    void update(const QPointF& pos, const ToolContext& context) override;
    void end(const QPointF& pos, const ToolContext& context) override;
    void cancel() override;
    [[nodiscard]] const Item* preview() const override { return m_item.get(); }

private:
    FreehandItem::Ending m_ending;
    std::shared_ptr<FreehandItem> m_item;
};

} // namespace recrayon
