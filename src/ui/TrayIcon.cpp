#include "ui/TrayIcon.h"

#include "ui/Icons.h"

#include <QAction>

namespace recrayon {

TrayIcon::TrayIcon(const AppActions& actions, QObject* parent)
    : QSystemTrayIcon(icons::icon(icons::IconId::App), parent) {
    setToolTip(QStringLiteral("Recrayon"));

    m_menu.addAction(actions.toggleDrawing);
    m_menu.addAction(actions.toggleVisibility);
    m_menu.addAction(actions.toggleWhiteboard);
    if (actions.whiteboardMenu) {
        m_menu.addMenu(actions.whiteboardMenu);
    }
    m_menu.addAction(actions.toggleToolbar);
    if (actions.collapseToolbar) {
        m_menu.addAction(actions.collapseToolbar);
    }
    if (actions.minimize) {
        m_menu.addAction(actions.minimize); // to the taskbar; the toolbar button collapses instead
    }
    m_menu.addSeparator();
    m_menu.addAction(actions.undo);
    m_menu.addAction(actions.redo);
    m_menu.addAction(actions.clear);
    m_menu.addSeparator();
    m_menu.addAction(actions.toggleReplay);
    m_menu.addAction(actions.toggleSpotlight);
    m_menu.addAction(actions.toggleHalo);
    m_menu.addAction(actions.screenshot);
    if (actions.screenshotMenu) {
        m_menu.addMenu(actions.screenshotMenu);
    }
    m_menu.addAction(actions.toggleRecording);
    if (actions.recordingMenu) {
        m_menu.addMenu(actions.recordingMenu);
    }
    m_menu.addSeparator();
    m_menu.addAction(actions.settings);
    m_menu.addAction(actions.quit);
    setContextMenu(&m_menu);

    // Click: bring back a minimized or hidden toolbar; otherwise toggle draw mode.
    connect(this, &QSystemTrayIcon::activated, this,
            [draw = actions.toggleDrawing,
             toolbar = actions.toggleToolbar](QSystemTrayIcon::ActivationReason reason) {
                if (reason != QSystemTrayIcon::Trigger) {
                    return;
                }
                if (toolbar->isEnabled() && !toolbar->isChecked()) {
                    toolbar->setChecked(true);
                } else {
                    draw->toggle();
                }
            });
}

} // namespace recrayon
