#pragma once

#include "config/SessionState.h"
#include "config/Settings.h"
#include "core/Document.h"
#include "tools/ToolController.h"
#include "ui/AppActions.h"
#include "ui/PointerHighlight.h"
#include "ui/RegionPicker.h"
#include "ui/ReplayPlayer.h"

#include <QMenu>
#include <QObject>
#include <QStringList>
#include <QSystemTrayIcon>
#include <QUndoGroup>

#include <memory>
#include <vector>

namespace recrayon {

class CaptureController;
class GlobalHotkeys;
class OverlayManager;
class Toolbar;
class TrayIcon;
enum class InteractionMode;

/// Composition root: creates the model, tools, windows and platform services and wires them
/// together. Screenshots and recordings are delegated to CaptureController.
class Application final : public QObject {
    Q_OBJECT

public:
    explicit Application(QObject* parent = nullptr);
    ~Application() override;

    /// Shows the overlays, toolbar and tray icon and registers global hotkeys.
    void start();

private:
    /// Action triggered by a global shortcut.
    struct ShortcutBinding {
        ShortcutId id;
        QAction* action;
    };

    void createActions();
    void createCaptureActions();
    void registerHotkeys();
    /// (Re)registers every global shortcut from m_settings and refreshes tooltips.
    /// Returns the shortcuts the OS refused (typically: taken by another application).
    QStringList applyShortcuts();
    /// Tooltip text of a shortcut's action (without the keys).
    [[nodiscard]] QString describe(ShortcutId id) const;
    void openSettings();
    /// Starts a new instance and quits this one (to apply a new language).
    void restart();
    void placeToolbar();
    /// Shows the toolbar again, whether it was minimized or hidden.
    void restoreToolbar();
    void onToolbarMinimized(bool minimized);
    /// Moves the toolbar fully inside the screen it is on; false if it is on no screen.
    bool keepToolbarOnScreen();
    /// Saves the toolbar position, stroke width and tool (SessionState).
    void saveSession();
    void onModeChanged(InteractionMode mode);
    void onPointerHighlightChanged(PointerHighlightMode mode);
    /// Shows the whiteboard page on the screen under the pointer or on every screen.
    void showWhiteboard(WhiteboardScope scope);
    void hideWhiteboard();
    void startReplay();
    /// Esc from the toolbar: stop the replay, leave the whiteboard or leave draw mode.
    void onEscapePressed();
    /// Keeps the toolbar above the overlays (both are top-most windows).
    void raiseToolbar();
    void onReplayChanged(bool active);
    /// Back to interact mode after placing an element, if the user chose that for the tool.
    void onGestureCommitted(ToolKind kind);
    [[nodiscard]] Document& activeDocument() noexcept;
    void updateClearEnabled();
    /// Text font, whiteboard color... from m_settings.
    void applyDrawingSettings();
    /// (Re)creates the toolbar from m_settings, keeping its position.
    void rebuildToolbar();
    void notify(const QString& title, const QString& message,
                QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information);

    // Declaration order matters: members are destroyed in reverse, so windows and services go
    // before the tools and the document they reference.
    Settings m_settings;
    SessionState m_session;
    Document m_desktopDocument;
    Document m_whiteboardDocument;
    QUndoGroup m_undoGroup; // undo/redo follow whichever page is active
    ToolController m_tools;
    PointerHighlight m_pointerHighlight;
    RegionPicker m_picker;
    ReplayPlayer m_replay;
    AppActions m_actions;
    std::vector<ShortcutBinding> m_shortcutBindings;
    std::unique_ptr<OverlayManager> m_overlays;
    std::unique_ptr<CaptureController> m_capture;
    std::unique_ptr<Toolbar> m_toolbar;
    std::unique_ptr<TrayIcon> m_tray;
    std::unique_ptr<GlobalHotkeys> m_hotkeys;
    std::unique_ptr<QMenu> m_whiteboardMenu;
    bool m_settingsOpen = false;
    bool m_whiteboardActive = false;
    bool m_drawingBeforeReplay = false;
};

} // namespace recrayon
