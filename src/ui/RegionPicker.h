#pragma once

#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QString>

#include <functional>
#include <optional>

class QPainter;

namespace recrayon {

/// Interactive choice of a capture area, drawn on the overlays:
///  - drag to select a rectangle (it may span several screens),
///  - click a window to select it (highlighted while hovering),
///  - click the desktop to select the whole screen,
///  - Esc cancels.
///
/// Works in logical desktop coordinates. Window lookup and native pixel sizes come from the
/// platform layer through injected functions, so this class stays platform independent.
class RegionPicker final : public QObject {
    Q_OBJECT

public:
    struct Selection {
        QRectF logicalRect;
        quintptr windowId = 0; ///< non-zero when a window was clicked
    };

    struct WindowHit {
        quintptr id = 0;
        QRectF logicalRect;
        QString title;
    };

    using WindowLookup = std::function<std::optional<WindowHit>(const QPointF& logicalPos)>;
    using NativeSize = std::function<QSize(const QRectF& logicalRect)>;

    explicit RegionPicker(QObject* parent = nullptr);

    void setWindowLookup(WindowLookup lookup);
    void setNativeSize(NativeSize nativeSize);

    [[nodiscard]] bool isActive() const noexcept { return m_active; }
    void start();
    void cancel();

    // ---- Pointer input from the overlays, logical desktop coordinates ----
    void press(const QPointF& pos);
    void move(const QPointF& pos);
    void release(const QPointF& pos);

    /// Paints the dimmed desktop, the selection and the hints. @p area is the part of the
    /// desktop covered by the calling overlay; the painter uses desktop coordinates.
    void paint(QPainter& painter, const QRectF& area) const;

signals:
    void changed();
    void finished(const recrayon::RegionPicker::Selection& selection);
    void canceled();

private:
    [[nodiscard]] bool isDragging() const;
    [[nodiscard]] QRectF highlightedRect() const;
    void updateHover(const QPointF& pos);

    WindowLookup m_windowLookup;
    NativeSize m_nativeSize;
    std::optional<WindowHit> m_hover;
    QPointF m_pressPos;
    QPointF m_currentPos;
    bool m_active = false;
    bool m_pressed = false;
};

} // namespace recrayon
