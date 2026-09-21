#include "platform/CaptureExclusion.h"

#include <QWindow>

#if defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// Not declared by older SDK / MinGW headers.
#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif
#endif

namespace recrayon::platform {

bool setExcludedFromCapture(QWindow* window, bool excluded) {
    if (!window) {
        return false;
    }
#if defined(Q_OS_WIN)
    const auto hwnd = reinterpret_cast<HWND>(window->winId());
    return SetWindowDisplayAffinity(hwnd, excluded ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE) != 0;
#else
    Q_UNUSED(excluded)
    return false;
#endif
}

} // namespace recrayon::platform
