#pragma once

#include <QColor>
#include <QKeySequence>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

#include <array>
#include <cstddef>

class QSettings;

namespace recrayon {

/// Actions that can be bound to a system-wide shortcut.
enum class ShortcutId {
    ToggleDrawing,
    ToggleVisibility,
    Whiteboard,
    Clear,
    Spotlight,
    Halo,
    Screenshot,       ///< default screenshot target
    ScreenshotRegion, ///< always asks for a region or window
    Replay,           ///< redraw everything from scratch
    Recording,        ///< start/stop with the default recording target
    RecordRegion,     ///< start recording a region or window (stops if recording)
};

inline constexpr std::array kAllShortcuts{
    ShortcutId::ToggleDrawing, ShortcutId::ToggleVisibility, ShortcutId::Whiteboard,
    ShortcutId::Clear,         ShortcutId::Spotlight,        ShortcutId::Halo,
    ShortcutId::Replay,        ShortcutId::Screenshot,       ShortcutId::ScreenshotRegion,
    ShortcutId::Recording,     ShortcutId::RecordRegion,
};

inline constexpr std::size_t kPaletteSize = 6;

inline constexpr std::size_t kShortcutCount = kAllShortcuts.size();

/// What a screenshot covers.
enum class ScreenshotTarget {
    ScreenUnderCursor,
    AllScreens,     ///< one image with every screen
    RegionOrWindow, ///< asks the user to drag a rectangle or click a window
};

/// What a recording covers.
enum class RecordingTarget {
    ScreenUnderCursor,
    FollowCursor,       ///< one video that switches to whichever screen has the pointer
    AllScreensSeparate, ///< one video per screen
    AllScreensCombined, ///< one video with every screen side by side
    RegionOrWindow,     ///< asks for a rectangle or a window (a window is followed if it moves)
};

/// Video encoding preset offered to the user.
///
/// There is no 60 fps preset on purpose: Qt 6.8's QScreenCapture delivers ~30 fps whatever the
/// screen does, so a 60 fps video would only repeat frames (bigger files, no smoother). Real
/// 60 fps needs our own capture (DXGI Desktop Duplication).
enum class VideoPreset {
    High,    ///< native size, 30 fps, high quality: sharp text (default)
    Compact, ///< 75 % size, normal quality: files about a third of the size
    Maximum, ///< native size, the encoder's top quality level: ~30 % bigger than High
};

/// Encoding parameters of a VideoPreset. Plain values; capture/ maps them to QMediaRecorder.
struct VideoEncoding {
    int frameRate = 30;
    qreal scale = 1.0;    ///< output size relative to the recorded area
    int qualityLevel = 3; ///< QMediaRecorder::Quality (0 very low ... 4 very high)

    friend bool operator==(const VideoEncoding&, const VideoEncoding&) = default;
};

[[nodiscard]] VideoEncoding videoEncoding(VideoPreset preset);

/// UI languages (ISO 639-1). English is the source language; the others have a translation.
inline const QStringList kSupportedLanguages{QStringLiteral("en"), QStringLiteral("es")};

/// The language to use: @p setting if supported, otherwise the first supported entry of
/// @p systemLanguages (QLocale::uiLanguages(), e.g. "es-ES"), otherwise English.
[[nodiscard]] QString effectiveLanguage(const QString& setting, const QStringList& systemLanguages);

/// Where the whiteboard page is shown.
enum class WhiteboardScope {
    ScreenUnderCursor, ///< only the screen the pointer is on when it opens
    AllScreens,
};

/// How fast a replay redraws the annotations.
enum class ReplaySpeed { Slow, Normal, Fast };

/// Size of the toolbar buttons.
enum class ToolbarSize { Small, Normal, Large };

/// Direction in which the toolbar grows.
enum class ToolbarOrientation {
    Vertical,   ///< a column of sections, top to bottom (default)
    Horizontal, ///< a row of sections, left to right
};

/// How the menu of a toolbar button with several options (whiteboard, screenshot, record) opens.
enum class ToolbarMenuTrigger {
    LeftClick,  ///< a plain click opens the menu (default)
    RightClick, ///< a click runs the default target; right click (or press and hold) opens it
};

/// Pixel sizes of a ToolbarSize.
struct ToolbarMetrics {
    int buttonSize = 34;
    int iconSize = 22;

    friend bool operator==(const ToolbarMetrics&, const ToolbarMetrics&) = default;
};

[[nodiscard]] ToolbarMetrics toolbarMetrics(ToolbarSize size);

/// Buttons side by side across the toolbar: columns when vertical, rows when horizontal.
inline constexpr int kMinToolbarLanes = 1;
inline constexpr int kMaxToolbarLanes = 3;

/// The toolbar items of @p defaultOrder arranged as in @p savedOrder. Ids of @p savedOrder that
/// are not in @p defaultOrder (removed items) are dropped; items missing from @p savedOrder (added
/// by a newer version) keep their default place, right after their default predecessor.
[[nodiscard]] QStringList orderedToolbarItems(const QStringList& defaultOrder,
                                              const QStringList& savedOrder);

/// User preferences. Plain value type: load() / save() convert from / to QSettings.
struct Settings {
    /// UI language code from kSupportedLanguages; empty = follow the system language.
    QString language;

    /// Global shortcut per action, indexed by ShortcutId. An empty sequence disables it.
    std::array<QKeySequence, kShortcutCount> shortcuts = defaultShortcuts();

    /// Draw the real mouse cursor into screenshots and recordings.
    bool showCursorInCaptures = true;

    /// Let the toolbar appear in screenshots / in recordings (it is hidden from both by default).
    /// Only Windows can keep a window out of captures; elsewhere the toolbar always shows.
    bool showToolbarInScreenshots = false;
    bool showToolbarInRecordings = false;

    ScreenshotTarget screenshotTarget = ScreenshotTarget::ScreenUnderCursor;
    RecordingTarget recordingTarget = RecordingTarget::ScreenUnderCursor;

    /// Where files are written. Empty = the default folder (see defaultScreenshotDirectory()).
    QString screenshotDirectory;
    QString recordingDirectory;

    VideoPreset videoPreset = VideoPreset::High;

    /// Record the microphone into the videos (AAC).
    bool recordMicrophone = false;
    /// QAudioDevice::id() of the chosen microphone; empty = the system default.
    QString microphoneId;

    /// The folder actually used: the chosen one, or the default.
    [[nodiscard]] QString screenshotFolder() const {
        return screenshotDirectory.isEmpty() ? defaultScreenshotDirectory() : screenshotDirectory;
    }
    [[nodiscard]] QString recordingFolder() const {
        return recordingDirectory.isEmpty() ? defaultRecordingDirectory() : recordingDirectory;
    }
    /// <Pictures>/Recrayon and <Videos>/Recrayon (the home directory when the system has no
    /// such location).
    [[nodiscard]] static QString defaultScreenshotDirectory();
    [[nodiscard]] static QString defaultRecordingDirectory();

    /// Font family for text and callouts; empty = the system's default UI font.
    QString textFontFamily;
    int textPixelSize = 28;

    /// Colors offered by the toolbar, and which one is selected at start-up.
    std::array<QColor, kPaletteSize> palette = defaultPalette();
    int initialColorIndex = 0;

    QColor whiteboardColor{Qt::white};

    /// Tools (ids from toolId()) after which the app goes back to the mouse (interact mode).
    QStringList returnToCursorTools;

    ReplaySpeed replaySpeed = ReplaySpeed::Normal;
    /// Multiplier for ReplayTimeline.
    [[nodiscard]] qreal replaySpeedFactor() const {
        return replaySpeed == ReplaySpeed::Slow   ? 0.5
               : replaySpeed == ReplaySpeed::Fast ? 2.0
                                                  : 1.0;
    }
    WhiteboardScope whiteboardScope = WhiteboardScope::AllScreens;

    /// Toolbar items the user chose to hide (ids defined by the toolbar, e.g. "tool.pen").
    QStringList hiddenToolbarItems;
    /// Toolbar item ids in the user's order; empty = the default order. Apply it with
    /// orderedToolbarItems(), which also handles ids unknown to this version.
    QStringList toolbarOrder;
    ToolbarSize toolbarSize = ToolbarSize::Normal;
    ToolbarOrientation toolbarOrientation = ToolbarOrientation::Vertical;
    /// kMinToolbarLanes..kMaxToolbarLanes.
    int toolbarLanes = 2;
    ToolbarMenuTrigger toolbarMenuTrigger = ToolbarMenuTrigger::LeftClick;

    [[nodiscard]] bool isToolbarItemVisible(const QString& id) const {
        return !hiddenToolbarItems.contains(id);
    }
    [[nodiscard]] QColor initialColor() const {
        return palette[static_cast<std::size_t>(initialColorIndex)];
    }

    [[nodiscard]] const QKeySequence& shortcut(ShortcutId id) const {
        return shortcuts[static_cast<std::size_t>(id)];
    }
    void setShortcut(ShortcutId id, const QKeySequence& keys) {
        shortcuts[static_cast<std::size_t>(id)] = keys;
    }

    /// Pairs of actions that share the same (non-empty) shortcut.
    [[nodiscard]] QList<QPair<ShortcutId, ShortcutId>> duplicateShortcuts() const;

    [[nodiscard]] static std::array<QKeySequence, kShortcutCount> defaultShortcuts();
    [[nodiscard]] static std::array<QColor, kPaletteSize> defaultPalette();

    /// Reads settings; missing or invalid values fall back to the defaults.
    [[nodiscard]] static Settings load(QSettings& store);
    void save(QSettings& store) const;

    friend bool operator==(const Settings&, const Settings&) = default;
};

/// Stable key used in the settings file ("toggleDrawing", ...).
[[nodiscard]] QString settingsKey(ShortcutId id);

/// A global shortcut must include Ctrl, Alt or Meta (Shift alone is not enough), except for the
/// function keys F1–F24. Anything else would steal ordinary typing from every application.
[[nodiscard]] bool isSafeGlobalShortcut(const QKeySequence& keys);

} // namespace recrayon
