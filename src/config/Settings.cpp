#include "config/Settings.h"

#include <QDir>
#include <QSettings>
#include <QStandardPaths>

#include <algorithm>
#include <utility>

namespace recrayon {

namespace {

constexpr auto kShortcutsGroup = "shortcuts";
constexpr auto kLanguageKey = "general/language";
constexpr auto kShowCursorKey = "capture/showCursor";
constexpr auto kScreenshotTargetKey = "capture/screenshotTarget";
constexpr auto kRecordingTargetKey = "capture/recordingTarget";
constexpr auto kScreenshotDirKey = "capture/screenshotDirectory";
constexpr auto kRecordingDirKey = "capture/recordingDirectory";
constexpr auto kVideoPresetKey = "capture/videoPreset";
constexpr auto kMicrophoneKey = "capture/recordMicrophone";
constexpr auto kMicrophoneIdKey = "capture/microphoneId";
constexpr auto kTextFontKey = "drawing/textFont";
constexpr auto kTextSizeKey = "drawing/textPixelSize";
constexpr auto kPaletteKey = "drawing/palette";
constexpr auto kInitialColorKey = "drawing/initialColor";
constexpr auto kWhiteboardColorKey = "drawing/whiteboardColor";
constexpr auto kWhiteboardScopeKey = "drawing/whiteboardScope";
constexpr auto kReturnToCursorKey = "drawing/returnToCursor";
constexpr auto kReplaySpeedKey = "drawing/replaySpeed";
constexpr auto kHiddenToolbarItemsKey = "toolbar/hidden";
constexpr auto kToolbarOrderKey = "toolbar/order";
constexpr auto kToolbarSizeKey = "toolbar/size";
constexpr auto kToolbarOrientationKey = "toolbar/orientation";
constexpr auto kToolbarLanesKey = "toolbar/lanes";

constexpr int kMinTextPixelSize = 8;
constexpr int kMaxTextPixelSize = 200;

QString recrayonFolderIn(QStandardPaths::StandardLocation location) {
    QString base = QStandardPaths::writableLocation(location);
    if (base.isEmpty()) {
        base = QDir::homePath();
    }
    return QDir::cleanPath(QDir(base).filePath(QStringLiteral("Recrayon")));
}

QKeySequence globalKeys(Qt::Key key) {
    // Ctrl+Alt+Shift combinations are rarely used by other applications.
    return QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT | key);
}

template <typename Enum, std::size_t N>
using NameTable = std::array<std::pair<Enum, const char*>, N>;

// Stable names stored in the settings file; never reuse or rename an entry.
constexpr NameTable<ScreenshotTarget, 3> kScreenshotTargetNames{{
    {ScreenshotTarget::ScreenUnderCursor, "cursor"},
    {ScreenshotTarget::AllScreens, "all"},
    {ScreenshotTarget::RegionOrWindow, "region"},
}};

constexpr NameTable<RecordingTarget, 5> kRecordingTargetNames{{
    {RecordingTarget::ScreenUnderCursor, "cursor"},
    {RecordingTarget::FollowCursor, "follow"},
    {RecordingTarget::AllScreensSeparate, "all-separate"},
    {RecordingTarget::AllScreensCombined, "all-combined"},
    {RecordingTarget::RegionOrWindow, "region"},
}};

constexpr NameTable<VideoPreset, 3> kVideoPresetNames{{
    {VideoPreset::High, "high"},
    {VideoPreset::Compact, "compact"},
    {VideoPreset::Maximum, "maximum"},
}};

constexpr NameTable<ToolbarSize, 3> kToolbarSizeNames{{
    {ToolbarSize::Small, "small"},
    {ToolbarSize::Normal, "normal"},
    {ToolbarSize::Large, "large"},
}};

constexpr NameTable<ToolbarOrientation, 2> kToolbarOrientationNames{{
    {ToolbarOrientation::Vertical, "vertical"},
    {ToolbarOrientation::Horizontal, "horizontal"},
}};

template <typename Enum, std::size_t N>
QString nameOf(const NameTable<Enum, N>& table, Enum value) {
    for (const auto& [entry, name] : table) {
        if (entry == value) {
            return QLatin1String(name);
        }
    }
    return {};
}

template <typename Enum, std::size_t N>
Enum valueOf(const NameTable<Enum, N>& table, const QString& name, Enum fallback) {
    for (const auto& [entry, entryName] : table) {
        if (name == QLatin1String(entryName)) {
            return entry;
        }
    }
    return fallback;
}

} // namespace

std::array<QKeySequence, kShortcutCount> Settings::defaultShortcuts() {
    std::array<QKeySequence, kShortcutCount> keys;
    const auto set = [&keys](ShortcutId id, Qt::Key key) {
        keys[static_cast<std::size_t>(id)] = globalKeys(key);
    };
    set(ShortcutId::ToggleDrawing, Qt::Key_D);
    set(ShortcutId::ToggleVisibility, Qt::Key_H);
    set(ShortcutId::Whiteboard, Qt::Key_W);
    set(ShortcutId::Clear, Qt::Key_C);
    set(ShortcutId::Spotlight, Qt::Key_F);
    set(ShortcutId::Halo, Qt::Key_P);
    set(ShortcutId::Replay, Qt::Key_Space);
    set(ShortcutId::Screenshot, Qt::Key_S);
    set(ShortcutId::ScreenshotRegion, Qt::Key_A);
    set(ShortcutId::Recording, Qt::Key_R);
    set(ShortcutId::RecordRegion, Qt::Key_V);
    return keys;
}

VideoEncoding videoEncoding(VideoPreset preset) {
    // Measured with Qt 6.8.3 / h264_mf on a 1080p screen-like clip, in Mbit/s: quality level
    // 2 = 5.9, 3 = 10, 4 = 13; 720p at level 3 = 5.9. Compact (75 % and level 2) ends up at
    // roughly a third of High.
    switch (preset) {
    case VideoPreset::High:
        return {30, 1.0, 3};
    case VideoPreset::Compact:
        return {30, 0.75, 2};
    case VideoPreset::Maximum:
        return {30, 1.0, 4};
    }
    return {};
}

ToolbarMetrics toolbarMetrics(ToolbarSize size) {
    switch (size) {
    case ToolbarSize::Small:
        return {28, 18};
    case ToolbarSize::Normal:
        return {34, 22};
    case ToolbarSize::Large:
        return {44, 30};
    }
    return {};
}

QStringList orderedToolbarItems(const QStringList& defaultOrder, const QStringList& savedOrder) {
    QStringList result;
    for (const QString& id : savedOrder) {
        if (defaultOrder.contains(id) && !result.contains(id)) {
            result << id;
        }
    }
    // Items the saved order does not know go after their default predecessor (or first).
    for (qsizetype i = 0; i < defaultOrder.size(); ++i) {
        const QString& id = defaultOrder[i];
        if (result.contains(id)) {
            continue;
        }
        qsizetype position = 0;
        for (qsizetype j = i - 1; j >= 0; --j) {
            const qsizetype predecessor = result.indexOf(defaultOrder[j]);
            if (predecessor >= 0) {
                position = predecessor + 1;
                break;
            }
        }
        result.insert(position, id);
    }
    return result;
}

QString effectiveLanguage(const QString& setting, const QStringList& systemLanguages) {
    if (kSupportedLanguages.contains(setting)) {
        return setting;
    }
    for (const QString& system : systemLanguages) {
        const QString code = system.section(QLatin1Char('-'), 0, 0).toLower(); // "es-ES" -> "es"
        if (kSupportedLanguages.contains(code)) {
            return code;
        }
    }
    return QStringLiteral("en");
}

QString Settings::defaultScreenshotDirectory() {
    return recrayonFolderIn(QStandardPaths::PicturesLocation);
}

QString Settings::defaultRecordingDirectory() {
    return recrayonFolderIn(QStandardPaths::MoviesLocation);
}

std::array<QColor, kPaletteSize> Settings::defaultPalette() {
    return {QColor(0xE5, 0x39, 0x35), QColor(0xFD, 0xD8, 0x35), QColor(0x43, 0xA0, 0x47),
            QColor(0x1E, 0x88, 0xE5), QColor(0x21, 0x21, 0x21), QColor(0xFF, 0xFF, 0xFF)};
}

QList<QPair<ShortcutId, ShortcutId>> Settings::duplicateShortcuts() const {
    QList<QPair<ShortcutId, ShortcutId>> duplicates;
    for (std::size_t i = 0; i < kShortcutCount; ++i) {
        for (std::size_t j = i + 1; j < kShortcutCount; ++j) {
            if (!shortcuts[i].isEmpty() && shortcuts[i] == shortcuts[j]) {
                duplicates.append({kAllShortcuts[i], kAllShortcuts[j]});
            }
        }
    }
    return duplicates;
}

Settings Settings::load(QSettings& store) {
    Settings settings;

    settings.language = store.value(QLatin1String(kLanguageKey)).toString();
    if (!kSupportedLanguages.contains(settings.language)) {
        settings.language.clear(); // unknown or removed language: follow the system
    }

    store.beginGroup(QLatin1String(kShortcutsGroup));
    for (const ShortcutId id : kAllShortcuts) {
        const QString key = settingsKey(id);
        if (store.contains(key)) {
            // Stored as portable text; an empty string means "disabled" on purpose.
            settings.setShortcut(id, QKeySequence::fromString(store.value(key).toString(),
                                                              QKeySequence::PortableText));
        }
    }
    store.endGroup();

    settings.showCursorInCaptures =
        store.value(QLatin1String(kShowCursorKey), settings.showCursorInCaptures).toBool();
    settings.screenshotTarget =
        valueOf(kScreenshotTargetNames, store.value(QLatin1String(kScreenshotTargetKey)).toString(),
                settings.screenshotTarget);
    settings.recordingTarget =
        valueOf(kRecordingTargetNames, store.value(QLatin1String(kRecordingTargetKey)).toString(),
                settings.recordingTarget);

    settings.screenshotDirectory = store.value(QLatin1String(kScreenshotDirKey)).toString();
    settings.recordingDirectory = store.value(QLatin1String(kRecordingDirKey)).toString();
    settings.videoPreset =
        valueOf(kVideoPresetNames, store.value(QLatin1String(kVideoPresetKey)).toString(),
                settings.videoPreset);
    settings.recordMicrophone =
        store.value(QLatin1String(kMicrophoneKey), settings.recordMicrophone).toBool();
    settings.microphoneId = store.value(QLatin1String(kMicrophoneIdKey)).toString();

    settings.textFontFamily =
        store.value(QLatin1String(kTextFontKey), settings.textFontFamily).toString();
    settings.textPixelSize =
        std::clamp(store.value(QLatin1String(kTextSizeKey), settings.textPixelSize).toInt(),
                   kMinTextPixelSize, kMaxTextPixelSize);

    // Stored as "#rrggbb" names; invalid or missing entries keep their default.
    const QStringList colors = store.value(QLatin1String(kPaletteKey)).toStringList();
    for (qsizetype i = 0; i < std::min<qsizetype>(colors.size(), kPaletteSize); ++i) {
        const QColor color = QColor::fromString(colors.at(i));
        if (color.isValid()) {
            settings.palette[static_cast<std::size_t>(i)] = color;
        }
    }
    const int initial = store.value(QLatin1String(kInitialColorKey), 0).toInt();
    settings.initialColorIndex =
        initial >= 0 && initial < static_cast<int>(kPaletteSize) ? initial : 0;

    const QColor board =
        QColor::fromString(store.value(QLatin1String(kWhiteboardColorKey)).toString());
    if (board.isValid()) {
        settings.whiteboardColor = board;
    }

    settings.whiteboardScope =
        store.value(QLatin1String(kWhiteboardScopeKey)).toString() == QLatin1String("screen")
            ? WhiteboardScope::ScreenUnderCursor
            : WhiteboardScope::AllScreens;

    settings.returnToCursorTools = store.value(QLatin1String(kReturnToCursorKey)).toStringList();
    settings.returnToCursorTools.sort();
    const QString speed = store.value(QLatin1String(kReplaySpeedKey)).toString();
    settings.replaySpeed = speed == QLatin1String("slow")   ? ReplaySpeed::Slow
                           : speed == QLatin1String("fast") ? ReplaySpeed::Fast
                                                            : ReplaySpeed::Normal;

    settings.hiddenToolbarItems = store.value(QLatin1String(kHiddenToolbarItemsKey)).toStringList();
    settings.hiddenToolbarItems.sort();
    settings.toolbarOrder = store.value(QLatin1String(kToolbarOrderKey)).toStringList();
    settings.toolbarSize =
        valueOf(kToolbarSizeNames, store.value(QLatin1String(kToolbarSizeKey)).toString(),
                settings.toolbarSize);
    settings.toolbarOrientation = valueOf(
        kToolbarOrientationNames, store.value(QLatin1String(kToolbarOrientationKey)).toString(),
        settings.toolbarOrientation);
    settings.toolbarLanes =
        std::clamp(store.value(QLatin1String(kToolbarLanesKey), settings.toolbarLanes).toInt(),
                   kMinToolbarLanes, kMaxToolbarLanes);
    return settings;
}

void Settings::save(QSettings& store) const {
    store.setValue(QLatin1String(kLanguageKey), language);

    store.beginGroup(QLatin1String(kShortcutsGroup));
    for (const ShortcutId id : kAllShortcuts) {
        store.setValue(settingsKey(id), shortcut(id).toString(QKeySequence::PortableText));
    }
    store.endGroup();

    store.setValue(QLatin1String(kShowCursorKey), showCursorInCaptures);
    store.setValue(QLatin1String(kScreenshotTargetKey),
                   nameOf(kScreenshotTargetNames, screenshotTarget));
    store.setValue(QLatin1String(kRecordingTargetKey),
                   nameOf(kRecordingTargetNames, recordingTarget));

    store.setValue(QLatin1String(kScreenshotDirKey), screenshotDirectory);
    store.setValue(QLatin1String(kRecordingDirKey), recordingDirectory);
    store.setValue(QLatin1String(kVideoPresetKey), nameOf(kVideoPresetNames, videoPreset));
    store.setValue(QLatin1String(kMicrophoneKey), recordMicrophone);
    store.setValue(QLatin1String(kMicrophoneIdKey), microphoneId);

    store.setValue(QLatin1String(kTextFontKey), textFontFamily);
    store.setValue(QLatin1String(kTextSizeKey), textPixelSize);
    QStringList colors;
    for (const QColor& color : palette) {
        colors << color.name();
    }
    store.setValue(QLatin1String(kPaletteKey), colors);
    store.setValue(QLatin1String(kInitialColorKey), initialColorIndex);
    store.setValue(QLatin1String(kWhiteboardColorKey), whiteboardColor.name());
    store.setValue(QLatin1String(kWhiteboardScopeKey),
                   whiteboardScope == WhiteboardScope::ScreenUnderCursor ? QStringLiteral("screen")
                                                                         : QStringLiteral("all"));
    store.setValue(QLatin1String(kHiddenToolbarItemsKey), hiddenToolbarItems);
    store.setValue(QLatin1String(kToolbarOrderKey), toolbarOrder);
    store.setValue(QLatin1String(kToolbarSizeKey), nameOf(kToolbarSizeNames, toolbarSize));
    store.setValue(QLatin1String(kToolbarOrientationKey),
                   nameOf(kToolbarOrientationNames, toolbarOrientation));
    store.setValue(QLatin1String(kToolbarLanesKey), toolbarLanes);
    store.setValue(QLatin1String(kReturnToCursorKey), returnToCursorTools);
    store.setValue(QLatin1String(kReplaySpeedKey),
                   replaySpeed == ReplaySpeed::Slow   ? QStringLiteral("slow")
                   : replaySpeed == ReplaySpeed::Fast ? QStringLiteral("fast")
                                                      : QStringLiteral("normal"));
}

QString settingsKey(ShortcutId id) {
    switch (id) {
    case ShortcutId::ToggleDrawing:
        return QStringLiteral("toggleDrawing");
    case ShortcutId::ToggleVisibility:
        return QStringLiteral("toggleVisibility");
    case ShortcutId::Whiteboard:
        return QStringLiteral("whiteboard");
    case ShortcutId::Clear:
        return QStringLiteral("clear");
    case ShortcutId::Spotlight:
        return QStringLiteral("spotlight");
    case ShortcutId::Halo:
        return QStringLiteral("halo");
    case ShortcutId::Screenshot:
        return QStringLiteral("screenshot");
    case ShortcutId::ScreenshotRegion:
        return QStringLiteral("screenshotRegion");
    case ShortcutId::Replay:
        return QStringLiteral("replay");
    case ShortcutId::Recording:
        return QStringLiteral("recording");
    case ShortcutId::RecordRegion:
        return QStringLiteral("recordRegion");
    }
    return {};
}

bool isSafeGlobalShortcut(const QKeySequence& keys) {
    if (keys.isEmpty()) {
        return true; // disabled
    }
    const QKeyCombination combination = keys[0];
    const Qt::Key key = combination.key();
    if (key >= Qt::Key_F1 && key <= Qt::Key_F24) {
        return true;
    }
    const Qt::KeyboardModifiers modifiers = combination.keyboardModifiers();
    return modifiers.testAnyFlags(Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
}

} // namespace recrayon
