#include "platform/GlobalHotkeys.h"

#if defined(Q_OS_WIN)
#include "platform/windows/WindowsGlobalHotkeys.h"
#endif

namespace recrayon {

namespace {

/// Used on platforms without a backend yet. The toolbar and the tray
/// icon remain fully functional.
class NullGlobalHotkeys final : public GlobalHotkeys {
public:
    NullGlobalHotkeys() = default;

    [[nodiscard]] bool isSupported() const override { return false; }
    bool registerHotkey(int /*id*/, const QKeySequence& /*sequence*/) override { return false; }
    void unregisterAll() override {}
};

} // namespace

GlobalHotkeys::GlobalHotkeys() = default;
GlobalHotkeys::~GlobalHotkeys() = default;

std::unique_ptr<GlobalHotkeys> GlobalHotkeys::create() {
#if defined(Q_OS_WIN)
    return std::make_unique<WindowsGlobalHotkeys>();
#else
    return std::make_unique<NullGlobalHotkeys>();
#endif
}

} // namespace recrayon
