#include "ui/OverlayManager.h"

#include "tools/ToolController.h"
#include "ui/OverlayWindow.h"

#include <QCursor>
#include <QGuiApplication>
#include <QScreen>

#include <algorithm>
#include <utility>

namespace recrayon {

OverlayManager::OverlayManager(Document& document, ToolController& tools,
                               PointerHighlight& pointerHighlight, RegionPicker& picker,
                               ReplayPlayer& replay, QObject* parent)
    : QObject(parent), m_desktopDocument(document), m_tools(tools),
      m_pointerHighlight(pointerHighlight), m_picker(picker), m_replay(replay) {
    const auto screens = QGuiApplication::screens();
    for (QScreen* screen : screens) {
        addOverlay(screen);
    }
    connect(qGuiApp, &QGuiApplication::screenAdded, this, &OverlayManager::addOverlay);
    connect(&m_tools, &ToolController::textRequested, this,
            [this](const TextRequest& request, const QFont& font, const QColor& color) {
                if (OverlayWindow* overlay = overlayAt(request.anchor)) {
                    overlay->beginTextInput(request, font, color);
                } else {
                    m_tools.cancelText();
                }
            });
    connect(&m_tools, &ToolController::textInputClosed, this, [this] {
        for (const auto& overlay : m_overlays) {
            overlay->endTextInput();
        }
    });
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &OverlayManager::removeOverlay);
}

OverlayManager::~OverlayManager() = default;

void OverlayManager::setMode(InteractionMode mode) {
    if (mode == m_mode) {
        return;
    }
    if (mode == InteractionMode::Draw && !m_visible) {
        setAnnotationsVisible(true);
    }

    m_mode = mode;
    const bool drawing = mode == InteractionMode::Draw;
    for (const auto& overlay : m_overlays) {
        overlay->setDrawingEnabled(drawing);
    }

    // Give keyboard focus to the overlay under the pointer so Esc / Ctrl+Z / tool keys work.
    if (drawing) {
        focusOverlayUnderCursor();
    }
    emit modeChanged(mode);
}

void OverlayManager::setAnnotationsVisible(bool visible) {
    if (visible == m_visible) {
        return;
    }
    if (!visible && m_mode == InteractionMode::Draw) {
        setMode(InteractionMode::Interact);
    }
    m_visible = visible;
    if (m_shown) {
        for (const auto& overlay : m_overlays) {
            overlay->setVisible(visible);
        }
    }
    emit visibilityChanged(visible);
}

void OverlayManager::setSharedActions(const QList<QAction*>& actions) {
    for (const auto& overlay : m_overlays) {
        for (QAction* action : std::as_const(m_sharedActions)) {
            overlay->removeAction(action);
        }
        overlay->addActions(actions);
    }
    m_sharedActions = actions;
}

void OverlayManager::setDrawModeFrameVisible(bool visible) {
    m_frameVisible = visible;
    for (const auto& overlay : m_overlays) {
        overlay->setFrameVisible(visible);
    }
}

void OverlayManager::setPicking(bool picking) {
    if (picking == m_picking) {
        return;
    }
    if (picking) {
        m_picking = true; // set first: listeners of modeChanged can tell this is for picking
        setMode(InteractionMode::Interact);
        m_visibleBeforePicking = m_visible;
        setAnnotationsVisible(true);
    }
    m_picking = picking;
    for (const auto& overlay : m_overlays) {
        overlay->setPickingEnabled(picking);
    }
    if (picking) {
        focusOverlayUnderCursor(); // keyboard focus for Esc
    } else if (!m_visibleBeforePicking) {
        setAnnotationsVisible(false);
    }
}

void OverlayManager::showWhiteboard(Document& page, const QColor& color, QScreen* screen) {
    m_whiteboard = Whiteboard{&page, color, screen, screen == nullptr};
    for (const auto& overlay : m_overlays) {
        applyPage(*overlay);
    }
}

void OverlayManager::setWhiteboardColor(const QColor& color) {
    if (m_whiteboard) {
        m_whiteboard->color = color;
        for (const auto& overlay : m_overlays) {
            applyPage(*overlay);
        }
    }
}

void OverlayManager::hideWhiteboard() {
    m_whiteboard.reset();
    for (const auto& overlay : m_overlays) {
        applyPage(*overlay);
    }
}

void OverlayManager::focusOverlayUnderCursor() {
    if (OverlayWindow* overlay = overlayUnderCursor()) {
        overlay->takeFocus();
    }
}

QList<const Document*> OverlayManager::visiblePages() const {
    QList<const Document*> pages;
    for (const auto& overlay : m_overlays) {
        const Document* page = &overlay->document();
        if (!pages.contains(page)) {
            pages.append(page);
        }
    }
    return pages;
}

void OverlayManager::applyPage(OverlayWindow& overlay) const {
    const bool onWhiteboard = m_whiteboard && (m_whiteboard->allScreens ||
                                               m_whiteboard->screen == overlay.targetScreen());
    if (onWhiteboard) {
        overlay.setDocument(*m_whiteboard->page);
        overlay.setBackground(m_whiteboard->color);
    } else {
        overlay.setDocument(m_desktopDocument);
        overlay.setBackground(std::nullopt);
    }
}

void OverlayManager::setCaptureHeartbeat(int framesPerSecond) {
    m_heartbeat = framesPerSecond;
    for (const auto& overlay : m_overlays) {
        overlay->setCaptureHeartbeat(framesPerSecond);
    }
}

void OverlayManager::showOverlays() {
    m_shown = true;
    for (const auto& overlay : m_overlays) {
        overlay->setVisible(m_visible);
    }
}

void OverlayManager::addOverlay(QScreen* screen) {
    auto overlay = std::make_unique<OverlayWindow>(screen, m_desktopDocument, m_tools,
                                                   m_pointerHighlight, m_picker, m_replay);
    overlay->addActions(m_sharedActions);
    overlay->setFrameVisible(m_frameVisible);
    overlay->setCaptureHeartbeat(m_heartbeat);
    overlay->setPickingEnabled(m_picking);
    applyPage(*overlay);
    overlay->setDrawingEnabled(m_mode == InteractionMode::Draw);
    connect(overlay.get(), &OverlayWindow::raised, this, &OverlayManager::overlayRaised);
    connect(overlay.get(), &OverlayWindow::escapePressed, this,
            [this] { setMode(InteractionMode::Interact); });
    if (m_shown && m_visible) {
        overlay->show();
    }
    m_overlays.push_back(std::move(overlay));
}

void OverlayManager::removeOverlay(QScreen* screen) {
    std::erase_if(m_overlays, [screen](const std::unique_ptr<OverlayWindow>& overlay) {
        return overlay->targetScreen() == screen;
    });
}

OverlayWindow* OverlayManager::overlayAt(const QPointF& documentPos) const {
    const auto it = std::find_if(m_overlays.cbegin(), m_overlays.cend(), [&](const auto& o) {
        return QRectF(o->targetScreen()->geometry()).contains(documentPos);
    });
    return it != m_overlays.cend() ? it->get() : nullptr;
}

OverlayWindow* OverlayManager::overlayUnderCursor() const {
    const QPoint cursor = QCursor::pos();
    const auto it = std::find_if(m_overlays.cbegin(), m_overlays.cend(), [cursor](const auto& o) {
        return o->targetScreen()->geometry().contains(cursor);
    });
    return it != m_overlays.cend() ? it->get() : nullptr;
}

} // namespace recrayon
