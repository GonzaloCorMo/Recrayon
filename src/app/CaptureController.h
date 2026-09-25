#pragma once

#include "config/Settings.h"
#include "ui/RegionPicker.h"
#include "ui/SettingsDialog.h"

#include <QObject>
#include <QRect>
#include <QString>

#include <functional>
#include <memory>

class QAction;
class QMenu;
class QScreen;

namespace recrayon {

namespace capture {
class ComposedRecorder;
class RecordingSession;
} // namespace capture

struct AppActions;
class OverlayManager;
class PointerHighlight;
class RecordingIndicator;

/// Everything about screenshots and screen recordings: the actions and menus, asking the user
/// for a region or window, and choosing the right recorder for each target:
///
/// | Target                         | Implementation                                    |
/// |--------------------------------|---------------------------------------------------|
/// | Screen under cursor            | RecordingSession, 1 native screen capture         |
/// | All screens, one file each     | RecordingSession, N native screen captures        |
/// | Follow cursor / all in one /   | ComposedRecorder: frames composed from every      |
/// | region / window                | screen, view re-evaluated every frame             |
class CaptureController final : public QObject {
    Q_OBJECT

public:
    /// Shows a system notification. @p fileToReveal, when set, is the file the notification
    /// opens in the file manager if the user clicks it.
    using Notifier = std::function<void(const QString& title, const QString& message, bool warning,
                                        const QString& fileToReveal)>;

    CaptureController(const Settings& settings, OverlayManager& overlays,
                      PointerHighlight& pointerHighlight, RegionPicker& picker, Notifier notify,
                      QObject* parent = nullptr);
    ~CaptureController() override;

    /// Creates the screenshot / recording actions and menus into @p actions.
    void createActions(AppActions& actions);

    [[nodiscard]] bool isRecordingAvailable() const noexcept { return m_session != nullptr; }
    /// Audio inputs available now (empty without Qt Multimedia).
    [[nodiscard]] static QList<SettingsDialog::Microphone> microphones();
    [[nodiscard]] bool isRecording() const;

    /// Keeps the toolbar in or out of captures. The application owns the toolbar window, so it
    /// provides the setter; @p excluded false makes the toolbar visible to captures.
    using ExclusionSetter = std::function<void(bool excluded)>;
    void setToolbarExclusion(ExclusionSetter setter);

    /// Re-applies settings that affect an ongoing recording (cursor, toolbar in the video).
    void applySettings();

    void takeScreenshot(ScreenshotTarget target);
    void startRecording(RecordingTarget target);
    void stopRecording();
    /// Pauses / resumes the running recording (the paused time is not in the video).
    void setRecordingPaused(bool paused);
    [[nodiscard]] bool isRecordingPaused() const noexcept { return m_paused; }

private:
    enum class PendingPick { None, Screenshot, Recording };

    void beginPick(PendingPick purpose);
    void onPickFinished(const RegionPicker::Selection& selection);
    void onPickCanceled();
    void captureNativeArea(const QRect& nativeArea);
    /// @p areaSize: native size of the recorded area; the video size follows the preset.
    void startComposed(std::function<QRect()> view, const QSize& areaSize);
    void onRecordingChanged();
    void onRecordingFinished(const QStringList& paths);
    void onRecordingError(const QString& message);
    void setRecordingChecked(bool checked);
    /// Hides the toolbar from captures unless the settings say otherwise for what is running now.
    void applyToolbarExclusion(bool forScreenshot = false);

    const Settings& m_settings;
    OverlayManager& m_overlays;
    PointerHighlight& m_pointerHighlight;
    RegionPicker& m_picker;
    Notifier m_notify;
    ExclusionSetter m_setToolbarExcluded;

    QAction* m_toggleRecording = nullptr;
    std::unique_ptr<QMenu> m_screenshotMenu;
    std::unique_ptr<QMenu> m_recordingMenu;
    capture::RecordingSession* m_session = nullptr;  // QObject child; null without Multimedia
    capture::ComposedRecorder* m_composed = nullptr; // QObject child; null without Multimedia
    std::unique_ptr<RecordingIndicator> m_indicator; // null without Multimedia
    QString m_recordingLabel;                 ///< what is being recorded, shown by the indicator
    QList<QAction*> m_recordingTargetActions; ///< disabled while a recording runs
    bool m_paused = false;
    VideoEncoding m_encoding;      ///< of the current / last recording
    bool m_withMicrophone = false; ///< the current / last recording has an audio track
    PendingPick m_pendingPick = PendingPick::None;
};

} // namespace recrayon
