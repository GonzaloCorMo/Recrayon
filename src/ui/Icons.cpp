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

/// Filled arrow head at @p tip, pointing away from @p from. Every arrow in the set uses it, so
/// they all end the same way.
void drawArrowHead(QPainter& p, const QPointF& from, const QPointF& tip, qreal size,
                   const QColor& color) {
    const geometry::ArrowHead head = geometry::arrowHead(tip, from, size);
    const QBrush brush = p.brush();
    p.setBrush(color);
    p.drawPolygon(QPolygonF{head.left, tip, head.right});
    p.setBrush(brush);
}

/// Body of a pen pointing down-left, shared by the application icon and the pen tool.
void drawPen(QPainter& p) {
    p.drawPolygon(QPolygonF{QPointF(11, 37), QPointF(14, 28), QPointF(30, 12), QPointF(36, 18),
                            QPointF(20, 34)});
    p.drawLine(QPointF(26, 16), QPointF(32, 22));
}

/// Mouse cursor of the given @p height, with its tip at @p tip.
QPolygonF cursorShape(const QPointF& tip, qreal height) {
    const qreal u = height / 30.0; // the shape is designed 30 units tall
    const auto at = [&tip, u](qreal x, qreal y) {
        return QPointF(tip.x() + x * u, tip.y() + y * u);
    };
    return QPolygonF{tip, at(0, 30), at(7, 23), at(12, 34), at(16, 32), at(11, 21), at(19, 21)};
}

void drawCursor(QPainter& p, const QPointF& tip, qreal height) {
    p.drawPolygon(cursorShape(tip, height));
}

/// Curved arrow of the undo button; redo mirrors it.
void drawUndoArrow(QPainter& p, const QColor& color) {
    QPainterPath path;
    path.moveTo(15, 22);
    path.lineTo(27, 22);
    path.cubicTo(37, 22, 37, 37, 27, 37);
    path.lineTo(19, 37);
    p.drawPath(path);
    drawArrowHead(p, QPointF(23, 22), QPointF(13, 22), 12, color);
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
        drawCursor(p, QPointF(15, 8), 30);
        break;
    case IconId::Pen:
        drawPen(p);
        break;
    case IconId::Highlighter: {
        // Chisel marker at 45 degrees over the translucent band it leaves behind.
        p.drawPolygon(
            QPolygonF{QPointF(17, 27), QPointF(29, 15), QPointF(37, 23), QPointF(25, 35)});
        p.drawPolygon(
            QPolygonF{QPointF(17, 27), QPointF(25, 35), QPointF(13, 38), QPointF(14, 31)});
        QColor band = color;
        band.setAlpha(110);
        p.setPen(QPen(band, 6, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(12, 43), QPointF(36, 43));
        p.setPen(glyphPen(color));
        break;
    }
    case IconId::FreeArrow: {
        // Free stroke that straightens at the end, so the head reads as its tip.
        QPainterPath curve;
        curve.moveTo(9, 39);
        curve.cubicTo(14, 20, 22, 39, 28, 23);
        p.drawPath(curve);
        p.drawLine(QPointF(28, 23), QPointF(34, 15));
        drawArrowHead(p, QPointF(29, 22), QPointF(38, 10), 12, color);
        break;
    }
    case IconId::Line:
        p.drawLine(QPointF(11, 37), QPointF(37, 11));
        break;
    case IconId::Arrow:
        p.drawLine(QPointF(11, 37), QPointF(32, 16));
        drawArrowHead(p, QPointF(20, 28), QPointF(38, 10), 12, color);
        break;
    case IconId::Rectangle:
        p.drawRoundedRect(QRectF(9, 14, 30, 20), 3, 3);
        break;
    case IconId::Ellipse:
        p.drawEllipse(QRectF(8, 14, 32, 20));
        break;
    case IconId::Eraser:
        // Rubber block tilted on the surface it is rubbing out.
        p.save();
        p.translate(25, 24);
        p.rotate(-45);
        p.drawRoundedRect(QRectF(-15, -8, 30, 16), 3, 3);
        p.drawLine(QPointF(3, -8), QPointF(3, 8)); // the softer half of the rubber
        p.restore();
        p.drawLine(QPointF(19, 40), QPointF(39, 40));
        break;
    case IconId::Move:
        // Cross with open chevrons: solid heads at this size turn the middle into a blob.
        p.drawLine(QPointF(24, 9), QPointF(24, 39));
        p.drawLine(QPointF(9, 24), QPointF(39, 24));
        p.drawPolyline(QPolygonF{QPointF(19, 14), QPointF(24, 9), QPointF(29, 14)});
        p.drawPolyline(QPolygonF{QPointF(19, 34), QPointF(24, 39), QPointF(29, 34)});
        p.drawPolyline(QPolygonF{QPointF(14, 19), QPointF(9, 24), QPointF(14, 29)});
        p.drawPolyline(QPolygonF{QPointF(34, 19), QPointF(39, 24), QPointF(34, 29)});
        break;
    case IconId::Text:
        p.drawLine(QPointF(12, 13), QPointF(36, 13));
        p.drawLine(QPointF(24, 13), QPointF(24, 37));
        p.drawLine(QPointF(18, 37), QPointF(30, 37));
        break;
    case IconId::Callout:
        // Label ("T") at the tail of the arrow it names, clearly apart from it.
        p.drawLine(QPointF(7, 24), QPointF(19, 24));
        p.drawLine(QPointF(13, 24), QPointF(13, 38));
        p.drawLine(QPointF(25, 38), QPointF(33, 24));
        drawArrowHead(p, QPointF(30, 29), QPointF(38, 15), 11, color);
        break;
    case IconId::Whiteboard:
        // Board on a stand, with a stroke drawn on it.
        p.drawRoundedRect(QRectF(8, 9, 32, 23), 3, 3);
        p.drawLine(QPointF(24, 32), QPointF(24, 37));
        p.drawLine(QPointF(24, 37), QPointF(18, 42));
        p.drawLine(QPointF(24, 37), QPointF(30, 42));
        p.drawPolyline(
            QPolygonF{QPointF(14, 25), QPointF(20, 17), QPointF(26, 23), QPointF(34, 15)});
        break;
    case IconId::Play: {
        QPainterPath triangle;
        triangle.moveTo(17, 11);
        triangle.lineTo(38, 24);
        triangle.lineTo(17, 37);
        triangle.closeSubpath();
        p.setPen(QPen(color, kStroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(color);
        p.drawPath(triangle);
        p.setBrush(Qt::NoBrush);
        break;
    }
    case IconId::Undo:
        drawUndoArrow(p, color);
        break;
    case IconId::Redo:
        p.translate(kCanvas, 0);
        p.scale(-1, 1);
        drawUndoArrow(p, color);
        break;
    case IconId::Clear:
        // Waste bin: lid, handle, body and the two marks a bin usually has.
        p.drawLine(QPointF(9, 15), QPointF(39, 15));
        p.drawPolyline(QPolygonF{QPointF(19, 15), QPointF(19, 9), QPointF(29, 9), QPointF(29, 15)});
        p.drawPolyline(
            QPolygonF{QPointF(13, 15), QPointF(16, 40), QPointF(32, 40), QPointF(35, 15)});
        p.drawLine(QPointF(21, 21), QPointF(21, 34));
        p.drawLine(QPointF(27, 21), QPointF(27, 34));
        break;
    case IconId::Visibility: {
        QPainterPath eye;
        eye.moveTo(7, 24);
        eye.quadTo(24, 7, 41, 24);
        eye.quadTo(24, 41, 7, 24);
        p.drawPath(eye);
        p.setBrush(color);
        p.drawEllipse(QPointF(24, 24), 4.5, 4.5);
        p.setBrush(Qt::NoBrush);
        break;
    }
    case IconId::CustomColor: {
        QConicalGradient gradient(QPointF(24, 24), 90);
        for (int i = 0; i <= 6; ++i) {
            gradient.setColorAt(i / 6.0, QColor::fromHsv((i * 60) % 360, 215, 250));
        }
        QPainterPath wheel;
        wheel.setFillRule(Qt::OddEvenFill);
        wheel.addEllipse(QPointF(24, 24), 15, 15);
        wheel.addEllipse(QPointF(24, 24), 5.5, 5.5); // see-through hole
        p.setPen(Qt::NoPen);
        p.fillPath(wheel, gradient);
        p.setPen(glyphPen(color));
        break;
    }
    case IconId::Screenshot:
        // Camera: body, viewfinder bump, lens and flash.
        p.drawRoundedRect(QRectF(8, 16, 32, 22), 4, 4);
        p.drawPolyline(
            QPolygonF{QPointF(17, 16), QPointF(20, 11), QPointF(28, 11), QPointF(31, 16)});
        p.drawEllipse(QPointF(24, 28), 6.5, 6.5);
        p.setBrush(color);
        p.drawEllipse(QPointF(34, 21), 1.4, 1.4);
        p.setBrush(Qt::NoBrush);
        break;
    case IconId::Record:
        p.drawEllipse(QPointF(24, 24), 15, 15);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0xE5, 0x39, 0x35));
        p.drawEllipse(QPointF(24, 24), 8, 8);
        p.setBrush(Qt::NoBrush);
        p.setPen(glyphPen(color));
        break;
    case IconId::Stop:
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0xE5, 0x39, 0x35));
        p.drawRoundedRect(QRectF(15, 15, 18, 18), 3, 3);
        p.setBrush(Qt::NoBrush);
        p.setPen(glyphPen(color));
        break;
    case IconId::Pause:
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawRoundedRect(QRectF(16, 13, 5.5, 22), 2.5, 2.5);
        p.drawRoundedRect(QRectF(26.5, 13, 5.5, 22), 2.5, 2.5);
        p.setBrush(Qt::NoBrush);
        p.setPen(glyphPen(color));
        break;
    case IconId::Spotlight: {
        // Everything dimmed except a circle of light: the shape people already recognised.
        QPainterPath dim;
        dim.setFillRule(Qt::OddEvenFill);
        dim.addRoundedRect(QRectF(7, 11, 34, 26), 4, 4);
        dim.addEllipse(QPointF(24, 24), 9.5, 9.5);
        QColor shade = color;
        shade.setAlpha(110);
        p.fillPath(dim, shade);
        p.drawEllipse(QPointF(24, 24), 9.5, 9.5);
        break;
    }
    case IconId::Halo:
        // Just the ring of light: a cursor inside only added noise at 22 px.
        p.setPen(QPen(QColor(0xFD, 0xD8, 0x35), 4.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawEllipse(QPointF(24, 24), 13, 13);
        p.setPen(glyphPen(color));
        break;
    case IconId::Settings: {
        // Cogwheel: teeth around a ring with a hole, filled so it reads at small sizes.
        constexpr int kTeeth = 8;
        constexpr qreal kStep = 360.0 / kTeeth;
        const QPointF center(24, 24);
        const auto at = [&center](qreal radius, qreal degrees) {
            return QLineF::fromPolar(radius, degrees).translated(center).p2();
        };
        QPainterPath gear;
        for (int i = 0; i < kTeeth; ++i) {
            const qreal angle = i * kStep;
            const QPointF points[]{at(15.0, angle - kStep * 0.30), at(20.0, angle - kStep * 0.17),
                                   at(20.0, angle + kStep * 0.17), at(15.0, angle + kStep * 0.30)};
            for (const QPointF& point : points) {
                if (gear.elementCount() == 0) {
                    gear.moveTo(point);
                } else {
                    gear.lineTo(point);
                }
            }
        }
        gear.closeSubpath();
        gear.addEllipse(center, 6.5, 6.5); // the hole, left out by the odd-even rule
        gear.setFillRule(Qt::OddEvenFill);
        p.setPen(QPen(color, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(color);
        p.drawPath(gear);
        p.setBrush(Qt::NoBrush);
        p.setPen(glyphPen(color));
        break;
    }
    case IconId::Collapse:
        // Panel folding into the edge: the bar is the edge, the chevron goes into it.
        p.drawLine(QPointF(14, 12), QPointF(14, 36));
        p.drawPolyline(QPolygonF{QPointF(34, 15), QPointF(24, 24), QPointF(34, 33)});
        break;
    case IconId::Minimize:
        // Down into the bar: clearer than a lone dash for "minimize to the taskbar".
        p.drawLine(QPointF(24, 10), QPointF(24, 26));
        drawArrowHead(p, QPointF(24, 18), QPointF(24, 32), 12, color);
        p.drawLine(QPointF(14, 39), QPointF(34, 39));
        break;
    case IconId::Quit:
        p.drawLine(QPointF(15, 15), QPointF(33, 33));
        p.drawLine(QPointF(33, 15), QPointF(15, 33));
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
