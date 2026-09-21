#pragma once

#include <QRect>
#include <QString>

#include <optional>

namespace recrayon::platform {

/// A top-level window of another application.
struct WindowInfo {
    quintptr id = 0;    ///< opaque native handle (HWND on Windows)
    QRect nativeBounds; ///< visible frame, native desktop pixels (no invisible resize borders)
    QString title;
};

/// True when windowAt() / windowBounds() are implemented on this platform.
[[nodiscard]] bool canPickWindows();

/// Top-most visible window of another process at @p nativePos. Our own windows (overlays,
/// toolbar) and the desktop background are skipped.
[[nodiscard]] std::optional<WindowInfo> windowAt(const QPoint& nativePos);

/// Current visible bounds of window @p id; empty when it was closed or minimized.
[[nodiscard]] std::optional<QRect> windowBounds(quintptr id);

} // namespace recrayon::platform
