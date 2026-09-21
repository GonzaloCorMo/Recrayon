#include "ui/OverlayHint.h"

#include <QFontMetricsF>
#include <QPainter>

#include <algorithm>

namespace recrayon::hints {

namespace {
const QColor kBackground(32, 33, 36, 230);
const QColor kText(0xE8, 0xEA, 0xED);
constexpr qreal kPadding = 6.0;
constexpr qreal kRadius = 4.0;
constexpr qreal kBannerTop = 24.0;

QSizeF labelSize(const QPainter& painter, const QString& text) {
    const QFontMetricsF metrics(painter.font());
    return metrics.size(Qt::TextSingleLine, text) + QSizeF(2 * kPadding, 2 * kPadding);
}
} // namespace

qreal labelHeight(const QPainter& painter) {
    return QFontMetricsF(painter.font()).height() + 2 * kPadding;
}

void paintLabel(QPainter& painter, const QPointF& topLeft, const QString& text,
                const QRectF& area) {
    QRectF box(topLeft, labelSize(painter, text));
    box.moveLeft(std::max(area.left(), std::min(box.left(), area.right() - box.width())));
    box.moveTop(std::max(area.top(), std::min(box.top(), area.bottom() - box.height())));
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(kBackground);
    painter.drawRoundedRect(box, kRadius, kRadius);
    painter.setPen(kText);
    painter.drawText(box, Qt::AlignCenter, text);
    painter.restore();
}

void paintBanner(QPainter& painter, const QRectF& area, const QString& text) {
    const qreal width = labelSize(painter, text).width();
    paintLabel(painter, QPointF(area.center().x() - width / 2.0, area.top() + kBannerTop), text,
               area);
}

} // namespace recrayon::hints
