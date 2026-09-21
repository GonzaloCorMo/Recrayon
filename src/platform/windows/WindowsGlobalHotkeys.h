#pragma once

#include "platform/GlobalHotkeys.h"

#include <QAbstractNativeEventFilter>

#include <vector>

namespace recrayon {

/// Win32 backend: RegisterHotKey() with a null HWND posts WM_HOTKEY to the GUI thread's
/// message queue, which Qt hands to native event filters as "windows_generic_MSG".
class WindowsGlobalHotkeys final : public GlobalHotkeys, public QAbstractNativeEventFilter {
public:
    WindowsGlobalHotkeys();
    ~WindowsGlobalHotkeys() override;

    [[nodiscard]] bool isSupported() const override { return true; }
    bool registerHotkey(int id, const QKeySequence& sequence) override;
    void unregisterAll() override;

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

private:
    std::vector<int> m_ids;
};

} // namespace recrayon
