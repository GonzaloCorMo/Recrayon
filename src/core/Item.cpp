#include "core/Item.h"

#include <QLineF>
#include <QPainter>

namespace recrayon {

void Item::paintPartial(QPainter& painter, qreal progress) const {
    if (progress <= 0.0) {
        return;
    }
    if (progress >= 1.0) {
        paint(painter);
        return;
    }
    painter.save();
    painter.setOpacity(painter.opacity() * progress);
    paint(painter);
    painter.restore();
}

qreal Item::drawLength() const {
    const QRectF bounds = boundingRect();
    return QLineF(bounds.topLeft(), bounds.bottomRight()).length();
}

} // namespace recrayon
