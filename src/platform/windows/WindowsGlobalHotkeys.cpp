#include "platform/windows/WindowsGlobalHotkeys.h"

#include <QCoreApplication>
#include <QKeyCombination>
#include <QLoggingCategory>

#include <algorithm>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

Q_LOGGING_CATEGORY(lcWindowsHotkeys, "recrayon.platform.hotkeys")

namespace recrayon {

namespace {

UINT toNativeModifiers(Qt::KeyboardModifiers modifiers) {
    UINT native = MOD_NOREPEAT; // do not fire repeatedly while the keys are held
    if (modifiers.testFlag(Qt::ControlModifier)) {
        native |= MOD_CONTROL;
    }
    if (modifiers.testFlag(Qt::AltModifier)) {
        native |= MOD_ALT;
    }
    if (modifiers.testFlag(Qt::ShiftModifier)) {
        native |= MOD_SHIFT;
    }
    if (modifiers.testFlag(Qt::MetaModifier)) {
        native |= MOD_WIN;
    }
    return native;
}

/// Maps a Qt key to a Win32 virtual-key code. Returns 0 for unsupported keys.
UINT toVirtualKey(Qt::Key key) {
    // VK codes for letters and digits equal their upper-case ASCII values, as do Qt's.
    if ((key >= Qt::Key_A && key <= Qt::Key_Z) || (key >= Qt::Key_0 && key <= Qt::Key_9)) {
        return static_cast<UINT>(key);
    }
    if (key >= Qt::Key_F1 && key <= Qt::Key_F24) {
        return VK_F1 + static_cast<UINT>(key - Qt::Key_F1);
    }
    switch (key) {
    case Qt::Key_Space:
        return VK_SPACE;
    case Qt::Key_Escape:
        return VK_ESCAPE;
    case Qt::Key_Tab:
        return VK_TAB;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        return VK_RETURN;
    case Qt::Key_Backspace:
        return VK_BACK;
    case Qt::Key_Insert:
        return VK_INSERT;
    case Qt::Key_Delete:
        return VK_DELETE;
    case Qt::Key_Home:
        return VK_HOME;
    case Qt::Key_End:
        return VK_END;
    case Qt::Key_PageUp:
        return VK_PRIOR;
    case Qt::Key_PageDown:
        return VK_NEXT;
    case Qt::Key_Left:
        return VK_LEFT;
    case Qt::Key_Up:
        return VK_UP;
    case Qt::Key_Right:
        return VK_RIGHT;
    case Qt::Key_Down:
        return VK_DOWN;
    case Qt::Key_Print:
        return VK_SNAPSHOT;
    case Qt::Key_Pause:
        return VK_PAUSE;
    default:
        return 0;
    }
}

} // namespace

WindowsGlobalHotkeys::WindowsGlobalHotkeys() {
    QCoreApplication::instance()->installNativeEventFilter(this);
}

WindowsGlobalHotkeys::~WindowsGlobalHotkeys() {
    unregisterAll();
    if (QCoreApplication* app = QCoreApplication::instance()) {
        app->removeNativeEventFilter(this);
    }
}

bool WindowsGlobalHotkeys::registerHotkey(int id, const QKeySequence& sequence) {
    if (sequence.isEmpty()) {
        return false;
    }
    if (sequence.count() > 1) {
        qCWarning(lcWindowsHotkeys)
            << "Only the first chord of" << sequence.toString() << "is used for a global hotkey";
    }

    const QKeyCombination combination = sequence[0];
    const UINT virtualKey = toVirtualKey(combination.key());
    if (virtualKey == 0) {
        qCWarning(lcWindowsHotkeys)
            << "Unsupported key for a global hotkey:" << sequence.toString();
        return false;
    }

    if (!RegisterHotKey(nullptr, id, toNativeModifiers(combination.keyboardModifiers()),
                        virtualKey)) {
        qCWarning(lcWindowsHotkeys) << "RegisterHotKey failed for" << sequence.toString()
                                    << "- Win32 error" << GetLastError();
        return false;
    }
    m_ids.push_back(id);
    return true;
}

void WindowsGlobalHotkeys::unregisterAll() {
    for (const int id : m_ids) {
        UnregisterHotKey(nullptr, id);
    }
    m_ids.clear();
}

bool WindowsGlobalHotkeys::nativeEventFilter(const QByteArray& eventType, void* message,
                                             qintptr* /*result*/) {
    if (eventType != "windows_generic_MSG") {
        return false;
    }
    const auto* msg = static_cast<const MSG*>(message);
    if (msg->message != WM_HOTKEY) {
        return false;
    }
    const int id = static_cast<int>(msg->wParam);
    if (std::find(m_ids.cbegin(), m_ids.cend(), id) == m_ids.cend()) {
        return false;
    }
    emit activated(id);
    return true;
}

} // namespace recrayon
