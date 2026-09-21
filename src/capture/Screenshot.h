#pragma once

#include "core/DesktopLayout.h"

#include <QImage>
#include <QPointF>
#include <QRect>
#include <QString>

namespace recrayon::capture {

/// A grabbed image covering an area of the virtual desktop, in native pixels.
struct Grab {
    QImage image;     ///< device pixels, devicePixelRatio 1
    QRect nativeArea; ///< what `image` shows, in native desktop coordinates
};

/// Grabs @p nativeArea exactly as displayed, overlays (annotations) included. Works for a
/// screen, all screens, a region or a window's bounds: every screen it touches is grabbed at
/// its native resolution and copied in place; parts outside every screen stay black. Windows
/// excluded from capture (the toolbar, see platform::setExcludedFromCapture) are left out.
[[nodiscard]] Grab grabNativeArea(const QRect& nativeArea);

/// Paints @p cursor (devicePixelRatio set, @p hotspot logical) with its hotspot at the logical
/// desktop position @p cursorPos, scaled to the pixel ratio of the screen it is on.
void paintCursor(Grab& grab, const QImage& cursor, const QPointF& hotspot, const QPointF& cursorPos,
                 const DesktopLayout& layout);

/// New timestamped file path "Recrayon_<date>.png" inside @p directory (created if needed).
[[nodiscard]] QString newScreenshotPath(const QString& directory);

/// New timestamped path *without extension* inside @p directory (created if needed); recorders
/// append a per-screen suffix and ".mp4".
[[nodiscard]] QString newRecordingBasePath(const QString& directory);

} // namespace recrayon::capture
