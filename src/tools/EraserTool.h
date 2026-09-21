#pragma once

#include "tools/Tool.h"

namespace recrayon {

/// Object eraser: removes whole items the pointer passes over.
///
/// Everything erased during one gesture is grouped into a single undo step (QUndoStack macro).
/// The macro is only opened on the first hit, so a gesture that erases nothing leaves no
/// empty entry in the history. The erase radius is half the context style width.
class EraserTool final : public Tool {
public:
    void begin(const QPointF& pos, const ToolContext& context) override;
    void update(const QPointF& pos, const ToolContext& context) override;
    void end(const QPointF& pos, const ToolContext& context) override;
    void cancel() override;

private:
    void eraseAt(const QPointF& pos, const ToolContext& context);
    void closeMacro();

    Document* m_document = nullptr;
    QPointF m_lastPos;
    bool m_macroOpen = false;
};

} // namespace recrayon
