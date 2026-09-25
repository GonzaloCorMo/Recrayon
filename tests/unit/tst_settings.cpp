#include "config/SessionState.h"
#include "config/Settings.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

using namespace recrayon;

class TestSettings : public QObject {
    Q_OBJECT

private slots:
    void defaultsHaveUniqueSafeShortcuts() {
        const Settings settings;
        QVERIFY(settings.duplicateShortcuts().isEmpty());
        for (const ShortcutId id : kAllShortcuts) {
            QVERIFY(!settings.shortcut(id).isEmpty());
            QVERIFY(isSafeGlobalShortcut(settings.shortcut(id)));
        }
        QVERIFY(settings.showCursorInCaptures);
        // The toolbar stays out of screenshots and recordings unless the user asks for it.
        QVERIFY(!settings.showToolbarInScreenshots);
        QVERIFY(!settings.showToolbarInRecordings);
        QCOMPARE(settings.screenshotTarget, ScreenshotTarget::ScreenUnderCursor);
        QCOMPARE(settings.recordingTarget, RecordingTarget::ScreenUnderCursor);
    }

    void saveAndLoadRoundTrip() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("settings.ini"));

        Settings original;
        original.setShortcut(ShortcutId::Screenshot, QKeySequence(Qt::META | Qt::Key_F9));
        original.setShortcut(ShortcutId::Halo, QKeySequence()); // disabled on purpose
        original.showCursorInCaptures = false;
        original.showToolbarInScreenshots = true;
        original.screenshotTarget = ScreenshotTarget::RegionOrWindow;
        original.recordingTarget = RecordingTarget::AllScreensCombined;
        {
            QSettings store(path, QSettings::IniFormat);
            original.save(store);
        }

        QSettings store(path, QSettings::IniFormat);
        const Settings loaded = Settings::load(store);
        QVERIFY(loaded == original);
        QVERIFY(loaded.shortcut(ShortcutId::Halo).isEmpty());
    }

    void captureFoldersRoundTripAndDefault() {
        Settings defaults;
        QVERIFY(defaults.screenshotDirectory.isEmpty());
        QCOMPARE(defaults.screenshotFolder(), Settings::defaultScreenshotDirectory());
        QCOMPARE(defaults.recordingFolder(), Settings::defaultRecordingDirectory());
        QVERIFY(Settings::defaultScreenshotDirectory().endsWith(QStringLiteral("/Recrayon")));
        QVERIFY(Settings::defaultRecordingDirectory().endsWith(QStringLiteral("/Recrayon")));

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("settings.ini"));
        Settings original;
        original.screenshotDirectory = dir.filePath(QStringLiteral("shots"));
        original.recordingDirectory = dir.filePath(QStringLiteral("videos"));
        {
            QSettings store(path, QSettings::IniFormat);
            original.save(store);
        }
        QSettings store(path, QSettings::IniFormat);
        const Settings loaded = Settings::load(store);
        QCOMPARE(loaded.screenshotDirectory, original.screenshotDirectory);
        QCOMPARE(loaded.recordingFolder(), dir.filePath(QStringLiteral("videos")));
        QVERIFY(loaded == original);
    }

    void videoPresetRoundTripsAndMapsToEncoding() {
        QCOMPARE(Settings{}.videoPreset, VideoPreset::High);
        const VideoEncoding high = videoEncoding(VideoPreset::High);
        const VideoEncoding compact = videoEncoding(VideoPreset::Compact);
        // High is what recordings used before presets existed.
        QCOMPARE(high, (VideoEncoding{30, 1.0, 3}));
        QVERIFY(compact.scale < high.scale);
        QVERIFY(compact.qualityLevel < high.qualityLevel);
        QCOMPARE(compact.frameRate, high.frameRate); // Qt's screen capture gives ~30 fps anyway

        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("settings.ini"));
        for (const VideoPreset preset : {VideoPreset::High, VideoPreset::Compact}) {
            Settings original;
            original.videoPreset = preset;
            {
                QSettings store(path, QSettings::IniFormat);
                original.save(store);
            }
            QSettings store(path, QSettings::IniFormat);
            QCOMPARE(Settings::load(store).videoPreset, preset);
        }
        {
            QSettings store(path, QSettings::IniFormat);
            store.setValue(QStringLiteral("capture/videoPreset"), QStringLiteral("smooth"));
        }
        QSettings store(path, QSettings::IniFormat);
        QCOMPARE(Settings::load(store).videoPreset, VideoPreset::High);
    }

    void microphoneOptionsRoundTrip() {
        QVERIFY(!Settings{}.recordMicrophone); // never record audio unless asked
        QVERIFY(Settings{}.microphoneId.isEmpty());
        QCOMPARE(videoEncoding(VideoPreset::Maximum).qualityLevel, 4);
        QVERIFY(videoEncoding(VideoPreset::Maximum).qualityLevel >
                videoEncoding(VideoPreset::High).qualityLevel);

        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("settings.ini"));
        Settings original;
        original.recordMicrophone = true;
        original.microphoneId = QStringLiteral("{0.0.1.00000000}.{abc-123}");
        original.videoPreset = VideoPreset::Maximum;
        {
            QSettings store(path, QSettings::IniFormat);
            original.save(store);
        }
        QSettings store(path, QSettings::IniFormat);
        QVERIFY(Settings::load(store) == original);
    }

    void languageFollowsSystemUnlessChosen() {
        QVERIFY(Settings{}.language.isEmpty());
        const QStringList spanishSystem{QStringLiteral("es-ES"), QStringLiteral("en-US")};
        QCOMPARE(effectiveLanguage(QString(), spanishSystem), QStringLiteral("es"));
        QCOMPARE(effectiveLanguage(QStringLiteral("en"), spanishSystem), QStringLiteral("en"));
        // First supported system language wins; unsupported ones are skipped.
        QCOMPARE(effectiveLanguage(QString(), {QStringLiteral("ja-JP"), QStringLiteral("es-MX")}),
                 QStringLiteral("es"));
        QCOMPARE(effectiveLanguage(QString(), {QStringLiteral("ja-JP")}), QStringLiteral("en"));
        QCOMPARE(effectiveLanguage(QStringLiteral("xx"), {}), QStringLiteral("en"));

        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("settings.ini"));
        Settings original;
        original.language = QStringLiteral("es");
        {
            QSettings store(path, QSettings::IniFormat);
            original.save(store);
        }
        {
            QSettings store(path, QSettings::IniFormat);
            QCOMPARE(Settings::load(store).language, QStringLiteral("es"));
            store.setValue(QStringLiteral("general/language"), QStringLiteral("klingon"));
        }
        QSettings store(path, QSettings::IniFormat);
        QVERIFY(Settings::load(store).language.isEmpty());
    }

    void missingOrInvalidValuesFallBackToDefaults() {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("settings.ini"));
        {
            QSettings store(path, QSettings::IniFormat);
            store.setValue(QStringLiteral("capture/screenshotTarget"), QStringLiteral("bogus"));
            store.setValue(QStringLiteral("capture/recordingTarget"), 42);
        }
        QSettings store(path, QSettings::IniFormat);
        QVERIFY(Settings::load(store) == Settings{});
    }

    void everyRecordingTargetRoundTrips() {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("settings.ini"));
        for (const RecordingTarget target :
             {RecordingTarget::ScreenUnderCursor, RecordingTarget::FollowCursor,
              RecordingTarget::AllScreensSeparate, RecordingTarget::AllScreensCombined,
              RecordingTarget::RegionOrWindow}) {
            Settings settings;
            settings.recordingTarget = target;
            {
                QSettings store(path, QSettings::IniFormat);
                settings.save(store);
            }
            QSettings store(path, QSettings::IniFormat);
            QCOMPARE(Settings::load(store).recordingTarget, target);
        }
    }

    void drawingAndToolbarOptionsRoundTrip() {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("settings.ini"));
        Settings original;
        original.textFontFamily = QStringLiteral("Consolas");
        original.textPixelSize = 40;
        original.palette[2] = QColor(0x12, 0x34, 0x56);
        original.initialColorIndex = 3;
        original.whiteboardColor = QColor(0x10, 0x30, 0x20);
        original.whiteboardScope = WhiteboardScope::ScreenUnderCursor;
        original.returnToCursorTools = {QStringLiteral("arrow"), QStringLiteral("text")};
        original.replaySpeed = ReplaySpeed::Fast;
        original.hiddenToolbarItems = {QStringLiteral("quit"), QStringLiteral("tool.line")};
        {
            QSettings store(path, QSettings::IniFormat);
            original.save(store);
        }
        QSettings store(path, QSettings::IniFormat);
        const Settings loaded = Settings::load(store);
        QVERIFY(loaded == original);
        QVERIFY(!loaded.isToolbarItemVisible(QStringLiteral("tool.line")));
        QVERIFY(loaded.isToolbarItemVisible(QStringLiteral("tool.pen")));
        QCOMPARE(loaded.initialColor(), QColor(0x1E, 0x88, 0xE5));
    }

    void invalidDrawingValuesAreSanitized() {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("settings.ini"));
        {
            QSettings store(path, QSettings::IniFormat);
            store.setValue(QStringLiteral("drawing/textPixelSize"), 5000);
            store.setValue(QStringLiteral("drawing/initialColor"), 42);
            store.setValue(QStringLiteral("drawing/palette"),
                           QStringList{QStringLiteral("not a color"), QStringLiteral("#00ff00")});
        }
        QSettings store(path, QSettings::IniFormat);
        const Settings loaded = Settings::load(store);
        QCOMPARE(loaded.textPixelSize, 200);
        QCOMPARE(loaded.initialColorIndex, 0);
        QCOMPARE(loaded.palette[0], Settings::defaultPalette()[0]); // invalid entry kept default
        QCOMPARE(loaded.palette[1], QColor(0, 255, 0));
    }

    void toolbarLayoutRoundTripsAndIsSanitized() {
        const Settings defaults;
        QCOMPARE(defaults.toolbarSize, ToolbarSize::Normal);
        QCOMPARE(defaults.toolbarOrientation, ToolbarOrientation::Vertical);
        QCOMPARE(defaults.toolbarLanes, 2);
        // A plain click opens the menu of the buttons that have one.
        QCOMPARE(defaults.toolbarMenuTrigger, ToolbarMenuTrigger::LeftClick);
        QVERIFY(defaults.toolbarOrder.isEmpty());
        // Normal is the size the toolbar always had.
        QCOMPARE(toolbarMetrics(ToolbarSize::Normal), (ToolbarMetrics{34, 22}));
        QVERIFY(toolbarMetrics(ToolbarSize::Small).buttonSize <
                toolbarMetrics(ToolbarSize::Normal).buttonSize);
        QVERIFY(toolbarMetrics(ToolbarSize::Large).buttonSize >
                toolbarMetrics(ToolbarSize::Normal).buttonSize);
        for (const ToolbarSize size :
             {ToolbarSize::Small, ToolbarSize::Normal, ToolbarSize::Large}) {
            QVERIFY(toolbarMetrics(size).iconSize < toolbarMetrics(size).buttonSize);
        }

        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("settings.ini"));
        Settings original;
        original.toolbarSize = ToolbarSize::Large;
        original.toolbarOrientation = ToolbarOrientation::Horizontal;
        original.toolbarLanes = 3;
        original.toolbarOrder = {QStringLiteral("undo"), QStringLiteral("tool.pen")};
        original.toolbarMenuTrigger = ToolbarMenuTrigger::RightClick;
        {
            QSettings store(path, QSettings::IniFormat);
            original.save(store);
        }
        {
            QSettings store(path, QSettings::IniFormat);
            QVERIFY(Settings::load(store) == original);
            store.setValue(QStringLiteral("toolbar/size"), QStringLiteral("huge"));
            store.setValue(QStringLiteral("toolbar/orientation"), QStringLiteral("diagonal"));
            store.setValue(QStringLiteral("toolbar/lanes"), 9);
            store.setValue(QStringLiteral("toolbar/menuTrigger"), QStringLiteral("middle"));
        }
        QSettings store(path, QSettings::IniFormat);
        const Settings loaded = Settings::load(store);
        QCOMPARE(loaded.toolbarSize, ToolbarSize::Normal);
        QCOMPARE(loaded.toolbarOrientation, ToolbarOrientation::Vertical);
        QCOMPARE(loaded.toolbarLanes, kMaxToolbarLanes);
        QCOMPARE(loaded.toolbarMenuTrigger, ToolbarMenuTrigger::LeftClick);
    }

    void toolbarOrderKeepsUserOrderAndPlacesNewItems() {
        const QStringList defaults{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c"),
                                   QStringLiteral("d")};
        QCOMPARE(orderedToolbarItems(defaults, {}), defaults);
        const QStringList reversed{QStringLiteral("d"), QStringLiteral("c"), QStringLiteral("b"),
                                   QStringLiteral("a")};
        QCOMPARE(orderedToolbarItems(defaults, reversed), reversed);
        // Unknown and repeated ids are dropped.
        QCOMPARE(orderedToolbarItems(defaults, {QStringLiteral("x"), QStringLiteral("d"),
                                                QStringLiteral("c"), QStringLiteral("d"),
                                                QStringLiteral("b"), QStringLiteral("a")}),
                 reversed);
        // "c" is new: it goes right after "b", its default predecessor.
        QCOMPARE(orderedToolbarItems(
                     defaults, {QStringLiteral("d"), QStringLiteral("b"), QStringLiteral("a")}),
                 (QStringList{QStringLiteral("d"), QStringLiteral("b"), QStringLiteral("c"),
                              QStringLiteral("a")}));
        // A new first item stays first; "d" follows "c" wherever the user put it.
        QCOMPARE(orderedToolbarItems(defaults, {QStringLiteral("c"), QStringLiteral("b")}),
                 (QStringList{QStringLiteral("a"), QStringLiteral("c"), QStringLiteral("d"),
                              QStringLiteral("b")}));
    }

    void sessionStateRoundTripsAndRejectsGarbage() {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("settings.ini"));
        {
            QSettings store(path, QSettings::IniFormat);
            QVERIFY(SessionState::load(store) == SessionState{}); // nothing saved yet
        }
        SessionState original;
        original.toolbarPosition = QPoint(-1200, 340); // monitors left of the primary one
        original.strokeWidth = 8.0;
        original.tool = QStringLiteral("arrow");
        {
            QSettings store(path, QSettings::IniFormat);
            original.save(store);
        }
        {
            QSettings store(path, QSettings::IniFormat);
            QVERIFY(SessionState::load(store) == original);
            store.setValue(QStringLiteral("session/toolbarPosition"), QStringLiteral("nowhere"));
            store.setValue(QStringLiteral("session/strokeWidth"), 5000);
        }
        {
            QSettings store(path, QSettings::IniFormat);
            const SessionState loaded = SessionState::load(store);
            QVERIFY(!loaded.toolbarPosition);
            QVERIFY(!loaded.strokeWidth);
            QCOMPARE(loaded.tool, QStringLiteral("arrow"));
            SessionState{}.save(store); // empty values remove the keys
        }
        QSettings store(path, QSettings::IniFormat);
        QVERIFY(SessionState::load(store) == SessionState{});
    }

    void detectsDuplicates() {
        Settings settings;
        settings.setShortcut(ShortcutId::Clear, settings.shortcut(ShortcutId::ToggleDrawing));
        const auto duplicates = settings.duplicateShortcuts();
        QCOMPARE(duplicates.size(), qsizetype{1});
        QCOMPARE(duplicates.first().first, ShortcutId::ToggleDrawing);
        QCOMPARE(duplicates.first().second, ShortcutId::Clear);
    }

    void emptyShortcutsAreNotDuplicates() {
        Settings settings;
        settings.setShortcut(ShortcutId::Clear, {});
        settings.setShortcut(ShortcutId::Halo, {});
        QVERIFY(settings.duplicateShortcuts().isEmpty());
    }

    void safeShortcutRules() {
        QVERIFY(isSafeGlobalShortcut(QKeySequence(Qt::CTRL | Qt::Key_A)));
        QVERIFY(isSafeGlobalShortcut(QKeySequence(Qt::ALT | Qt::Key_1)));
        QVERIFY(isSafeGlobalShortcut(QKeySequence(Qt::Key_F8)));
        QVERIFY(!isSafeGlobalShortcut(QKeySequence(Qt::Key_A)));
        QVERIFY(!isSafeGlobalShortcut(QKeySequence(Qt::SHIFT | Qt::Key_A)));
    }
};

QTEST_GUILESS_MAIN(TestSettings)
#include "tst_settings.moc"
