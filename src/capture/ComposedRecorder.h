#pragma once

#include "config/Settings.h"

#include <QAudioDevice>
#include <QElapsedTimer>
#include <QImage>
#include <QMediaCaptureSession>
#include <QMediaRecorder>
#include <QObject>
#include <QRect>
#include <QString>
#include <QTimer>

#include <functional>
#include <memory>
#include <vector>

class QAudioInput;
class QVideoFrameInput;

namespace recrayon::capture {

/// Records an arbitrary, possibly moving, area of the virtual desktop into one MP4 file.
///
/// Every screen is captured natively (QScreenCapture → QVideoSink). At the output frame rate
/// the recorder asks its ViewProvider which native desktop area to show, paints the matching
/// parts of the latest screen frames into one image (letterboxed to the fixed output size) and
/// feeds it to QMediaRecorder through QVideoFrameInput. That single mechanism covers:
///  - all screens in one video (view = union of the screens),
///  - following the pointer (view = screen under the cursor, re-evaluated every frame),
///  - a region (fixed view) or a window (view = its current bounds).
///
/// Requires Qt Multimedia 6.8 (QVideoFrameInput).
class ComposedRecorder final : public QObject {
    Q_OBJECT

public:
    /// Native desktop rectangle to show in the next frame.
    using ViewProvider = std::function<QRect()>;

    explicit ComposedRecorder(QObject* parent = nullptr);
    ~ComposedRecorder() override;

    [[nodiscard]] bool isRecording() const noexcept { return m_running; }

    /// Starts recording. @p outputSize must be even in both dimensions (see
    /// geometry::evenVideoSize()). Ignored if already recording.
    /// A non-null @p microphone is recorded as the audio track.
    void start(ViewProvider view, const QSize& outputSize, const QString& filePath,
               const VideoEncoding& encoding, const QAudioDevice& microphone = {});
    /// Stops and finalizes the file. Safe to call twice.
    void stop();
    /// Pauses / resumes: no frames are sent while paused and the paused time is left out of the
    /// timestamps, so the video continues seamlessly. With a microphone the recorder itself is
    /// paused too, otherwise the audio would keep running.
    void setPaused(bool paused);

signals:
    void recordingChanged(bool recording);
    void finished(const QString& filePath);
    void errorOccurred(const QString& message);

private:
    struct Source;

    void composeFrame();
    void onError(const QString& message);
    void finish();

    std::vector<std::unique_ptr<Source>> m_sources;
    QMediaCaptureSession m_session;
    QMediaRecorder m_recorder;
    std::unique_ptr<QVideoFrameInput> m_input;
    std::unique_ptr<QAudioInput> m_audioInput;
    QTimer m_timer;
    QElapsedTimer m_clock;
    qint64 m_frameDurationUs = 0;
    qint64 m_pausedNs = 0;      ///< total time spent paused, removed from frame timestamps
    qint64 m_pauseStartNs = -1; ///< clock value when the current pause began, -1 if running
    ViewProvider m_view;
    QImage m_canvas;
    bool m_running = false;
    bool m_failed = false;
};

} // namespace recrayon::capture
