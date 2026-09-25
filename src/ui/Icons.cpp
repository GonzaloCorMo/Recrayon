#include "ui/Icons.h"

#include "core/Geometry.h"

#include <QConicalGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPolygonF>

#include <algorithm>
#include <functional>

namespace recrayon::icons {

namespace {

constexpr int kCanvas = 48;   // design grid, logical pixels
constexpr qreal kScale = 2.0; // rendered at 2x so icons stay sharp on HiDPI screens
constexpr qreal kStroke = 3.5;

QIcon render(const std::function<void(QPainter&)>& draw) {
    QPixmap pixmap(kCanvas * static_cast<int>(kScale), kCanvas * static_cast<int>(kScale));
    pixmap.setDevicePixelRatio(kScale);
    pixmap.fill(Qt::transparent);
    {
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        draw(painter);
    }
    return QIcon(pixmap);
}

QPen glyphPen(const QColor& color) {
    return QPen(color, kStroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
}

/// Filled arrow head at @p tip, pointing away from @p from.
void drawArrowHead(QPainter& p, const QPointF& from, const QPointF& tip, qreal size,
                   const QColor& color) {
    const geometry::ArrowHead head = geometry::arrowHead(tip, from, size);
    const QBrush brush = p.brush();
    p.setBrush(color);
    p.drawPolygon(QPolygonF{head.left, tip, head.right});
    p.setBrush(brush);
}

void drawPen(QPainter& p) {
    p.drawPolygon(QPolygonF{QPointF(10, 38), QPointF(13, 28), QPointF(31, 10), QPointF(38, 17),
                            QPointF(20, 35)});
    p.drawLine(QPointF(27, 14), QPointF(34, 21));
}

void drawUndoArrow(QPainter& p) {
    QPainterPath path;
    path.moveTo(16, 16);
    path.lineTo(29, 16);
    path.cubicTo(40, 16, 40, 34, 29, 34);
    path.lineTo(14, 34);
    p.drawPath(path);
    p.drawPolyline(QPolygonF{QPointF(22, 10), QPointF(16, 16), QPointF(22, 22)});
}

void drawGlyph(QPainter& p, IconId id, const QColor& color) {
    switch (id) {
    case IconId::App: {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0xE5, 0x39, 0x35));
        p.drawEllipse(QPointF(24, 24), 22, 22);
        p.setPen(glyphPen(Qt::white));
        p.setBrush(Qt::NoBrush);
        drawPen(p);
        break;
    }
    case IconId::Cursor:
        p.drawPolygon(QPolygonF{QPointF(14, 8), QPointF(14, 38), QPointF(21, 31), QPointF(27, 42),
                                QPointF(32, 40), QPointF(26, 29), QPointF(36, 29)});
        break;
    case IconId::Pen:
        drawPen(p);
        break;
    case IconId::Highlighter: {
        p.drawPolygon(
            QPolygonF{QPointF(14, 30), QPointF(30, 14), QPointF(37, 21), QPointF(21, 37)});
        p.drawPolyline(QPolygonF{QPointF(14, 30), QPointF(10, 38), QPointF(21, 37)});
        QColor band = color;
        band.setAlpha(120);
        p.setPen(QPen(band, 5, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(8, 44), QPointF(40, 44));
        break;
    }
    case IconId::FreeArrow: {
        // Hand-drawn stroke that straightens at the end, with a solid head on the very tip.
        QPainterPath curve;
        curve.moveTo(7, 40);
        curve.cubicTo(13, 18, 22, 40, 29, 22);
        p.drawPath(curve);
        p.drawLine(QPointF(29, 22), QPointF(36, 13));
        drawArrowHead(p, QPointF(29, 22), QPointF(38, 10), 13, color);
        break;
    }
    case IconId::Line:
        p.drawLine(QPointF(10, 38), QPointF(38, 10));
        break;
    case IconId::Arrow:
        p.drawLine(QPointF(10, 38), QPointF(36, 12));
        p.drawPolyline(QPolygonF{QPointF(22, 12), QPointF(36, 12), QPointF(36, 26)});
        break;
    case IconId::Rectangle:
        p.drawRoundedRect(QRectF(9, 13, 30, 22), 2, 2);
        break;
    case IconId::Ellipse:
        p.drawEllipse(QRectF(7, 13, 34, 22));
        break;
    case IconId::Eraser:
        p.drawPolygon(QPolygonF{QPointF(8, 30), QPointF(26, 12), QPointF(38, 24), QPointF(22, 40),
                                QPointF(16, 40)});
        p.drawLine(QPointF(17, 21), QPointF(29, 33));
        p.drawLine(QPointF(22, 40), QPointF(40, 40));
        break;
    case IconId::Move: {
        p.drawLine(QPointF(24, 7), QPointF(24, 41));
        p.drawLine(QPointF(7, 24), QPointF(41, 24));
        p.drawPolyline(QPolygonF{QPointF(19, 12), QPointF(24, 7), QPointF(29, 12)});
        p.drawPolyline(QPolygonF{QPointF(19, 36), QPointF(24, 41), QPointF(29, 36)});
        p.drawPolyline(QPolygonF{QPointF(12, 19), QPointF(7, 24), QPointF(12, 29)});
        p.drawPolyline(QPolygonF{QPointF(36, 19), QPointF(41, 24), QPointF(36, 29)});
        break;
    }
    case IconId::Text:
        p.drawLine(QPointF(11, 11), QPointF(37, 11));
        p.drawLine(QPointF(24, 11), QPointF(24, 39));
        p.drawLine(QPointF(18, 39), QPointF(30, 39));
        break;
    case IconId::Callout: {
        // "T" label at the tail and, clearly apart from it, the arrow it labels.
        p.drawLine(QPointF(4, 27), QPointF(16, 27));
        p.drawLine(QPointF(10, 27), QPointF(10, 41));
        p.drawLine(QPointF(23, 40), QPointF(33, 21));
        drawArrowHead(p, QPointF(23, 40), QPointF(38, 12), 13, color);
        break;
    }
    case IconId::Whiteboard:
        p.drawRoundedRect(QRectF(6, 8, 36, 26), 2, 2);
        p.drawLine(QPointF(24, 34), QPointF(18, 42));
        p.drawLine(QPointF(24, 34), QPointF(30, 42));
        p.drawPolyline(
            QPolygonF{QPointF(12, 26), QPointF(19, 18), QPointF(25, 23), QPointF(35, 14)});
        break;
    case IconId::Play:
        p.drawPolygon(QPolygonF{QPointF(16, 10), QPointF(38, 24), QPointF(16, 38)});
        break;
    case IconId::Undo:
        drawUndoArrow(p);
        break;
    case IconId::Redo:
        p.translate(kCanvas, 0);
        p.scale(-1, 1);
        drawUndoArrow(p);
        break;
    case IconId::Clear:
        p.drawLine(QPointF(9, 14), QPointF(39, 14));
        p.drawPolyline(QPolygonF{QPointF(19, 14), QPointF(19, 9), QPointF(29, 9), QPointF(29, 14)});
        p.drawPolygon(
            QPolygonF{QPointF(13, 14), QPointF(16, 40), QPointF(32, 40), QPointF(35, 14)});
        break;
    case IconId::Visibility: {
        QPainterPath eye;
        eye.moveTo(5, 24);
        eye.quadTo(24, 4, 43, 24);
        eye.quadTo(24, 44, 5, 24);
        p.drawPath(eye);
        p.drawEllipse(QPointF(24, 24), 5.5, 5.5);
        break;
    }
    case IconId::CustomColor: {
        QConicalGradient gradient(QPointF(24, 24), 0);
        for (int i = 0; i <= 6; ++i) {
            gradient.setColorAt(i / 6.0, QColor::fromHsv((i * 60) % 360, 220, 255));
        }
        p.setPen(QPen(QColor(255, 255, 255, 140), 2));
        p.setBrush(gradient);
        p.drawEllipse(QPointF(24, 24), 15, 15);
        break;
    }
    case IconId::Screenshot:
        p.drawRoundedRect(QRectF(7, 15, 34, 24), 4, 4);
        p.drawPolyline(
            QPolygonF{QPointF(16, 15), QPointF(19, 10), QPointF(29, 10), QPointF(32, 15)});
        p.drawEllipse(QPointF(24, 27), 6.5, 6.5);
        break;
    case IconId::Record:
        p.drawEllipse(QPointF(24, 24), 16, 16);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0xE5, 0x39, 0x35));
        p.drawEllipse(QPointF(24, 24), 10, 10);
        break;
    case IconId::Stop:
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0xE5, 0x39, 0x35));
        p.drawRoundedRect(QRectF(13, 13, 22, 22), 3, 3);
        break;
    case IconId::Pause:
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawRoundedRect(QRectF(14, 12, 7, 24), 2, 2);
        p.drawRoundedRect(QRectF(27, 12, 7, 24), 2, 2);
        break;
    case IconId::Spotlight: {
        QPainterPath dim;
        dim.setFillRule(Qt::OddEvenFill);
        dim.addRoundedRect(QRectF(5, 9, 38, 30), 3, 3);
        dim.addEllipse(QPointF(24, 24), 9, 9);
        QColor shade = color;
        shade.setAlpha(110);
        p.fillPath(dim, shade);
        p.drawEllipse(QPointF(24, 24), 9, 9);
        break;
    }
    case IconId::Halo: {
        QColor glow(255, 235, 59, 120);
        p.setBrush(glow);
        p.setPen(QPen(QColor(255, 235, 59), 2.5));
        p.drawEllipse(QPointF(21, 21), 14, 14);
        p.setPen(glyphPen(color));
        p.setBrush(Qt::NoBrush);
        p.drawPolygon(QPolygonF{QPointF(21, 21), QPointF(21, 42), QPointF(26, 37), QPointF(30, 44),
                                QPointF(33, 42), QPointF(29, 35), QPointF(36, 35)});
        break;
    }
    case IconId::Settings: {
        // Cogwheel: filled silhouette with square teeth and a hole, instead of a ring with rays
        // (which read as a sun).
        constexpr int kTeeth = 8;
        constexpr qreal kStep = 360.0 / kTeeth;
        constexpr qreal kOuter = 21.0;
        constexpr qreal kRoot = 15.5;
        const QPointF center(24, 24);
        const auto at = [&center](qreal radius, qreal degrees) {
            return QLineF::fromPolar(radius, degrees).translated(center).p2();
        };
        QPainterPath gear;
        for (int i = 0; i < kTeeth; ++i) {
            const qreal angle = i * kStep;
            const QPointF points[]{
                at(kRoot, angle - kStep * 0.34), at(kOuter, angle - kStep * 0.20),
                at(kOuter, angle + kStep * 0.20), at(kRoot, angle + kStep * 0.34)};
            for (const QPointF& point : points) {
                if (gear.elementCount() == 0) {
                    gear.moveTo(point);
                } else {
                    gear.lineTo(point);
                }
            }
        }
        gear.closeSubpath();
        gear.addEllipse(center, 7.0, 7.0); // the hole, left out by the odd-even rule
        gear.setFillRule(Qt::OddEvenFill);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPath(gear);
        p.setBrush(Qt::NoBrush);
        p.setPen(glyphPen(color));
        break;
    }
    case IconId::Collapse:
        // Panel folding into the edge: the bar is the edge, the chevron goes into it.
        p.drawLine(QPointF(12, 11), QPointF(12, 37));
        p.drawPolyline(QPolygonF{QPointF(34, 13), QPointF(22, 24), QPointF(34, 35)});
        break;
    case IconId::Minimize:
        // Down into the bar: clearer than a lone dash for "minimize to the taskbar".
        p.drawLine(QPointF(24, 9), QPointF(24, 26));
        drawArrowHead(p, QPointF(24, 9), QPointF(24, 31), 12, color);
        p.drawLine(QPointF(13, 39), QPointF(35, 39));
        break;
    case IconId::Quit:
        p.drawLine(QPointF(14, 14), QPointF(34, 34));
        p.drawLine(QPointF(34, 14), QPointF(14, 34));
        break;
    }
}

} // namespace

QIcon icon(IconId id, const QColor& color) {
    return render([id, color](QPainter& p) {
        p.setPen(glyphPen(color));
        p.setBrush(Qt::NoBrush);
        drawGlyph(p, id, color);
    });
}

QIcon swatch(const QColor& color) {
    return render([color](QPainter& p) {
        p.setPen(QPen(QColor(255, 255, 255, 140), 2));
        p.setBrush(color);
        p.drawEllipse(QPointF(24, 24), 15, 15);
    });
}

QIcon strokeWidth(qreal width, const QColor& color) {
    const qreal radius = std::clamp(width * 1.2, 3.0, 17.0);
    return render([radius, color](QPainter& p) {
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(QPointF(24, 24), radius, radius);
    });
}

} // namespace recrayon::icons
