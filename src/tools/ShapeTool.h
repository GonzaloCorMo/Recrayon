#pragma once

#include "core/ShapeItem.h"
#include "tools/Tool.h"

#include <memory>

namespace recrayon {

/// Drag-to-size shapes: line, arrow, rectangle, ellipse.
class ShapeTool final : public Tool {
public:
    explicit ShapeTool(ShapeKind kind) noexcept : m_kind(kind) {}

    void begin(const QPointF& pos, const ToolContext& context) override;
    void update(const QPointF& pos, const ToolContext& context) override;
    void end(const QPointF& pos, const ToolContext& context) override;
    void cancel() override;
    [[nodiscard]] const Item* preview() const override { return m_item.get(); }

private:
    ShapeKind m_kind;
    std::shared_ptr<ShapeItem> m_item;
};

} // namespace recrayon
