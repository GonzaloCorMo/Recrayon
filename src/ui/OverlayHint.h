#pragma once

#include <QPointF>
#include <QRectF>
#include <QString>

class QPainter;

namespace recrayon::hints {

/// Dark rounded label with @p text whose top-left corner is at @p topLeft, pushed back inside
/// @p area when it would overflow. Same look everywhere: region picker, draw mode, whiteboard,
/// text editing.
void paintLabel(QPainter& painter, const QPointF& topLeft, const QString& text, const QRectF& area);

/// Label centered horizontally near the top of @p area, for "how do I get out of here" hints.
void paintBanner(QPainter& painter, const QRectF& area, const QString& text);

/// Height of a label drawn with the painter's current font.
[[nodiscard]] qreal labelHeight(const QPainter& painter);

} // namespace recrayon::hints
