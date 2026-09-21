#include "capture/ComposedRecorder.h"

#include "core/Geometry.h"
#include "platform/DesktopGeometry.h"

#include <QAudioInput>
#include <QGuiApplication>
#include <QMediaFormat>
#include <QMutex>
#include <QMutexLocker>
#include <QPainter>
#include <QScreen>
#include <QScreenCapture>
#include <QUrl>
#include <QVideoFrame>
#include <QVideoFrameInput>
#include <QVideoSink>

namespace recrayon::capture {

/// Native capture of one screen. Frames may arrive on a capture thread, so only the latest one
/// is kept (under a mutex) and converted to an image when the compositor needs it.
struct ComposedRecorder::Source {
    QRect native;
    QScreenCapture capture;
    QMediaCaptureSession session;
    QVideoSink sink;

    QMutex mutex;
    QVideoFrame pending;
    bool hasPending = false;

    QImage image; // last converted frame, GUI thread only

    ~Source() {
        capture.setActive(false);
        session.setVideoSink(nullptr);
        session.setScreenCapture(nullptr);
    }

    /// Converts the newest frame, if any arrived since the last call.
    void refreshImage() {
        QVideoFrame frame;
        {
            const QMutexLocker locker(&mutex);
            if (!hasPending) {
                return;
            }
            frame = std::move(pending);
            pending = {};
            hasPending = false;
        }
        QImage converted = frame.toImage();
        if (!converted.isNull()) {
            image = std::move(converted);
        }
    }
};

ComposedRecorder::ComposedRecorder(QObject* parent) : QObject(parent) {
    m_session.setRecorder(&m_recorder);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &ComposedRecorder::composeFrame);

    connect(&m_recorder, &QMediaRecorder::recorderStateChanged, this,
            [this](QMediaRecorder::RecorderState state) {
                if (state == QMediaRecorder::StoppedState) {
                    finish();
                }
            });
    connect(&m_recorder, &QMediaRecorder::errorOccurred, this,
            [this](QMediaRecorder::Error /*error*/, const QString& message) { onError(message); });
}

ComposedRecorder::~ComposedRecorder() {
    m_running = false; // no signals from a half-destroyed object
    m_timer.stop();
    m_recorder.stop();
    m_sources.clear();
    m_session.setVideoFrameInput(nullptr);
    m_session.setAudioInput(nullptr);
    m_session.setRecorder(nullptr);
}

void ComposedRecorder::start(ViewProvider view, const QSize& outputSize, const QString& filePath,
                             const VideoEncoding& encoding, const QAudioDevice& microphone) {
    if (m_running || !view || outputSize.isEmpty()) {
        return;
    }
    m_running = true;
    m_failed = false;
    m_view = std::move(view);

    m_sources.clear();
    const auto screens = QGuiApplication::screens();
    for (QScreen* screen : screens) {
        auto source = std::make_unique<Source>();
        Source* raw = source.get();
        raw->native = platform::screenMapping(screen).native;
        raw->session.setScreenCapture(&raw->capture);
        raw->session.setVideoSink(&raw->sink);
        // Direct connection: runs on whatever thread delivers the frame.
        connect(
            &raw->sink, &QVideoSink::videoFrameChanged, &raw->sink,
            [raw](const QVideoFrame& frame) {
                const QMutexLocker locker(&raw->mutex);
                raw->pending = frame;
                raw->hasPending = true;
            },
            Qt::DirectConnection);
        connect(
            &raw->capture, &QScreenCapture::errorOccurred, this,
            [this](QScreenCapture::Error /*error*/, const QString& message) { onError(message); });
        raw->capture.setScreen(screen);
        raw->capture.setActive(true);
        m_sources.push_back(std::move(source));
    }

    m_canvas = QImage(outputSize, QImage::Format_RGB32);
    m_canvas.fill(Qt::black);
    // Default-constructed on purpose: in Qt 6.8.3 an input created with an explicit
    // QVideoFrameFormat never becomes ready and rejects every frame. Without one, the format is
    // taken from the first frame.
    m_input = std::make_unique<QVideoFrameInput>();
    m_session.setVideoFrameInput(m_input.get());
    if (microphone.isNull()) {
        m_session.setAudioInput(nullptr);
        m_audioInput.reset();
    } else {
        m_audioInput = std::make_unique<QAudioInput>(microphone);
        m_session.setAudioInput(m_audioInput.get());
    }

    QMediaFormat mediaFormat;
    mediaFormat.setFileFormat(QMediaFormat::MPEG4);
    mediaFormat.setVideoCodec(QMediaFormat::VideoCodec::H264);
    mediaFormat.setAudioCodec(QMediaFormat::AudioCodec::AAC);
    m_recorder.setMediaFormat(mediaFormat);
    m_recorder.setQuality(static_cast<QMediaRecorder::Quality>(encoding.qualityLevel));
    m_recorder.setVideoFrameRate(encoding.frameRate);
    m_recorder.setVideoResolution(outputSize);
    m_recorder.setOutputLocation(QUrl::fromLocalFile(filePath));
    m_recorder.record();

    m_pausedNs = 0;
    m_pauseStartNs = -1;
    m_frameDurationUs = 1'000'000 / encoding.frameRate;
    m_timer.setInterval(1000 / encoding.frameRate);
    m_clock.start();
    m_timer.start();
    emit recordingChanged(true);
}

void ComposedRecorder::composeFrame() {
    if (!m_running || !m_input) {
        return;
    }
    const QRect view = m_view();
    if (view.isEmpty()) {
        return; // e.g. the followed window is minimized: keep the last frame
    }

    m_canvas.fill(Qt::black);
    {
        QPainter painter(&m_canvas);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        const QRectF target =
            geometry::fitCentered(QSizeF(view.size()), QRectF(QPointF(0, 0), m_canvas.size()));
        painter.setClipRect(target);
        painter.translate(target.topLeft());
        painter.scale(target.width() / view.width(), target.height() / view.height());
        painter.translate(-QPointF(view.topLeft()));
        for (const auto& source : m_sources) {
            if (!source->native.intersects(view)) {
                continue;
            }
            source->refreshImage();
            if (!source->image.isNull()) {
                painter.drawImage(QRectF(source->native), source->image);
            }
        }
    }

    QVideoFrame frame(m_canvas);
    const qint64 now = (m_clock.nsecsElapsed() - m_pausedNs) / 1000;
    frame.setStartTime(now);
    frame.setEndTime(now + m_frameDurationUs);
    // Returns false while the encoder is still starting or its queue is full: the frame is
    // simply dropped, the next tick sends a fresh one.
    m_input->sendVideoFrame(frame);
}

void ComposedRecorder::stop() {
    if (!m_running) {
        return;
    }
    m_timer.stop();
    if (m_recorder.recorderState() == QMediaRecorder::StoppedState) {
        finish();
    } else {
        m_recorder.stop(); // finish() runs when the file is finalized
    }
}

void ComposedRecorder::setPaused(bool paused) {
    if (!m_running || paused == (m_pauseStartNs >= 0)) {
        return;
    }
    if (paused) {
        m_timer.stop();
        m_pauseStartNs = m_clock.nsecsElapsed();
        if (m_audioInput) {
            m_recorder.pause();
        }
    } else {
        m_pausedNs += m_clock.nsecsElapsed() - m_pauseStartNs;
        m_pauseStartNs = -1;
        if (m_audioInput) {
            m_recorder.record();
        }
        m_timer.start();
    }
}

void ComposedRecorder::onError(const QString& message) {
    if (!m_running) {
        return;
    }
    m_failed = true;
    emit errorOccurred(message);
    stop();
}

void ComposedRecorder::finish() {
    if (!m_running) {
        return;
    }
    m_running = false;
    m_timer.stop();
    m_sources.clear();
    m_session.setVideoFrameInput(nullptr);
    m_input.reset();
    m_session.setAudioInput(nullptr);
    m_audioInput.reset();
    emit recordingChanged(false);
    if (!m_failed) {
        emit finished(m_recorder.actualLocation().toLocalFile());
    }
}

} // namespace recrayon::capture
