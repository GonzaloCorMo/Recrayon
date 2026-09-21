#include "capture/RecordingSession.h"

#include "capture/ScreenRecorder.h"

#include <QScreen>

namespace recrayon::capture {

RecordingSession::RecordingSession(QObject* parent) : QObject(parent) {}

RecordingSession::~RecordingSession() = default;

void RecordingSession::start(const QList<QScreen*>& screens, const QString& basePath,
                             const VideoEncoding& encoding, const QAudioDevice& microphone) {
    if (m_active || screens.isEmpty()) {
        return;
    }
    m_recorders.clear();
    m_finishedFiles.clear();
    m_failed = false;
    m_active = true;
    m_running = screens.size();
    emit recordingChanged(true);

    for (qsizetype i = 0; i < screens.size(); ++i) {
        auto recorder = std::make_unique<ScreenRecorder>();
        ScreenRecorder* raw = recorder.get();
        connect(raw, &ScreenRecorder::stopped, this, &RecordingSession::onRecorderStopped);
        connect(raw, &ScreenRecorder::errorOccurred, this, [this](const QString& message) {
            if (!m_failed) {
                m_failed = true;
                emit errorOccurred(message);
                stop(); // one failed screen stops the whole session
            }
        });
        m_recorders.push_back(std::move(recorder));

        const QString path = screens.size() == 1
                                 ? basePath + QStringLiteral(".mp4")
                                 : QStringLiteral("%1_screen%2.mp4").arg(basePath).arg(i + 1);
        raw->start(screens.at(i), path, encoding, i == 0 ? microphone : QAudioDevice());
    }
}

void RecordingSession::stop() {
    for (const auto& recorder : m_recorders) {
        recorder->stop();
    }
}

void RecordingSession::setPaused(bool paused) {
    for (const auto& recorder : m_recorders) {
        recorder->setPaused(paused);
    }
}

void RecordingSession::onRecorderStopped(const QString& filePath) {
    if (!m_active) {
        return;
    }
    if (!filePath.isEmpty()) {
        m_finishedFiles.append(filePath);
    }
    if (--m_running > 0) {
        return;
    }
    m_active = false;
    emit recordingChanged(false);
    if (!m_finishedFiles.isEmpty()) {
        emit finished(m_finishedFiles);
    }
}

} // namespace recrayon::capture
