#include "ui/OverlayWindow.h"

#include "core/Document.h"
#include "core/Item.h"
#include "tools/ToolController.h"
#include "tools/ToolKind.h"
#include "ui/OverlayHint.h"
#include "ui/PointerHighlight.h"
#include "ui/RegionPicker.h"
#include "ui/ReplayPlayer.h"
#include "ui/TextEditBox.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QScreen>
#include <QWindow>

namespace recrayon {

namespace {

// Windows forwards mouse input on fully transparent pixels of a layered window to whatever is
// below it. In draw mode we need every pixel to catch the pointer, so the backdrop gets the
// smallest non-zero alpha: invisible to the eye, opaque to hit-testing.
const QColor kDrawModeBackdrop(0, 0, 0, 1);

// Thin frame around the screen so it is obvious that clicks are being captured.
const QColor kDrawModeFrame(0x8A, 0xB4, 0xF8, 180);
constexpr int kDrawModeFrameWidth = 3;

constexpr int kRepaintMargin = 2;

const QRect kHeartbeatPixel(0, 0, 1, 1);

} // namespace

OverlayWindow::OverlayWindow(QScreen* screen, Document& document, ToolController& tools,
                             PointerHighlight& pointerHighlight, RegionPicker& picker,
                             ReplayPlayer& replay)
    : QWidget(nullptr, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool |
                           Qt::NoDropShadowWindowHint),
      m_screen(screen), m_document(&document), m_tools(tools), m_pointerHighlight(pointerHighlight),
      m_picker(picker), m_replay(replay) {
    Q_ASSERT(screen);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setWindowTitle(QStringLiteral("Recrayon overlay"));
    updateCursor();

    connect(m_document, &Document::changed, this, &OverlayWindow::onDocumentChanged);
    connect(&m_tools, &ToolController::previewChanged, this, &OverlayWindow::onPreviewChanged);
    connect(&m_tools, &ToolController::toolChanged, this, &OverlayWindow::updateCursor);
    connect(&m_pointerHighlight, &PointerHighlight::moved, this, &OverlayWindow::onPreviewChanged);
    connect(&m_pointerHighlight, &PointerHighlight::appearanceChanged, this, [this] { update(); });
    connect(&m_picker, &RegionPicker::changed, this, [this] { update(); });
    connect(&m_tools, &ToolController::textCommitRequested, this, [this] {
        if (m_textEdit) {
            m_textEdit->commit();
        }
    });
    connect(&m_replay, &ReplayPlayer::frame, this, [this] { update(); });
    connect(screen, &QScreen::geometryChanged, this, &OverlayWindow::syncGeometry);

    m_heartbeat.setTimerType(Qt::PreciseTimer);
    connect(&m_heartbeat, &QTimer::timeout, this, [this] {
        m_heartbeatPhase = !m_heartbeatPhase;
        update(kHeartbeatPixel);
    });

    syncGeometry();
    winId(); // create the native window now so the input mode can be applied before showing
    applyInputMode();
}

void OverlayWindow::setDrawingEnabled(bool enabled) {
    if (enabled == m_drawing) {
        return;
    }
    if (!enabled && m_pressed) {
        m_pressed = false;
        m_tools.cancel();
    }
    m_drawing = enabled;
    applyInputMode();
    update();
}

void OverlayWindow::setPickingEnabled(bool enabled) {
    if (enabled == m_picking) {
        return;
    }
    if (enabled && m_pressed) {
        m_pressed = false;
        m_tools.cancel();
    }
    m_picking = enabled;
    setMouseTracking(enabled); // hover highlights windows before any button is pressed
    applyInputMode();
    updateCursor();
    update();
}

void OverlayWindow::setDocument(Document& document) {
    if (&document == m_document) {
        return;
    }
    disconnect(m_document, &Document::changed, this, &OverlayWindow::onDocumentChanged);
    m_document = &document;
    connect(m_document, &Document::changed, this, &OverlayWindow::onDocumentChanged);
    m_cacheDirty = true;
    update();
}

void OverlayWindow::setBackground(const std::optional<QColor>& color) {
    m_background = color;
    update();
}

void OverlayWindow::takeFocus() {
    raise();
    activateWindow();
    emit raised(); // the toolbar has to be raised again on top of us
}

void OverlayWindow::beginTextInput(const TextRequest& request, const QFont& font,
                                   const QColor& color) {
    endTextInput();
    m_textEdit = new TextEditBox(request, request.anchor - m_origin, font, color, this);
    connect(m_textEdit, &TextEditBox::committed, this,
            [this](const QString& text) { m_tools.commitText(text); });
    connect(m_textEdit, &TextEditBox::canceled, this, [this] { m_tools.cancelText(); });
    takeFocus(); // typing needs keyboard focus on this window
    m_textEdit->setFocus(Qt::OtherFocusReason);
    update(); // the hint now explains Enter / Esc
}

void OverlayWindow::endTextInput() {
    if (m_textEdit) {
        TextEditBox* edit = m_textEdit;
        m_textEdit = nullptr;
        edit->hide();
        edit->deleteLater();
        update();
    }
}

void OverlayWindow::setFrameVisible(bool visible) {
    if (visible == m_frameVisible) {
        return;
    }
    m_frameVisible = visible;
    if (m_drawing) {
        repaint(); // synchronous: callers may grab the screen right after
    }
}

void OverlayWindow::setCaptureHeartbeat(int framesPerSecond) {
    if (framesPerSecond > 0) {
        m_heartbeat.start(1000 / framesPerSecond);
    } else {
        m_heartbeat.stop();
        update(kHeartbeatPixel);
    }
}

void OverlayWindow::applyInputMode() {
    // Toggle on the QWindow, not via QWidget::setWindowFlags(): the latter recreates and hides
    // the native window, which would flicker and lose the always-on-top state.
    if (QWindow* window = windowHandle()) {
        window->setFlag(Qt::WindowTransparentForInput, !acceptsInput());
    }
}

void OverlayWindow::syncGeometry() {
    const QRect geometry = m_screen->geometry();
    m_origin = geometry.topLeft();
    setGeometry(geometry);
    m_cacheDirty = true;
    update();
}

void OverlayWindow::resizeEvent(QResizeEvent* event) {
    m_cacheDirty = true;
    QWidget::resizeEvent(event);
}

void OverlayWindow::rebuildCache() {
    const qreal dpr = devicePixelRatio();
    const QSize pixelSize = (QSizeF(size()) * dpr).toSize();
    if (m_cache.size() != pixelSize || !qFuzzyCompare(m_cache.devicePixelRatio(), dpr)) {
        m_cache = QPixmap(pixelSize);
        m_cache.setDevicePixelRatio(dpr);
    }
    m_cache.fill(Qt::transparent);

    QPainter painter(&m_cache);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(-m_origin);
    const QRectF visible(m_origin, QSizeF(size()));
    for (const ItemPtr& item : m_document->items()) {
        if (item->boundingRect().intersects(visible)) {
            item->paint(painter);
        }
    }
    m_cacheDirty = false;
}

void OverlayWindow::paintEvent(QPaintEvent* event) {
    if (m_cacheDirty) {
        rebuildCache();
    }

    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    if (m_background) {
        painter.fillRect(event->rect(), *m_background); // whiteboard: opaque page
    } else {
        painter.fillRect(event->rect(),
                         acceptsInput() ? kDrawModeBackdrop : QColor(Qt::transparent));
    }
    if (m_heartbeat.isActive() && event->rect().intersects(kHeartbeatPixel)) {
        // Alpha 1 <-> 2: invisible, but a real content change for the compositor.
        painter.fillRect(kHeartbeatPixel, QColor(0, 0, 0, m_heartbeatPhase ? 2 : 1));
    }
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    if (m_replay.isActive()) {
        // Replays redraw the page from scratch instead of showing the finished annotations.
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing);
        painter.translate(-m_origin);
        m_replay.paint(painter, m_document, QRectF(m_origin, QSizeF(size())));
        painter.restore();
    } else {
        painter.drawPixmap(QPointF(0, 0), m_cache);
    }

    if (const Item* preview = m_replay.isActive() ? nullptr : m_tools.preview()) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing);
        painter.translate(-m_origin);
        preview->paint(painter);
        painter.restore();
    }

    if (m_pointerHighlight.isActive()) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing);
        painter.translate(-m_origin);
        m_pointerHighlight.paint(painter, QRectF(m_origin, QSizeF(size())));
        painter.restore();
    }

    if (m_picking) {
        painter.save();
        painter.translate(-m_origin);
        m_picker.paint(painter, QRectF(m_origin, QSizeF(size())));
        painter.restore();
    } else if (m_frameVisible) {
        paintModeHint(painter);
    }
    if (!m_picking && m_drawing && m_frameVisible) {
        painter.setPen(QPen(kDrawModeFrame, kDrawModeFrameWidth));
        painter.setBrush(Qt::NoBrush);
        const int inset = kDrawModeFrameWidth / 2;
        painter.drawRect(rect().adjusted(inset, inset, -inset - 1, -inset - 1));
    }
}

void OverlayWindow::mousePressEvent(QMouseEvent* event) {
    if (m_picking) {
        if (event->button() == Qt::LeftButton) {
            m_picker.press(toDocument(event->position()));
        } else if (event->button() == Qt::RightButton) {
            m_picker.cancel();
        }
        event->accept();
        return;
    }
    if (m_replay.isActive()) {
        event->accept(); // no drawing while a replay is running
        return;
    }
    if (m_textEdit) {
        // Clicking outside the editor finishes the text instead of starting a new gesture.
        m_textEdit->commit();
        event->accept();
        return;
    }
    if (!m_drawing || event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }
    m_pressed = true;
    // With a single-screen whiteboard, screens show different pages: draw on this one's.
    m_tools.setDocument(*m_document);
    m_tools.press(toDocument(event->position()));
    event->accept();
}

void OverlayWindow::mouseMoveEvent(QMouseEvent* event) {
    if (m_picking) {
        m_picker.move(toDocument(event->position()));
        event->accept();
        return;
    }
    if (!m_pressed) {
        event->ignore();
        return;
    }
    m_tools.move(toDocument(event->position()));
    event->accept();
}

void OverlayWindow::mouseReleaseEvent(QMouseEvent* event) {
    if (m_picking) {
        if (event->button() == Qt::LeftButton) {
            m_picker.release(toDocument(event->position()));
        }
        event->accept();
        return;
    }
    if (!m_pressed || event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }
    m_pressed = false;
    m_tools.release(toDocument(event->position()));
    event->accept();
}

void OverlayWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        if (m_replay.isActive()) {
            m_replay.stop();
        } else if (m_picking) {
            m_picker.cancel();
        } else {
            emit escapePressed();
        }
        return;
    }
    if (m_picking) {
        return;
    }

    // Tool keys (1..9, 0, M), see kToolKeys.
    const Qt::KeyboardModifiers modifiers =
        event->modifiers() & ~Qt::KeyboardModifiers(Qt::KeypadModifier);
    if (!modifiers) {
        for (const ToolKey& entry : kToolKeys) {
            if (event->key() == entry.key) {
                m_tools.setCurrentTool(entry.kind);
                return;
            }
        }
    }

    QWidget::keyPressEvent(event);
}

void OverlayWindow::onDocumentChanged(const QRectF& dirtyRect) {
    m_cacheDirty = true;
    update(dirtyRect.isNull() ? rect() : toLocal(dirtyRect));
}

void OverlayWindow::onPreviewChanged(const QRectF& dirtyRect) {
    update(toLocal(dirtyRect));
}

void OverlayWindow::paintModeHint(QPainter& painter) {
    // Tell the user how to get out of every mode that captures the pointer or the keyboard.
    // Like the draw-mode frame, hints are hidden during screenshots and recordings.
    QString hint;
    if (m_replay.isActive()) {
        hint = tr("Replaying  ·  Esc to stop");
    } else if (m_textEdit) {
        hint = tr("Enter to confirm  ·  Shift+Enter for a new line  ·  Esc to cancel");
    } else if (m_drawing && m_background) {
        hint = tr("Whiteboard  ·  Esc to leave");
    } else if (m_drawing) {
        hint = tr("Draw mode  ·  Esc to go back to your applications");
    }
    if (!hint.isEmpty()) {
        hints::paintBanner(painter, QRectF(rect()), hint);
    }
}

void OverlayWindow::updateCursor() {
    setCursor(!m_picking && m_tools.currentTool() == ToolKind::Move ? Qt::SizeAllCursor
                                                                    : Qt::CrossCursor);
}

QRect OverlayWindow::toLocal(const QRectF& documentRect) const {
    return documentRect.translated(-m_origin).toAlignedRect().adjusted(
        -kRepaintMargin, -kRepaintMargin, kRepaintMargin, kRepaintMargin);
}

} // namespace recrayon
