#pragma once

#include "ui/AppActions.h"

#include <QMenu>
#include <QSystemTrayIcon>

namespace recrayon {

/// System tray / menu bar entry. Left click toggles draw mode; the context menu exposes the
/// application actions (useful when the toolbar is hidden or global hotkeys are unavailable).
class TrayIcon final : public QSystemTrayIcon {
    Q_OBJECT

public:
    explicit TrayIcon(const AppActions& actions, QObject* parent = nullptr);

private:
    QMenu m_menu;
};

} // namespace recrayon
