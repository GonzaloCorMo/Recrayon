#pragma once

#include "core/DesktopLayout.h"

class QScreen;

namespace recrayon::platform {

/// Logical and native geometry of @p screen.
///
/// Windows (per-monitor DPI): Qt keeps each screen's origin in native pixels and only scales
/// its size. Elsewhere logical coordinates scale uniformly with the device pixel ratio.
[[nodiscard]] ScreenMapping screenMapping(const QScreen* screen);

/// Mapping for every screen, in QGuiApplication::screens() order.
[[nodiscard]] DesktopLayout currentDesktopLayout();

} // namespace recrayon::platform
