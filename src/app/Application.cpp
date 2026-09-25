#include "app/Application.h"

#include "app/CaptureController.h"
#include "platform/CaptureExclusion.h"
#include "platform/CursorSprite.h"
#include "platform/DesktopGeometry.h"
#include "platform/GlobalHotkeys.h"
#include "platform/TopLevelWindows.h"
#include "ui/Icons.h"
#include "ui/OverlayManager.h"
#include "ui/SettingsDialog.h"
#include "ui/Toolbar.h"
#include "ui/TrayIcon.h"

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QCursor>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QProcess>
#include <QScreen>
#include <QSettings>
#include <QSignalBlocker>
#include <QUndoStack>
#include <QUrl>

Q_LOGGING_CATEGORY(lcApp, "recrayon.app")

#include <algorithm>

namespace recrayon {

namespace {

using icons::IconId;

QString withKeys(const QString& text, const QKeySequence& keys) {
    return QStringLiteral("%1 (%2)").arg(text, keys.toString(QKeySequence::NativeText));
}

int hotkeyId(ShortcutId id) {
    return static_cast<int>(id) + 1;
}

constexpr int kToolbarScreenMargin = 24;

} // namespace

Application::Application(QObject* parent) : QObject(parent), m_tools(m_desktopDocument) {
    {
        QSettings store;
        m_settings = Settings::load(store);
    }
    m_undoGroup.addStack(m_desktopDocument.undoStack());
    m_undoGroup.addStack(m_whiteboardDocument.undoStack());
    m_undoGroup.setActiveStack(m_desktopDocument.undoStack());
    applyDrawingSettings();
    m_tools.setColor(m_settings.initialColor());
    {
        QSettings store;
        m_session = SessionState::load(store);
    }
    if (m_session.strokeWidth) {
        m_tools.setWidth(*m_session.strokeWidth);
    }
    if (const auto tool = toolFromId(m_session.tool.toStdString())) {
        m_tools.setCurrentTool(*tool);
    }

    m_pointerHighlight.setCursorProvider([] {
        const platform::CursorSprite sprite = platform::currentCursorSprite();
        return PointerHighlight::CursorImage{sprite.image, sprite.hotspot, sprite.visible};
    });
    m_picker.setWindowLookup(
        [](const QPointF& logicalPos) -> std::optional<RegionPicker::WindowHit> {
            const DesktopLayout layout = platform::currentDesktopLayout();
            const auto window = platform::windowAt(layout.toNative(logicalPos).toPoint());
            if (!window) {
                return std::nullopt;
            }
            return RegionPicker::WindowHit{window->id, layout.toLogical(window->nativeBounds),
                                           window->title};
        });
    m_picker.setNativeSize([](const QRectF& logicalRect) {
        return platform::currentDesktopLayout().toNative(logicalRect).size();
    });

#if !defined(Q_OS_WIN)
    // Taskbar / dock icon of the toolbar window. Windows takes it from recrayon.exe (IDI_ICON1).
    QGuiApplication::setWindowIcon(icons::icon(IconId::App));
#endif
    createActions();

    m_overlays = std::make_unique<OverlayManager>(m_desktopDocument, m_tools, m_pointerHighlight,
                                                  m_picker, m_replay);
    m_overlays->setSharedActions({m_actions.undo, m_actions.redo});

    createCaptureActions();
    // Screenshots and recordings decide whether the toolbar shows up in them (Settings).
    m_capture->setToolbarExclusion([this](bool excluded) {
        m_toolbarExcludedFromCapture = excluded;
        if (m_toolbar && m_toolbar->windowHandle()) {
            platform::setExcludedFromCapture(m_toolbar->windowHandle(), excluded);
        }
    });

    rebuildToolbar();

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        m_tray = std::make_unique<TrayIcon>(m_actions);
        // Clicking "screenshot saved" / "recording saved" opens the folder it went to.
        connect(m_tray.get(), &QSystemTrayIcon::messageClicked, this,
                &Application::openNotificationFile);
    } else {
        qCInfo(lcApp) << "No system tray available; the toolbar can be minimized but not hidden";
        m_actions.toggleToolbar->setEnabled(false);
    }

    connect(m_overlays.get(), &OverlayManager::modeChanged, this, &Application::onModeChanged);
    connect(m_overlays.get(), &OverlayManager::visibilityChanged, m_actions.toggleVisibility,
            &QAction::setChecked);
    connect(&m_pointerHighlight, &PointerHighlight::modeChanged, this,
            &Application::onPointerHighlightChanged);
    connect(&m_replay, &ReplayPlayer::activeChanged, this, &Application::onReplayChanged);
    connect(&m_tools, &ToolController::gestureCommitted, this, &Application::onGestureCommitted);
    connect(&m_tools, &ToolController::toolChanged, this, &Application::saveSession);
    connect(&m_tools, &ToolController::widthChanged, this, &Application::saveSession);
    connect(qApp, &QCoreApplication::aboutToQuit, this, &Application::saveSession);

    // Toolbar and overlays are all top-most: whenever an overlay comes to the front (draw mode,
    // whiteboard, text editor) or the focus moves, put the toolbar back on top of it.
    connect(m_overlays.get(), &OverlayManager::overlayRaised, this, &Application::raiseToolbar);
    connect(qGuiApp, &QGuiApplication::focusWindowChanged, this, &Application::raiseToolbar);
}

Application::~Application() = default;

void Application::createActions() {
    m_actions.toggleDrawing = new QAction(icons::icon(IconId::Cursor), tr("Draw mode"), this);
    m_actions.toggleDrawing->setCheckable(true);
    connect(m_actions.toggleDrawing, &QAction::toggled, this, [this](bool on) {
        m_overlays->setMode(on ? InteractionMode::Draw : InteractionMode::Interact);
    });

    m_actions.toggleVisibility =
        new QAction(icons::icon(IconId::Visibility), tr("Show annotations"), this);
    m_actions.toggleVisibility->setCheckable(true);
    m_actions.toggleVisibility->setChecked(true);
    connect(m_actions.toggleVisibility, &QAction::toggled, this,
            [this](bool on) { m_overlays->setAnnotationsVisible(on); });

    m_actions.toggleToolbar = new QAction(tr("Show toolbar"), this);
    m_actions.toggleToolbar->setCheckable(true);
    m_actions.toggleToolbar->setChecked(true);
    connect(m_actions.toggleToolbar, &QAction::toggled, this, [this](bool on) {
        if (on) {
            restoreToolbar();
        } else {
            m_toolbar->hide();
        }
    });

    m_actions.collapseToolbar =
        new QAction(icons::icon(IconId::Collapse), tr("Collapse the toolbar"), this);
    m_actions.collapseToolbar->setToolTip(
        tr("Collapse the toolbar against the nearest edge; the arrow brings it back. Handy to "
           "keep it out of the way (and out of captures where it cannot be hidden)."));
    connect(m_actions.collapseToolbar, &QAction::triggered, this, [this] {
        if (m_toolbar) {
            m_toolbar->setCollapsed(true);
        }
    });

    m_actions.minimize = new QAction(icons::icon(IconId::Minimize), tr("Minimize"), this);
    m_actions.minimize->setToolTip(
        tr("Minimize: Recrayon keeps running; open it again from the taskbar or the tray icon"));
    connect(m_actions.minimize, &QAction::triggered, this, [this] { m_toolbar->showMinimized(); });

    m_actions.toggleWhiteboard =
        new QAction(icons::icon(IconId::Whiteboard), tr("Whiteboard"), this);
    m_actions.toggleWhiteboard->setCheckable(true);
    connect(m_actions.toggleWhiteboard, &QAction::toggled, this, [this](bool on) {
        if (on) {
            showWhiteboard(m_settings.whiteboardScope);
        } else {
            hideWhiteboard();
        }
    });
    m_whiteboardMenu = std::make_unique<QMenu>(tr("Whiteboard on"));
    m_whiteboardMenu->addAction(tr("The screen under the cursor"), this,
                                [this] { showWhiteboard(WhiteboardScope::ScreenUnderCursor); });
    m_whiteboardMenu->addAction(tr("All screens"), this,
                                [this] { showWhiteboard(WhiteboardScope::AllScreens); });
    m_actions.whiteboardMenu = m_whiteboardMenu.get();

    m_actions.undo = m_undoGroup.createUndoAction(this, tr("Undo"));
    m_actions.undo->setIcon(icons::icon(IconId::Undo));
    m_actions.undo->setShortcuts(QKeySequence::Undo);

    m_actions.redo = m_undoGroup.createRedoAction(this, tr("Redo"));
    m_actions.redo->setIcon(icons::icon(IconId::Redo));
    m_actions.redo->setShortcuts(QKeySequence::Redo);

    m_actions.clear = new QAction(icons::icon(IconId::Clear), tr("Clear all"), this);
    m_actions.clear->setEnabled(false);
    connect(m_actions.clear, &QAction::triggered, this, [this] { activeDocument().clear(); });
    connect(&m_desktopDocument, &Document::changed, this, &Application::updateClearEnabled);
    // Undo, redo and clear act on the page the user last drew on.
    connect(&m_tools, &ToolController::documentChanged, this, [this](Document& document) {
        m_undoGroup.setActiveStack(document.undoStack());
        updateClearEnabled();
    });
    connect(&m_whiteboardDocument, &Document::changed, this, &Application::updateClearEnabled);

    m_actions.toggleReplay = new QAction(icons::icon(IconId::Play), tr("Replay the drawing"), this);
    m_actions.toggleReplay->setCheckable(true);
    connect(m_actions.toggleReplay, &QAction::toggled, this, [this](bool on) {
        if (on) {
            startReplay();
        } else {
            m_replay.stop();
        }
    });

    m_actions.toggleSpotlight =
        new QAction(icons::icon(IconId::Spotlight), tr("Spotlight cursor"), this);
    m_actions.toggleSpotlight->setCheckable(true);
    connect(m_actions.toggleSpotlight, &QAction::toggled, this, [this](bool on) {
        m_pointerHighlight.setMode(on ? PointerHighlightMode::Spotlight
                                      : PointerHighlightMode::Off);
    });

    m_actions.toggleHalo = new QAction(icons::icon(IconId::Halo), tr("Highlight cursor"), this);
    m_actions.toggleHalo->setCheckable(true);
    connect(m_actions.toggleHalo, &QAction::toggled, this, [this](bool on) {
        m_pointerHighlight.setMode(on ? PointerHighlightMode::Halo : PointerHighlightMode::Off);
    });

    m_actions.settings = new QAction(icons::icon(IconId::Settings), tr("Settings…"), this);
    m_actions.settings->setToolTip(tr("Settings: shortcuts and capture options"));
    connect(m_actions.settings, &QAction::triggered, this, &Application::openSettings);

    m_actions.quit = new QAction(icons::icon(IconId::Quit), tr("Quit"), this);
    connect(m_actions.quit, &QAction::triggered, qApp, &QCoreApplication::quit);

    m_shortcutBindings.push_back({ShortcutId::ToggleDrawing, m_actions.toggleDrawing});
    m_shortcutBindings.push_back({ShortcutId::ToggleVisibility, m_actions.toggleVisibility});
    m_shortcutBindings.push_back({ShortcutId::Whiteboard, m_actions.toggleWhiteboard});
    m_shortcutBindings.push_back({ShortcutId::Clear, m_actions.clear});
    m_shortcutBindings.push_back({ShortcutId::Spotlight, m_actions.toggleSpotlight});
    m_shortcutBindings.push_back({ShortcutId::Halo, m_actions.toggleHalo});
    m_shortcutBindings.push_back({ShortcutId::Replay, m_actions.toggleReplay});
}

void Application::createCaptureActions() {
    m_capture = std::make_unique<CaptureController>(
        m_settings, *m_overlays, m_pointerHighlight, m_picker,
        [this](const QString& title, const QString& message, bool warning,
               const QString& fileToReveal) {
            notify(title, message,
                   warning ? QSystemTrayIcon::Warning : QSystemTrayIcon::Information, fileToReveal);
        });
    m_capture->createActions(m_actions);

    m_shortcutBindings.push_back({ShortcutId::Screenshot, m_actions.screenshot});
    m_shortcutBindings.push_back({ShortcutId::ScreenshotRegion, m_actions.screenshotRegion});
    m_shortcutBindings.push_back({ShortcutId::Recording, m_actions.toggleRecording});
    m_shortcutBindings.push_back({ShortcutId::RecordRegion, m_actions.recordRegion});
}

QString Application::menuHint() const {
    return m_settings.toolbarMenuTrigger == ToolbarMenuTrigger::LeftClick
               ? tr("click its toolbar button for other options")
               : tr("right-click its toolbar button for other options");
}

QString Application::describe(ShortcutId id) const {
    switch (id) {
    case ShortcutId::ToggleDrawing:
        return tr("Draw mode — when off, clicks reach the applications below");
    case ShortcutId::ToggleVisibility:
        return tr("Show / hide annotations");
    case ShortcutId::Whiteboard:
        return tr("Whiteboard — a blank page over the screens (Esc leaves it)");
    case ShortcutId::Clear:
        return tr("Clear all");
    case ShortcutId::Spotlight:
        return tr("Spotlight — dim everything except the area around the cursor");
    case ShortcutId::Halo:
        return tr("Highlight the cursor with a halo");
    case ShortcutId::Screenshot:
        return tr("Screenshot: %1 — %2")
            .arg(SettingsDialog::targetLabel(m_settings.screenshotTarget), menuHint());
    case ShortcutId::ScreenshotRegion:
        return tr("Screenshot of a region or window");
    case ShortcutId::Replay:
        return tr("Replay: redraw everything from scratch (Esc stops)");
    case ShortcutId::Recording:
        if (!m_capture || !m_capture->isRecordingAvailable()) {
            return tr("Recording unavailable: built without Qt Multimedia");
        }
        return tr("Record: %1 — %2")
            .arg(SettingsDialog::targetLabel(m_settings.recordingTarget), menuHint());
    case ShortcutId::RecordRegion:
        return tr("Record a region or window");
    }
    return {};
}

void Application::start() {
    m_overlays->showOverlays();
    placeToolbar();
    m_toolbar->show();
    m_toolbar->raise();
    if (m_session.toolbarCollapsed) {
        m_toolbar->setCollapsed(true); // left collapsed against an edge last time
    }
    if (!platform::setExcludedFromCapture(m_toolbar->windowHandle(),
                                          m_toolbarExcludedFromCapture)) {
        qCInfo(lcApp) << "This system cannot keep the toolbar out of screenshots and recordings";
    }
    registerHotkeys();

    if (m_tray) {
        m_tray->show();
        const QKeySequence& drawKeys = m_settings.shortcut(ShortcutId::ToggleDrawing);
        if (m_hotkeys->isSupported() && !drawKeys.isEmpty()) {
            m_tray->showMessage(
                tr("Recrayon is running"),
                tr("Press %1 to start drawing.").arg(drawKeys.toString(QKeySequence::NativeText)));
        }
    }
}

void Application::registerHotkeys() {
    m_hotkeys = GlobalHotkeys::create();
    if (!m_hotkeys->isSupported()) {
        qCInfo(lcApp) << "Global hotkeys are not available on this platform yet;"
                      << "use the toolbar or the tray icon";
    }
    connect(m_hotkeys.get(), &GlobalHotkeys::activated, this, [this](int id) {
        for (const ShortcutBinding& binding : m_shortcutBindings) {
            if (hotkeyId(binding.id) == id) {
                binding.action->trigger(); // toggles checkable actions, respects enabled state
                return;
            }
        }
    });

    const QStringList failures = applyShortcuts();
    if (!failures.isEmpty()) {
        qCWarning(lcApp) << "Could not register global hotkeys:" << failures;
        notify(tr("Some shortcuts are not available"),
               tr("Already in use by another application: %1. Change them in Settings.")
                   .arg(failures.join(QStringLiteral(", "))),
               QSystemTrayIcon::Warning);
    }
}

QStringList Application::applyShortcuts() {
    for (const ShortcutBinding& binding : m_shortcutBindings) {
        const QKeySequence& keys = m_settings.shortcut(binding.id);
        const QString description = describe(binding.id);
        binding.action->setToolTip(keys.isEmpty() ? description : withKeys(description, keys));
    }

    QStringList failures;
    if (!m_hotkeys || !m_hotkeys->isSupported()) {
        return failures;
    }
    m_hotkeys->unregisterAll();
    for (const ShortcutBinding& binding : m_shortcutBindings) {
        const QKeySequence& keys = m_settings.shortcut(binding.id);
        if (!keys.isEmpty() && !m_hotkeys->registerHotkey(hotkeyId(binding.id), keys)) {
            failures << QStringLiteral("%1 (%2)").arg(SettingsDialog::shortcutLabel(binding.id),
                                                      keys.toString(QKeySequence::NativeText));
        }
    }
    return failures;
}

void Application::openSettings() {
    if (m_settingsOpen) {
        return;
    }
    m_settingsOpen = true;
    m_overlays->setMode(InteractionMode::Interact);
    // Release the global shortcuts while editing, otherwise pressing a registered combination
    // would trigger its action instead of reaching the shortcut editor.
    if (m_hotkeys) {
        m_hotkeys->unregisterAll();
    }

    bool accepted = false;
    bool languageChanged = false;
    {
        // Scoped: the dialog is a child of the toolbar, which may be rebuilt below.
        SettingsDialog dialog(m_settings, m_capture->isRecordingAvailable(),
                              CaptureController::microphones(), m_toolbar.get());
        if (dialog.exec() == QDialog::Accepted) {
            const Settings previous = m_settings;
            m_settings = dialog.settings();
            QSettings store;
            m_settings.save(store);
            accepted = true;
            languageChanged = m_settings.language != previous.language;
            if (m_settings.palette != previous.palette ||
                m_settings.initialColorIndex != previous.initialColorIndex) {
                m_tools.setColor(m_settings.initialColor());
            }
        }
    }
    if (accepted) {
        m_capture->applySettings();
        applyDrawingSettings();
        rebuildToolbar();
    }

    if (languageChanged && !m_capture->isRecording() &&
        QMessageBox::question(m_toolbar.get(), tr("Change the language"),
                              tr("The new language is used after restarting Recrayon. Restart "
                                 "now? The current drawings will be cleared.")) ==
            QMessageBox::Yes) {
        m_settingsOpen = false;
        restart();
        return;
    }

    const QStringList failures = applyShortcuts();
    m_settingsOpen = false;
    if (!failures.isEmpty()) {
        QMessageBox::warning(m_toolbar.get(), tr("Some shortcuts are not available"),
                             tr("Another application already uses these shortcuts, so they will "
                                "not work until you choose different ones:\n\n%1")
                                 .arg(failures.join(QLatin1Char('\n'))));
    }
}

void Application::restart() {
    // Release the global shortcuts first, or the new instance could not register them.
    if (m_hotkeys) {
        m_hotkeys->unregisterAll();
    }
    if (!QProcess::startDetached(QCoreApplication::applicationFilePath(), {})) {
        notify(tr("Could not restart"), tr("Close Recrayon and open it again."),
               QSystemTrayIcon::Warning);
        applyShortcuts();
        return;
    }
    QCoreApplication::quit();
}

void Application::placeToolbar() {
    m_toolbar->adjustSize();
    // Where the user left it, unless that screen is gone.
    if (m_session.toolbarPosition) {
        m_toolbar->move(*m_session.toolbarPosition);
        if (keepToolbarOnScreen()) {
            return;
        }
    }
    const QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) {
        return;
    }
    // Default: a vertical toolbar at the middle of the right edge, a horizontal one at the top.
    const QRect area = screen->availableGeometry();
    if (m_settings.toolbarOrientation == ToolbarOrientation::Vertical) {
        m_toolbar->move(area.right() - m_toolbar->width() - kToolbarScreenMargin,
                        area.top() + (area.height() - m_toolbar->height()) / 2);
    } else {
        m_toolbar->move(area.left() + (area.width() - m_toolbar->width()) / 2,
                        area.top() + kToolbarScreenMargin);
    }
    keepToolbarOnScreen(); // larger than the screen: at least its start stays visible
}

bool Application::keepToolbarOnScreen() {
    const QRect frame(m_toolbar->pos(), m_toolbar->size());
    QScreen* screen = QGuiApplication::screenAt(frame.center());
    if (!screen) {
        screen = QGuiApplication::screenAt(frame.topLeft());
    }
    if (!screen) {
        return false;
    }
    const QRect area = screen->availableGeometry();
    const int x =
        std::clamp(frame.x(), area.left(), std::max(area.left(), area.right() + 1 - frame.width()));
    const int y =
        std::clamp(frame.y(), area.top(), std::max(area.top(), area.bottom() + 1 - frame.height()));
    m_toolbar->move(x, y);
    return true;
}

void Application::saveSession() {
    if (m_toolbar) {
        m_session.toolbarPosition = m_toolbar->pos();
    }
    m_session.strokeWidth = m_tools.width();
    if (m_toolbar) {
        m_session.toolbarCollapsed = m_toolbar->isCollapsed();
    }
    m_session.tool = QString::fromLatin1(toolId(m_tools.currentTool()));
    QSettings store;
    m_session.save(store);
}

void Application::onModeChanged(InteractionMode mode) {
    qCDebug(lcApp) << "Mode:" << (mode == InteractionMode::Draw ? "draw" : "interact");
    raiseToolbar();
    m_actions.toggleDrawing->setChecked(mode == InteractionMode::Draw);
    if (mode == InteractionMode::Interact && m_replay.isActive()) {
        m_replay.stop();
    }
    // Leaving draw mode (Esc, the draw toggle, hiding annotations) closes the whiteboard; picking
    // a capture area does not, so the whiteboard itself can be captured.
    if (mode == InteractionMode::Interact && m_whiteboardActive && !m_overlays->isPicking()) {
        hideWhiteboard();
    }
}

void Application::restoreToolbar() {
    m_toolbar->showNormal();
    m_toolbar->raise();
    m_toolbar->activateWindow();
    platform::setExcludedFromCapture(m_toolbar->windowHandle(), m_toolbarExcludedFromCapture);
}

void Application::onToolbarMinimized(bool minimized) {
    const QSignalBlocker blocker(m_actions.toggleToolbar);
    m_actions.toggleToolbar->setChecked(!minimized);
    // Nothing on screen tells a draw mode without toolbar apart, and it swallows the clicks.
    if (minimized) {
        m_overlays->setMode(InteractionMode::Interact);
    }
}

void Application::raiseToolbar() {
    // Deferred: on Windows the activation that raised the overlay finishes after this call.
    QTimer::singleShot(0, this, [this] {
        // Opening a menu changes the focus, which brings us here: raising the toolbar then would
        // put it on top of its own menu (both windows are top-most).
        if (QApplication::activePopupWidget()) {
            return;
        }
        if (m_toolbar && m_toolbar->isVisible() && !m_toolbar->isMinimized()) {
            m_toolbar->raise();
        }
    });
}

void Application::onEscapePressed() {
    // Esc while the toolbar has the keyboard (it was just clicked): same as Esc on an overlay.
    if (m_replay.isActive()) {
        m_replay.stop();
    } else if (m_whiteboardActive) {
        hideWhiteboard();
    } else {
        m_overlays->setMode(InteractionMode::Interact);
    }
}

void Application::onPointerHighlightChanged(PointerHighlightMode mode) {
    {
        const QSignalBlocker spotlightBlocker(m_actions.toggleSpotlight);
        const QSignalBlocker haloBlocker(m_actions.toggleHalo);
        m_actions.toggleSpotlight->setChecked(mode == PointerHighlightMode::Spotlight);
        m_actions.toggleHalo->setChecked(mode == PointerHighlightMode::Halo);
    }
    // The highlight is painted by the overlays, so they must be visible.
    if (mode != PointerHighlightMode::Off) {
        m_overlays->setAnnotationsVisible(true);
    }
}

void Application::showWhiteboard(WhiteboardScope scope) {
    QScreen* screen = nullptr;
    if (scope == WhiteboardScope::ScreenUnderCursor) {
        screen = QGuiApplication::screenAt(QCursor::pos());
        if (!screen) {
            screen = QGuiApplication::primaryScreen();
        }
    }
    m_whiteboardActive = true;
    m_overlays->showWhiteboard(m_whiteboardDocument, m_settings.whiteboardColor, screen);
    m_tools.setDocument(m_whiteboardDocument);
    m_overlays->setMode(InteractionMode::Draw);
    // The click may have come from the toolbar or the tray: Esc must work right away.
    m_overlays->focusOverlayUnderCursor();
    const QSignalBlocker blocker(m_actions.toggleWhiteboard);
    m_actions.toggleWhiteboard->setChecked(true);
}

void Application::startReplay() {
    if (m_replay.isActive()) {
        return;
    }
    m_tools.cancel();
    m_overlays->setAnnotationsVisible(true);
    m_drawingBeforeReplay = m_overlays->mode() == InteractionMode::Draw;
    if (!m_replay.start(m_overlays->visiblePages(), m_settings.replaySpeedFactor())) {
        const QSignalBlocker blocker(m_actions.toggleReplay);
        m_actions.toggleReplay->setChecked(false);
        notify(tr("Nothing to replay"), tr("Draw something first."));
        return;
    }
    // Draw mode gives the overlays keyboard focus, so Esc can stop the replay. setMode() does
    // nothing when already drawing, so ask for the focus explicitly: the click that started the
    // replay (or the previous tool) usually left it on the toolbar.
    m_overlays->setMode(InteractionMode::Draw);
    m_overlays->focusOverlayUnderCursor();
}

void Application::onReplayChanged(bool active) {
    {
        const QSignalBlocker blocker(m_actions.toggleReplay);
        m_actions.toggleReplay->setChecked(active);
    }
    if (!active && !m_drawingBeforeReplay && !m_whiteboardActive) {
        m_overlays->setMode(InteractionMode::Interact); // back to how it was before the replay
    }
}

void Application::onGestureCommitted(ToolKind kind) {
    // On the whiteboard the mouse has nothing to go back to: leaving draw mode would close it.
    if (!m_whiteboardActive &&
        m_settings.returnToCursorTools.contains(QString::fromLatin1(toolId(kind)))) {
        m_overlays->setMode(InteractionMode::Interact);
    }
}

void Application::hideWhiteboard() {
    if (!m_whiteboardActive) {
        return;
    }
    m_whiteboardActive = false;
    m_overlays->hideWhiteboard();
    m_tools.setDocument(m_desktopDocument);
    const QSignalBlocker blocker(m_actions.toggleWhiteboard);
    m_actions.toggleWhiteboard->setChecked(false);
}

Document& Application::activeDocument() noexcept {
    return m_tools.document();
}

void Application::updateClearEnabled() {
    m_actions.clear->setEnabled(!activeDocument().isEmpty());
}

void Application::applyDrawingSettings() {
    QFont font(m_settings.textFontFamily.isEmpty()
                   ? QFontDatabase::systemFont(QFontDatabase::GeneralFont).family()
                   : m_settings.textFontFamily);
    font.setPixelSize(m_settings.textPixelSize);
    m_tools.setTextFont(font);
    if (m_overlays) {
        m_overlays->setWhiteboardColor(m_settings.whiteboardColor);
    }
}

void Application::rebuildToolbar() {
    const bool existed = m_toolbar != nullptr;
    const QPoint position = existed ? m_toolbar->pos() : QPoint();
    const bool visible = !existed || m_toolbar->isVisible();

    m_toolbar = std::make_unique<Toolbar>(m_tools, m_actions, m_settings);
    connect(m_toolbar.get(), &Toolbar::toolPicked, this, [this] {
        m_overlays->setMode(InteractionMode::Draw);
        m_overlays->focusOverlayUnderCursor(); // tool keys and Esc keep working after the click
    });
    connect(m_toolbar.get(), &Toolbar::toolDeselected, this,
            [this] { m_overlays->setMode(InteractionMode::Interact); });
    connect(m_toolbar.get(), &Toolbar::minimizedChanged, this, &Application::onToolbarMinimized);
    connect(m_toolbar.get(), &Toolbar::collapsedChanged, this, [this](bool collapsed) {
        m_session.toolbarCollapsed = collapsed;
        saveSession();
    });
    connect(m_toolbar.get(), &Toolbar::quitRequested, m_actions.quit, &QAction::trigger);
    auto* escape = new QAction(m_toolbar.get());
    escape->setShortcut(Qt::Key_Escape);
    connect(escape, &QAction::triggered, this, &Application::onEscapePressed);
    m_toolbar->addAction(escape);
    if (!existed) {
        return; // start() places and shows it
    }
    m_toolbar->adjustSize();
    m_toolbar->move(position);
    keepToolbarOnScreen(); // a bigger size or another orientation may not fit where it was
    if (m_session.toolbarCollapsed) {
        m_toolbar->setCollapsed(true);
    }
    if (visible) {
        m_toolbar->show();
        m_toolbar->raise();
        platform::setExcludedFromCapture(m_toolbar->windowHandle(), m_toolbarExcludedFromCapture);
    }
}

void Application::notify(const QString& title, const QString& message,
                         QSystemTrayIcon::MessageIcon icon, const QString& fileToReveal) {
    qCInfo(lcApp).noquote() << title << message;
    m_notificationFile = fileToReveal;
    if (m_tray) {
        m_tray->showMessage(title, message, icon);
    }
}

void Application::openNotificationFile() {
    if (m_notificationFile.isEmpty()) {
        return;
    }
    const QFileInfo file(m_notificationFile);
#if defined(Q_OS_WIN)
    // Opens the folder with the file already selected.
    QProcess::startDetached(
        QStringLiteral("explorer.exe"),
        QStringList{QStringLiteral("/select,") + QDir::toNativeSeparators(file.filePath())});
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(file.absolutePath()));
#endif
}

} // namespace recrayon
