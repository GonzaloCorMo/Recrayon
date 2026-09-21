#pragma once

#include "config/Settings.h"

#include <QAudioDevice>
#include <QMediaCaptureSession>
#include <QMediaRecorder>
#include <QObject>
#include <QScreenCapture>
#include <QString>

#include <memory>

class QAudioInput;
class QScreen;

namespace recrayon::capture {

/// Records one screen (overlays included, mouse cursor not) to an MP4/H.264 file using Qt
/// Multimedia's FFmpeg backend. Only built when Qt Multimedia is available
/// (RECRAYON_HAS_RECORDING). Use RecordingSession to record several screens.
class ScreenRecorder final : public QObject {
    Q_OBJECT

public:
    explicit ScreenRecorder(QObject* parent = nullptr);
    ~ScreenRecorder() override;

    [[nodiscard]] bool isRecording() const noexcept { return m_running; }

    /// Starts recording @p screen into @p filePath. Ignored if already recording. The video is
    /// the screen's native size times @p encoding.scale. A non-null @p microphone is recorded
    /// as the audio track.
    void start(QScreen* screen, const QString& filePath, const VideoEncoding& encoding,
               const QAudioDevice& microphone = {});
    /// Stops and finalizes the file. stopped() follows once it is written. Safe to call twice.
    void stop();
    /// Pauses / resumes the recording; the paused time is left out of the video.
    void setPaused(bool paused);

signals:
    /// Emitted exactly once per start(): with the written file, or an empty path on failure.
    void stopped(const QString& filePath);
    void errorOccurred(const QString& message);

private:
    void onError(const QString& message);
    void finish();

    QScreenCapture m_screenCapture;
    QMediaCaptureSession m_session;
    QMediaRecorder m_recorder;
    std::unique_ptr<QAudioInput> m_audioInput;
    bool m_running = false;
    bool m_failed = false;
};

} // namespace recrayon::capture
