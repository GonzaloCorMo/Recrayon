#pragma once

#include <QImage>
#include <QPointF>

namespace recrayon::platform {

/// Picture of the mouse cursor as the OS currently shows it.
struct CursorSprite {
    QImage image;    ///< device pixels; devicePixelRatio() is set, so it paints at logical size
    QPointF hotspot; ///< logical offset of the click point inside the image
    /// False while the OS hides the cursor (e.g. "hide pointer while typing"); image is still
    /// filled then, so callers may decide to draw it anyway.
    bool visible = false;
};

/// Current system cursor; its shape follows the app under the pointer (arrow, I-beam, hand...).
/// Returns an empty image when there is no cursor at all.
///
/// Screen grabs and Qt's screen capture leave the cursor out, so callers paint this sprite
/// where they want it to appear. Windows reads the real cursor; other platforms get a generic
/// arrow until they have a native implementation.
[[nodiscard]] CursorSprite currentCursorSprite();

} // namespace recrayon::platform
