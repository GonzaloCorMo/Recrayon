#pragma once

class QAction;
class QMenu;

namespace recrayon {

/// Application-wide actions shared by the toolbar, the tray menu, overlays and global hotkeys.
/// Created and owned by Application; UI classes only hold non-owning pointers.
struct AppActions {
    QAction* toggleDrawing = nullptr;    ///< checkable; checked = draw mode
    QAction* toggleVisibility = nullptr; ///< checkable; checked = annotations visible
    QAction* toggleToolbar = nullptr;    ///< checkable; checked = toolbar shown (not minimized)
    QAction* minimize = nullptr;         ///< toolbar to the taskbar, app keeps running
    QAction* toggleWhiteboard = nullptr; ///< checkable; blank page (scope from settings)
    QMenu* whiteboardMenu = nullptr;     ///< whiteboard on this screen / on all screens
    QAction* undo = nullptr;
    QAction* redo = nullptr;
    QAction* clear = nullptr;
    QAction* toggleReplay = nullptr;     ///< checkable; redraw everything from scratch
    QAction* toggleSpotlight = nullptr;  ///< checkable; exclusive with toggleHalo
    QAction* toggleHalo = nullptr;       ///< checkable; exclusive with toggleSpotlight
    QAction* screenshot = nullptr;       ///< uses the default screenshot target
    QAction* screenshotRegion = nullptr; ///< asks for a region or window
    QAction* toggleRecording = nullptr;  ///< checkable; disabled without Qt Multimedia
    QAction* recordRegion = nullptr;     ///< asks for a region or window
    QMenu* screenshotMenu = nullptr;     ///< every screenshot target
    QMenu* recordingMenu = nullptr;      ///< every recording target
    QAction* settings = nullptr;
    QAction* quit = nullptr;
};

} // namespace recrayon
