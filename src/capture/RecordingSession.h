#pragma once

#include "config/Settings.h"

#include <QAudioDevice>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

#include <memory>
#include <vector>

class QScreen;

namespace recrayon::capture {

class ScreenRecorder;

/// Records one or several screens at the same time, one MP4 file per screen, using native
/// screen capture for each. Behaves as a single recording: it is "recording" while any screen
/// is, and finished() reports all files once every recorder has stopped.
class RecordingSession final : public QObject {
    Q_OBJECT

public:
    explicit RecordingSession(QObject* parent = nullptr);
    ~RecordingSession() override;

    [[nodiscard]] bool isRecording() const noexcept { return m_active; }

    /// Starts recording @p screens. Files are "<basePath>.mp4" for a single screen, or
    /// "<basePath>_screen<N>.mp4" (N = 1-based index in @p screens) for several.
    /// A non-null @p microphone is recorded into the first file only (one device, one track).
    void start(const QList<QScreen*>& screens, const QString& basePath,
               const VideoEncoding& encoding, const QAudioDevice& microphone = {});
    void stop();
    /// Pauses / resumes every screen at once.
    void setPaused(bool paused);

signals:
    void recordingChanged(bool recording);
    void finished(const QStringList& filePaths);
    void errorOccurred(const QString& message);

private:
    void onRecorderStopped(const QString& filePath);

    std::vector<std::unique_ptr<ScreenRecorder>> m_recorders;
    QStringList m_finishedFiles;
    qsizetype m_running = 0;
    bool m_active = false;
    bool m_failed = false;
};

} // namespace recrayon::capture
