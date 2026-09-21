#pragma once

#include <QtGlobal>

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

namespace recrayon {

/// Values index ToolController's tool table; the display order is kAllTools.
enum class ToolKind {
    Pen,
    Highlighter,
    FreeArrow,
    Line,
    Arrow,
    Rectangle,
    Ellipse,
    Eraser,
    Move,
    Text,
    Callout,
};

inline constexpr std::size_t kToolCount = 11;

/// Tools in toolbar order, with the key that selects each one in draw mode.
struct ToolKey {
    ToolKind kind;
    Qt::Key key;
};

inline constexpr std::array<ToolKey, kToolCount> kToolKeys{{
    {ToolKind::Pen, Qt::Key_1},
    {ToolKind::Highlighter, Qt::Key_2},
    {ToolKind::FreeArrow, Qt::Key_3},
    {ToolKind::Line, Qt::Key_4},
    {ToolKind::Arrow, Qt::Key_5},
    {ToolKind::Callout, Qt::Key_6},
    {ToolKind::Rectangle, Qt::Key_7},
    {ToolKind::Ellipse, Qt::Key_8},
    {ToolKind::Text, Qt::Key_9},
    {ToolKind::Eraser, Qt::Key_0},
    {ToolKind::Move, Qt::Key_M},
}};

inline constexpr std::array kAllTools = [] {
    std::array<ToolKind, kToolCount> tools{};
    for (std::size_t i = 0; i < kToolCount; ++i) {
        tools[i] = kToolKeys[i].kind;
    }
    return tools;
}();

[[nodiscard]] constexpr std::size_t toIndex(ToolKind kind) noexcept {
    return static_cast<std::size_t>(kind);
}

/// Stable name stored in settings (e.g. the tools that return to the mouse). Never rename.
[[nodiscard]] constexpr const char* toolId(ToolKind kind) noexcept {
    switch (kind) {
    case ToolKind::Pen:
        return "pen";
    case ToolKind::Highlighter:
        return "highlighter";
    case ToolKind::FreeArrow:
        return "freeArrow";
    case ToolKind::Line:
        return "line";
    case ToolKind::Arrow:
        return "arrow";
    case ToolKind::Rectangle:
        return "rectangle";
    case ToolKind::Ellipse:
        return "ellipse";
    case ToolKind::Eraser:
        return "eraser";
    case ToolKind::Move:
        return "move";
    case ToolKind::Text:
        return "text";
    case ToolKind::Callout:
        return "callout";
    }
    return "";
}

/// Inverse of toolId(); empty for an unknown id.
[[nodiscard]] constexpr std::optional<ToolKind> toolFromId(std::string_view id) noexcept {
    for (const ToolKind kind : kAllTools) {
        if (id == toolId(kind)) {
            return kind;
        }
    }
    return std::nullopt;
}

/// Tools that place a new element (as opposed to the eraser and the move tool).
[[nodiscard]] constexpr bool isDrawingTool(ToolKind kind) noexcept {
    return kind != ToolKind::Eraser && kind != ToolKind::Move;
}

[[nodiscard]] constexpr Qt::Key shortcutKey(ToolKind kind) noexcept {
    for (const ToolKey& entry : kToolKeys) {
        if (entry.kind == kind) {
            return entry.key;
        }
    }
    return Qt::Key_unknown;
}

} // namespace recrayon
