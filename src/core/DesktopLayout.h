#pragma once

#include <QList>
#include <QPointF>
#include <QRect>
#include <QRectF>

#include <utility>

namespace recrayon {

/// Where one screen sits in Qt's logical desktop coordinates and in native (device) pixels.
struct ScreenMapping {
    QRect logical; ///< QScreen::geometry()
    QRect native;  ///< same area in device pixels of the virtual desktop
    qreal devicePixelRatio = 1.0;

    friend bool operator==(const ScreenMapping&, const ScreenMapping&) = default;
};

/// Converts between logical desktop coordinates (what Qt widgets and QCursor use) and native
/// pixels (what screen grabs, window bounds and video frames use).
///
/// With mixed DPI the two differ per screen and there is no single scale factor, so every
/// conversion goes through the screen that contains the point (or the nearest one when the
/// point is in a gap between screens). Pure value type: built by the platform layer from the
/// real screens, testable with made-up ones.
class DesktopLayout {
public:
    DesktopLayout() = default;
    explicit DesktopLayout(QList<ScreenMapping> screens) : m_screens(std::move(screens)) {}

    [[nodiscard]] const QList<ScreenMapping>& screens() const noexcept { return m_screens; }
    [[nodiscard]] bool isEmpty() const noexcept { return m_screens.isEmpty(); }

    /// Union of all screens in native pixels.
    [[nodiscard]] QRect nativeBounds() const;

    /// Index of the screen containing @p point, or of the nearest one. -1 if there are none.
    [[nodiscard]] qsizetype indexAtLogical(const QPointF& point) const;
    [[nodiscard]] qsizetype indexAtNative(const QPointF& point) const;

    [[nodiscard]] QPointF toNative(const QPointF& logical) const;
    [[nodiscard]] QPointF toLogical(const QPointF& native) const;

    /// Corners are converted independently, so a rectangle spanning screens maps correctly.
    [[nodiscard]] QRect toNative(const QRectF& logical) const;
    [[nodiscard]] QRectF toLogical(const QRect& native) const;

private:
    QList<ScreenMapping> m_screens;
};

} // namespace recrayon
