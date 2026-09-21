#include "app/CaptureController.h"

#include "capture/Screenshot.h"
#include "core/Geometry.h"
#include "platform/CaptureExclusion.h"
#include "platform/CursorSprite.h"
#include "platform/DesktopGeometry.h"
#include "platform/TopLevelWindows.h"
#include "ui/AppActions.h"
#include "ui/Icons.h"
#include "ui/OverlayManager.h"
#include "ui/PointerHighlight.h"
#include "ui/RecordingIndicator.h"
#include "ui/SettingsDialog.h"

#if RECRAYON_HAS_RECORDING
#include "capture/ComposedRecorder.h"
#include "capture/RecordingSession.h"

#include <QAudioDevice>
#include <QMediaDevices>
#endif

#include <QAction>
#include <QClipboard>
#include <QCursor>
#include <QDesktopServices>
#include <QDir>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QMenu>
#include <QScreen>
#include <QSignalBlocker>
#include <QTimer>
#include <QUrl>

#include <algorithm>
#include <iterator>
#include <utility>

Q_LOGGING_CATEGORY(lcCapture, "recrayon.capture")

namespace recrayon {

namespace {

using icons::IconId;

/// Time for the window compositor to show the overlays without the picker / draw-mode frame
/// before the screen is grabbed.
constexpr int kScreenshotSettleMs = 80;

QScreen* screenUnderCursor() {
    QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
    return screen ? screen : QGuiApplication::primaryScreen();
}

/// Shows @p folder in the file manager (creating it first, so it always opens).
void openFolder(const QString& folder) {
    QDir().mkpath(folder);
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}

#if RECRAYON_HAS_RECORDING
/// The microphone chosen in @p settings: the saved device if it is connected, otherwise the
/// system default. Null when audio is off or there is no microphone at all.
QAudioDevice microphoneFor(const Settings& settings) {
    if (!settings.recordMicrophone) {
        return {};
    }
    const QList<QAudioDevice> inputs = QMediaDevices::audioInputs();
    for (const QAudioDevice& input : inputs) {
        if (QString::fromUtf8(input.id()) == settings.microphoneId) {
            return input;
        }
    }
    return QMediaDevices::defaultAudioInput();
}
#endif

} // namespace

QList<SettingsDialog::Microphone> CaptureController::microphones() {
    QList<SettingsDialog::Microphone> result;
#if RECRAYON_HAS_RECORDING
    const QList<QAudioDevice> inputs = QMediaDevices::audioInputs();
    for (const QAudioDevice& input : inputs) {
        result.append({QString::fromUtf8(input.id()), input.description()});
    }
#endif
    return result;
}

CaptureController::CaptureController(const Settings& settings, OverlayManager& overlays,
                                     PointerHighlight& pointerHighlight, RegionPicker& picker,
                                     Notifier notify, QObject* parent)
    : QObject(parent), m_settings(settings), m_overlays(overlays),
      m_pointerHighlight(pointerHighlight), m_picker(picker), m_notify(std::move(notify)) {
    connect(&m_picker, &RegionPicker::finished, this, &CaptureController::onPickFinished);
    connect(&m_picker, &RegionPicker::canceled, this, &CaptureController::onPickCanceled);

#if RECRAYON_HAS_RECORDING
    m_session = new capture::RecordingSession(this);
    connect(m_session, &capture::RecordingSession::recordingChanged, this,
            &CaptureController::onRecordingChanged);
    connect(m_session, &capture::RecordingSession::finished, this,
            &CaptureController::onRecordingFinished);
    connect(m_session, &capture::RecordingSession::errorOccurred, this,
            &CaptureController::onRecordingError);

    m_composed = new capture::ComposedRecorder(this);
    connect(m_composed, &capture::ComposedRecorder::recordingChanged, this,
            &CaptureController::onRecordingChanged);
    connect(m_composed, &capture::ComposedRecorder::finished, this,
            [this](const QString& path) { onRecordingFinished({path}); });
    connect(m_composed, &capture::ComposedRecorder::errorOccurred, this,
            &CaptureController::onRecordingError);

    m_indicator = std::make_unique<RecordingIndicator>();
    // Like the toolbar: visible to the user, absent from the video (where the OS supports it).
    // Applied to the native window before it is ever shown, so no frame contains it.
    m_indicator->winId();
    if (!platform::setExcludedFromCapture(m_indicator->windowHandle(), true)) {
        qCInfo(lcCapture) << "The recording indicator will be visible in recordings";
    }
    connect(m_indicator.get(), &RecordingIndicator::stopRequested, this,
            &CaptureController::stopRecording);
    connect(m_indicator.get(), &RecordingIndicator::pauseRequested, this,
            &CaptureController::setRecordingPaused);
#endif
}

CaptureController::~CaptureController() = default;

void CaptureController::createActions(AppActions& actions) {
    actions.screenshot = new QAction(icons::icon(IconId::Screenshot), tr("Screenshot"), this);
    connect(actions.screenshot, &QAction::triggered, this,
            [this] { takeScreenshot(m_settings.screenshotTarget); });

    actions.screenshotRegion =
        new QAction(SettingsDialog::targetLabel(ScreenshotTarget::RegionOrWindow), this);
    connect(actions.screenshotRegion, &QAction::triggered, this,
            [this] { takeScreenshot(ScreenshotTarget::RegionOrWindow); });

    m_screenshotMenu = std::make_unique<QMenu>(tr("Screenshot of"));
    for (const ScreenshotTarget target :
         {ScreenshotTarget::ScreenUnderCursor, ScreenshotTarget::AllScreens}) {
        m_screenshotMenu->addAction(SettingsDialog::targetLabel(target), this,
                                    [this, target] { takeScreenshot(target); });
    }
    m_screenshotMenu->addAction(actions.screenshotRegion);
    m_screenshotMenu->addSeparator();
    m_screenshotMenu->addAction(tr("Open the screenshots folder"), this,
                                [this] { openFolder(m_settings.screenshotFolder()); });
    actions.screenshotMenu = m_screenshotMenu.get();

    m_toggleRecording = new QAction(icons::icon(IconId::Record), tr("Record screen"), this);
    m_toggleRecording->setCheckable(true);
    actions.toggleRecording = m_toggleRecording;

    actions.recordRegion =
        new QAction(SettingsDialog::targetLabel(RecordingTarget::RegionOrWindow), this);

    m_recordingMenu = std::make_unique<QMenu>(tr("Record"));
    for (const RecordingTarget target :
         {RecordingTarget::ScreenUnderCursor, RecordingTarget::FollowCursor,
          RecordingTarget::AllScreensSeparate, RecordingTarget::AllScreensCombined}) {
        m_recordingTargetActions << m_recordingMenu->addAction(
            SettingsDialog::targetLabel(target), this, [this, target] { startRecording(target); });
    }
    // Not actions.recordRegion itself: that one also stops a running recording (its shortcut),
    // while menu entries are disabled during a recording.
    m_recordingTargetActions << m_recordingMenu->addAction(
        SettingsDialog::targetLabel(RecordingTarget::RegionOrWindow), this,
        [this] { startRecording(RecordingTarget::RegionOrWindow); });
    // Always available, even while recording.
    m_recordingMenu->addSeparator();
    m_recordingMenu->addAction(tr("Open the videos folder"), this,
                               [this] { openFolder(m_settings.recordingFolder()); });
    actions.recordingMenu = m_recordingMenu.get();

    if (!isRecordingAvailable()) {
        m_toggleRecording->setEnabled(false);
        actions.recordRegion->setEnabled(false);
        for (QAction* action : std::as_const(m_recordingTargetActions)) {
            action->setEnabled(false);
        }
        return;
    }
    connect(m_toggleRecording, &QAction::toggled, this, [this](bool on) {
        if (on) {
            startRecording(m_settings.recordingTarget);
        } else {
            stopRecording();
        }
    });
    connect(actions.recordRegion, &QAction::triggered, this, [this] {
        if (isRecording()) {
            stopRecording();
        } else {
            startRecording(RecordingTarget::RegionOrWindow);
        }
    });
}

bool CaptureController::isRecording() const {
#if RECRAYON_HAS_RECORDING
    return m_session->isRecording() || m_composed->isRecording();
#else
    return false;
#endif
}

void CaptureController::applySettings() {
    if (isRecording()) {
        m_pointerHighlight.setCursorReplicaEnabled(m_settings.showCursorInCaptures);
    }
}

// ---- Screenshots ---------------------------------------------------------------------------

void CaptureController::takeScreenshot(ScreenshotTarget target) {
    switch (target) {
    case ScreenshotTarget::ScreenUnderCursor:
        captureNativeArea(platform::screenMapping(screenUnderCursor()).native);
        return;
    case ScreenshotTarget::AllScreens:
        captureNativeArea(platform::currentDesktopLayout().nativeBounds());
        return;
    case ScreenshotTarget::RegionOrWindow:
        beginPick(PendingPick::Screenshot);
        return;
    }
}

void CaptureController::captureNativeArea(const QRect& nativeArea) {
    // Hide the draw-mode frame (a UI hint, not content) and give the compositor a moment to
    // show that, and the end of the picker, before grabbing.
    m_overlays.setDrawModeFrameVisible(false);
    QTimer::singleShot(kScreenshotSettleMs, this, [this, nativeArea] {
        capture::Grab grab = capture::grabNativeArea(nativeArea);
        m_overlays.setDrawModeFrameVisible(!isRecording());
        if (grab.image.isNull()) {
            m_notify(tr("Screenshot failed"), tr("Could not grab the screen."), true);
            return;
        }
        // While recording with the cursor option, the overlays already show a cursor replica.
        if (m_settings.showCursorInCaptures && !m_pointerHighlight.isCursorReplicaEnabled()) {
            const platform::CursorSprite cursor = platform::currentCursorSprite();
            // `visible` is ignored on purpose: screenshots are usually triggered from the
            // keyboard, and Windows hides the pointer while typing.
            if (!cursor.image.isNull()) {
                capture::paintCursor(grab, cursor.image, cursor.hotspot, QCursor::pos(),
                                     platform::currentDesktopLayout());
            }
        }
        const QString path = capture::newScreenshotPath(m_settings.screenshotFolder());
        if (!grab.image.save(path)) {
            m_notify(tr("Screenshot failed"), tr("Could not write %1").arg(path), true);
            return;
        }
        QGuiApplication::clipboard()->setImage(grab.image);
        m_notify(tr("Screenshot saved and copied to the clipboard"), QDir::toNativeSeparators(path),
                 false);
    });
}

// ---- Picking a region or window ------------------------------------------------------------

void CaptureController::beginPick(PendingPick purpose) {
    if (m_picker.isActive()) {
        return;
    }
    m_pendingPick = purpose;
    m_overlays.setPicking(true);
    m_picker.start();
}

void CaptureController::onPickFinished(const RegionPicker::Selection& selection) {
    m_overlays.setPicking(false);
    const PendingPick purpose = std::exchange(m_pendingPick, PendingPick::None);
    const DesktopLayout layout = platform::currentDesktopLayout();
    QRect nativeArea = layout.toNative(selection.logicalRect);
    if (selection.windowId != 0) {
        nativeArea = platform::windowBounds(selection.windowId).value_or(nativeArea);
    }
    qCDebug(lcCapture) << "Picked" << selection.logicalRect << "window" << selection.windowId
                       << "-> native" << nativeArea;
    if (nativeArea.isEmpty()) {
        onPickCanceled();
        return;
    }

    if (purpose == PendingPick::Screenshot) {
        captureNativeArea(nativeArea);
        return;
    }
    if (purpose != PendingPick::Recording) {
        return;
    }
    m_recordingLabel = selection.windowId == 0
                           ? tr("Region %1 × %2").arg(nativeArea.width()).arg(nativeArea.height())
                           : tr("Window");
    if (selection.windowId == 0) {
        startComposed([nativeArea] { return nativeArea; }, nativeArea.size());
        return;
    }
    // Follow the window: its bounds are read every frame; while minimized the last frame stays.
    const quintptr windowId = selection.windowId;
    startComposed(
        [windowId, last = nativeArea]() mutable {
            if (const auto bounds = platform::windowBounds(windowId)) {
                last = *bounds;
                return last;
            }
            return QRect();
        },
        nativeArea.size());
}

void CaptureController::onPickCanceled() {
    m_overlays.setPicking(false);
    if (std::exchange(m_pendingPick, PendingPick::None) == PendingPick::Recording) {
        setRecordingChecked(false);
    }
}

// ---- Recording -----------------------------------------------------------------------------

void CaptureController::startRecording(RecordingTarget target) {
#if RECRAYON_HAS_RECORDING
    if (isRecording() || m_picker.isActive()) {
        return;
    }
    setRecordingChecked(true);
    m_recordingLabel = SettingsDialog::targetLabel(target);
    m_encoding = videoEncoding(m_settings.videoPreset);
    const QString basePath = capture::newRecordingBasePath(m_settings.recordingFolder());
    const QAudioDevice microphone = microphoneFor(m_settings);
    m_withMicrophone = !microphone.isNull();
    if (m_settings.recordMicrophone && !m_withMicrophone) {
        m_notify(tr("No microphone found"), tr("Recording without audio."), true);
    }
    const DesktopLayout layout = platform::currentDesktopLayout();

    switch (target) {
    case RecordingTarget::ScreenUnderCursor:
        m_session->start({screenUnderCursor()}, basePath, m_encoding, microphone);
        return;
    case RecordingTarget::AllScreensSeparate:
        m_session->start(QGuiApplication::screens(), basePath, m_encoding, microphone);
        return;
    case RecordingTarget::AllScreensCombined: {
        const QRect bounds = layout.nativeBounds();
        startComposed([bounds] { return bounds; }, bounds.size());
        return;
    }
    case RecordingTarget::FollowCursor: {
        // Output large enough for the biggest screen; smaller ones are letterboxed.
        QSize largest;
        for (const ScreenMapping& screen : layout.screens()) {
            largest = largest.expandedTo(screen.native.size());
        }
        startComposed([] { return platform::screenMapping(screenUnderCursor()).native; }, largest);
        return;
    }
    case RecordingTarget::RegionOrWindow:
        beginPick(PendingPick::Recording);
        return;
    }
#else
    Q_UNUSED(target)
#endif
}

void CaptureController::startComposed(std::function<QRect()> view, const QSize& areaSize) {
#if RECRAYON_HAS_RECORDING
    const QSize outputSize = geometry::evenVideoSize(areaSize * m_encoding.scale);
    m_composed->start(std::move(view), outputSize,
                      capture::newRecordingBasePath(m_settings.recordingFolder()) +
                          QStringLiteral(".mp4"),
                      m_encoding, microphoneFor(m_settings));
#else
    Q_UNUSED(view)
    Q_UNUSED(areaSize)
#endif
}

void CaptureController::stopRecording() {
#if RECRAYON_HAS_RECORDING
    if (m_pendingPick == PendingPick::Recording) {
        m_picker.cancel();
        return;
    }
    m_session->stop();
    m_composed->stop();
#endif
}

void CaptureController::setRecordingPaused(bool paused) {
#if RECRAYON_HAS_RECORDING
    if (!isRecording() || paused == m_paused) {
        return;
    }
    m_paused = paused;
    m_session->setPaused(paused); // only the running recorder reacts
    m_composed->setPaused(paused);
    m_indicator->setPaused(paused);
    qCDebug(lcCapture) << (paused ? "Recording paused" : "Recording resumed");
#else
    Q_UNUSED(paused)
#endif
}

void CaptureController::onRecordingChanged() {
    const bool recording = isRecording();
    setRecordingChecked(recording);
    for (QAction* action : std::as_const(m_recordingTargetActions)) {
        action->setEnabled(!recording);
    }
    if (!recording) {
        m_paused = false;
    }
    m_overlays.setDrawModeFrameVisible(!recording);
#if RECRAYON_HAS_RECORDING
    // Native screen capture only delivers frames when the screen changes; the composed
    // recorder resends its last frames by itself.
    m_overlays.setCaptureHeartbeat(m_session->isRecording() ? m_encoding.frameRate : 0);
#endif
    // Screen capture leaves the cursor out; the overlays paint a replica that gets recorded.
    m_pointerHighlight.setCursorReplicaEnabled(recording && m_settings.showCursorInCaptures);

    if (m_indicator) {
        if (recording && !m_indicator->isVisible()) {
            m_indicator->start(m_withMicrophone ? tr("%1 + microphone").arg(m_recordingLabel)
                                                : m_recordingLabel);
        } else if (!recording) {
            m_indicator->stop();
        }
    }
}

void CaptureController::onRecordingFinished(const QStringList& paths) {
    QStringList native;
    std::transform(paths.cbegin(), paths.cend(), std::back_inserter(native),
                   [](const QString& path) { return QDir::toNativeSeparators(path); });
    m_notify(paths.size() == 1 ? tr("Recording saved")
                               : tr("%1 recordings saved").arg(paths.size()),
             native.join(QLatin1Char('\n')), false);
}

void CaptureController::onRecordingError(const QString& message) {
    qCWarning(lcCapture) << "Recording failed:" << message;
    m_notify(tr("Recording failed"), message, true);
}

void CaptureController::setRecordingChecked(bool checked) {
    if (m_toggleRecording) {
        const QSignalBlocker blocker(m_toggleRecording);
        m_toggleRecording->setChecked(checked);
    }
}

} // namespace recrayon
