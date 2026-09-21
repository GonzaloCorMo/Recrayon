#pragma once

#include <QElapsedTimer>
#include <QString>
#include <QTimer>
#include <QWidget>

class QLabel;
class QToolButton;

namespace recrayon {

/// Small always-on-top badge shown while a recording is running: a blinking red dot, the
/// elapsed time, a pause / resume button and a stop button. It tells the user at a glance that the
/// screen is being recorded, which is easy to forget with a click-through overlay.
///
/// It is a separate window (not an overlay) so it can be excluded from the capture itself; the
/// application calls platform::setExcludedFromCapture() on it, like it does for the toolbar.
/// Drag it anywhere with the mouse; the position is kept while the application runs.
class RecordingIndicator final : public QWidget {
    Q_OBJECT

public:
    explicit RecordingIndicator(QWidget* parent = nullptr);

    /// Shows the badge and restarts the clock. @p target describes what is being recorded.
    void start(const QString& target);
    /// Hides the badge and stops the clock.
    void stop();
    /// Shows the paused state: the clock stops and the button offers to resume.
    void setPaused(bool paused);
    [[nodiscard]] bool isPaused() const noexcept { return m_paused; }

    /// Places it at the top center of the screen under the pointer, unless the user moved it.
    void placeOnScreenUnderCursor();

signals:
    /// The user clicked the stop button.
    void stopRequested();
    /// The user clicked pause (@p paused true) or resume.
    void pauseRequested(bool paused);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    void tick();
    void updateTexts();
    [[nodiscard]] qint64 elapsedMs() const;

    QLabel* m_time = nullptr;
    QLabel* m_target = nullptr;
    QString m_targetText;
    QToolButton* m_pause = nullptr;
    QTimer m_timer;
    QElapsedTimer m_clock;   ///< running since the last start / resume
    qint64 m_recordedMs = 0; ///< recorded time before the last pause
    bool m_paused = false;
    bool m_blinkOn = true;
    bool m_moved = false; ///< the user grabbed it: stop placing it automatically
};

} // namespace recrayon
