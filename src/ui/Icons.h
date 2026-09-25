#pragma once

#include <QColor>
#include <QIcon>

namespace recrayon::icons {

enum class IconId {
    App,
    Cursor,
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
    Whiteboard,
    Play,
    Undo,
    Redo,
    Clear,
    Visibility,
    CustomColor,
    Screenshot,
    Record,
    Stop,
    Pause,
    Spotlight,
    Halo,
    Settings,
    Minimize,
    Collapse, ///< chevron pointing left; the toolbar rotates it per edge
    Quit,
};

/// Default glyph color, tuned for the dark toolbar background.
inline const QColor kGlyphColor{0xE8, 0xEA, 0xED};

/// Line-art icon painted at runtime (no image assets, crisp at any DPI).
[[nodiscard]] QIcon icon(IconId id, const QColor& color = kGlyphColor);

/// Filled circle used by the color picker.
[[nodiscard]] QIcon swatch(const QColor& color);

/// Dot whose size represents a stroke width.
[[nodiscard]] QIcon strokeWidth(qreal width, const QColor& color = kGlyphColor);

} // namespace recrayon::icons
