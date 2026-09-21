#pragma once

#include "core/Item.h"

#include <QFont>
#include <QList>
#include <QPointF>
#include <QString>
#include <QStringList>

namespace recrayon {

/// Multi-line text placed next to an anchor point (see geometry::alignedBox()).
///
/// Painted as plain text in the item's color and font, line by line.
class TextItem final : public Item {
public:
    TextItem(StrokeStyle style, QFont font, const QPointF& anchor, Qt::Alignment alignment,
             qreal gap, QString text);

    [[nodiscard]] const QString& text() const noexcept { return m_text; }
    [[nodiscard]] const QFont& font() const noexcept { return m_font; }
    [[nodiscard]] QPointF anchor() const noexcept { return m_anchor; }
    [[nodiscard]] QRectF textRect() const noexcept { return m_textRect; }

    void paint(QPainter& painter) const override;
    [[nodiscard]] QRectF boundingRect() const override;
    [[nodiscard]] bool hitTest(const QPointF& point, qreal tolerance) const override;
    void translate(const QPointF& delta) override;
    /// Typewriter effect: characters appear one after another.
    void paintPartial(QPainter& painter, qreal progress) const override;
    [[nodiscard]] qreal drawLength() const override;

    /// Size the text occupies with @p font (also used to size the editor while typing).
    [[nodiscard]] static QSizeF measure(const QString& text, const QFont& font);

private:
    void layout();
    QFont m_font;
    QPointF m_anchor;
    Qt::Alignment m_alignment;
    qreal m_gap;
    QString m_text;
    QRectF m_textRect;
    QStringList m_lines;
    QList<QPointF> m_baselines; // left end of each line's baseline, document coordinates
};

} // namespace recrayon
