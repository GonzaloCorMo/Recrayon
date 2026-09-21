#include "capture/ScreenRecorder.h"

#include "core/Geometry.h"
#include "platform/DesktopGeometry.h"

#include <QAudioInput>
#include <QMediaFormat>
#include <QScreen>
#include <QUrl>

namespace recrayon::capture {

ScreenRecorder::ScreenRecorder(QObject* parent) : QObject(parent) {
    m_session.setScreenCapture(&m_screenCapture);
    m_session.setRecorder(&m_recorder);

    connect(&m_recorder, &QMediaRecorder::recorderStateChanged, this,
            [this](QMediaRecorder::RecorderState state) {
                if (state == QMediaRecorder::StoppedState) {
                    finish();
                }
            });
    connect(&m_recorder, &QMediaRecorder::errorOccurred, this,
            [this](QMediaRecorder::Error /*error*/, const QString& message) { onError(message); });
    connect(&m_screenCapture, &QScreenCapture::errorOccurred, this,
            [this](QScreenCapture::Error /*error*/, const QString& message) { onError(message); });
}

ScreenRecorder::~ScreenRecorder() {
    m_running = false; // no signals from a half-destroyed object
    m_recorder.stop();
    m_screenCapture.setActive(false);
    m_session.setAudioInput(nullptr);
    m_session.setRecorder(nullptr);
    m_session.setScreenCapture(nullptr);
}

void ScreenRecorder::start(QScreen* screen, const QString& filePath, const VideoEncoding& encoding,
                           const QAudioDevice& microphone) {
    if (m_running || !screen) {
        return;
    }
    m_running = true;
    m_failed = false;

    m_screenCapture.setScreen(screen);
    m_screenCapture.setActive(true);

    if (microphone.isNull()) {
        m_session.setAudioInput(nullptr);
        m_audioInput.reset();
    } else {
        m_audioInput = std::make_unique<QAudioInput>(microphone);
        m_session.setAudioInput(m_audioInput.get());
    }

    QMediaFormat format;
    format.setFileFormat(QMediaFormat::MPEG4);
    format.setVideoCodec(QMediaFormat::VideoCodec::H264);
    format.setAudioCodec(QMediaFormat::AudioCodec::AAC);
    m_recorder.setMediaFormat(format);
    m_recorder.setQuality(static_cast<QMediaRecorder::Quality>(encoding.qualityLevel));
    m_recorder.setVideoFrameRate(encoding.frameRate);
    // Empty = the capture's own size; otherwise the encoder scales every frame.
    m_recorder.setVideoResolution(
        encoding.scale == 1.0
            ? QSize()
            : geometry::evenVideoSize(platform::screenMapping(screen).native.size() *
                                      encoding.scale));
    m_recorder.setOutputLocation(QUrl::fromLocalFile(filePath));
    m_recorder.record();
}

void ScreenRecorder::stop() {
    if (!m_running) {
        return;
    }
    if (m_recorder.recorderState() == QMediaRecorder::StoppedState) {
        finish(); // never got to record
    } else {
        m_recorder.stop(); // finish() runs when the file is finalized
    }
}

void ScreenRecorder::setPaused(bool paused) {
    if (!m_running) {
        return;
    }
    const QMediaRecorder::RecorderState state = m_recorder.recorderState();
    if (paused && state == QMediaRecorder::RecordingState) {
        m_recorder.pause();
    } else if (!paused && state == QMediaRecorder::PausedState) {
        m_recorder.record(); // resumes into the same file
    }
}

void ScreenRecorder::onError(const QString& message) {
    if (!m_running) {
        return;
    }
    m_failed = true;
    emit errorOccurred(message);
    stop();
}

void ScreenRecorder::finish() {
    if (!m_running) {
        return;
    }
    m_running = false;
    m_screenCapture.setActive(false);
    m_session.setAudioInput(nullptr);
    m_audioInput.reset();
    emit stopped(m_failed ? QString() : m_recorder.actualLocation().toLocalFile());
}

} // namespace recrayon::capture
