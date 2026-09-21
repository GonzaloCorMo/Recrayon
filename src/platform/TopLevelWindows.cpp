#include "platform/TopLevelWindows.h"

#include <iterator>

#if defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <dwmapi.h>
#include <windows.h>
#endif

namespace recrayon::platform {

#if defined(Q_OS_WIN)

namespace {

std::optional<QRect> visibleBounds(HWND hwnd) {
    RECT rect{};
    // The extended frame excludes the invisible resize borders Windows 10+ adds around windows.
    if (FAILED(DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(rect))) &&
        !GetWindowRect(hwnd, &rect)) {
        return std::nullopt;
    }
    const QRect bounds(QPoint(rect.left, rect.top), QPoint(rect.right - 1, rect.bottom - 1));
    if (bounds.isEmpty()) {
        return std::nullopt;
    }
    return bounds;
}

bool isCloaked(HWND hwnd) {
    // UWP apps and windows on other virtual desktops are "visible" but cloaked by DWM.
    DWORD cloaked = 0;
    return SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) &&
           cloaked != 0;
}

QString windowTitle(HWND hwnd) {
    wchar_t buffer[256] = {};
    const int length = GetWindowTextW(hwnd, buffer, static_cast<int>(std::size(buffer)));
    return QString::fromWCharArray(buffer, length);
}

bool isPickable(HWND hwnd) {
    if (!IsWindowVisible(hwnd) || IsIconic(hwnd) || isCloaked(hwnd)) {
        return false;
    }
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == GetCurrentProcessId()) {
        return false; // our overlays and toolbar
    }
    const LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TRANSPARENT) {
        return false; // other click-through overlays
    }
    wchar_t className[64] = {};
    GetClassNameW(hwnd, className, static_cast<int>(std::size(className)));
    const QString cls = QString::fromWCharArray(className);
    if (cls == QLatin1String("Progman") || cls == QLatin1String("WorkerW")) {
        return false; // desktop background
    }
    if ((exStyle & WS_EX_TOOLWINDOW) && windowTitle(hwnd).isEmpty()) {
        return false; // tooltips, shadows and similar helpers
    }
    return true;
}

struct SearchContext {
    POINT point;
    std::optional<WindowInfo> result;
};

BOOL CALLBACK findWindowAt(HWND hwnd, LPARAM param) {
    auto* context = reinterpret_cast<SearchContext*>(param);
    if (!isPickable(hwnd)) {
        return TRUE;
    }
    const auto bounds = visibleBounds(hwnd);
    if (!bounds || !bounds->contains(QPoint(context->point.x, context->point.y))) {
        return TRUE;
    }
    context->result = WindowInfo{reinterpret_cast<quintptr>(hwnd), *bounds, windowTitle(hwnd)};
    return FALSE; // EnumWindows walks top to bottom: the first hit is the visible one
}

} // namespace

bool canPickWindows() {
    return true;
}

std::optional<WindowInfo> windowAt(const QPoint& nativePos) {
    SearchContext context{{nativePos.x(), nativePos.y()}, std::nullopt};
    EnumWindows(findWindowAt, reinterpret_cast<LPARAM>(&context));
    return context.result;
}

std::optional<QRect> windowBounds(quintptr id) {
    const auto hwnd = reinterpret_cast<HWND>(id);
    if (!IsWindow(hwnd) || !IsWindowVisible(hwnd) || IsIconic(hwnd)) {
        return std::nullopt;
    }
    return visibleBounds(hwnd);
}

#else

bool canPickWindows() {
    return false;
}

std::optional<WindowInfo> windowAt(const QPoint& /*nativePos*/) {
    return std::nullopt;
}

std::optional<QRect> windowBounds(quintptr /*id*/) {
    return std::nullopt;
}

#endif

} // namespace recrayon::platform
