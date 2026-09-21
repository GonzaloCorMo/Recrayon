#include "core/TextItem.h"

#include "core/Geometry.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QStringList>

#include <algorithm>
#include <utility>

namespace recrayon {

namespace {

QStringList linesOf(const QString& text) {
    return text.split(QLatin1Char('\n'));
}

} // namespace

TextItem::TextItem(StrokeStyle style, QFont font, const QPointF& anchor, Qt::Alignment alignment,
                   qreal gap, QString text)
    : Item(std::move(style)), m_font(std::move(font)), m_anchor(anchor), m_alignment(alignment),
      m_gap(gap), m_text(std::move(text)) {
    layout();
}

QSizeF TextItem::measure(const QString& text, const QFont& font) {
    const QFontMetricsF metrics(font);
    qreal width = 0;
    const QStringList lines = linesOf(text);
    for (const QString& line : lines) {
        width = std::max(width, metrics.horizontalAdvance(line));
    }
    return {width, metrics.lineSpacing() * static_cast<qreal>(lines.size())};
}

void TextItem::layout() {
    m_textRect = geometry::alignedBox(m_anchor, measure(m_text, m_font), m_alignment, m_gap);

    const QFontMetricsF metrics(m_font);
    m_lines = linesOf(m_text);
    m_baselines.clear();
    qreal baseline = m_textRect.top() + metrics.ascent();
    for (const QString& line : std::as_const(m_lines)) {
        // Lines follow the box's horizontal alignment so multi-line labels look tidy.
        const qreal lineWidth = metrics.horizontalAdvance(line);
        qreal x = m_textRect.left() + (m_textRect.width() - lineWidth) / 2.0;
        if (m_alignment & Qt::AlignLeft) {
            x = m_textRect.left();
        } else if (m_alignment & Qt::AlignRight) {
            x = m_textRect.right() - lineWidth;
        }
        m_baselines.append(QPointF(x, baseline));
        baseline += metrics.lineSpacing();
    }
}

void TextItem::paint(QPainter& painter) const {
    painter.save();
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setFont(m_font);
    painter.setPen(style().color);
    for (qsizetype i = 0; i < m_lines.size(); ++i) {
        painter.drawText(m_baselines.at(i), m_lines.at(i));
    }
    painter.restore();
}

void TextItem::paintPartial(QPainter& painter, qreal progress) const {
    if (progress >= 1.0) {
        paint(painter);
        return;
    }
    qsizetype remaining = static_cast<qsizetype>(progress * m_text.size());
    if (remaining <= 0) {
        return;
    }
    painter.save();
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setFont(m_font);
    painter.setPen(style().color);
    for (qsizetype i = 0; i < m_lines.size() && remaining > 0; ++i) {
        const QString& line = m_lines.at(i);
        painter.drawText(m_baselines.at(i), line.left(remaining));
        remaining -= line.size() + 1; // + the line break
    }
    painter.restore();
}

qreal TextItem::drawLength() const {
    // "Typing speed": each character counts like this many pixels of stroke.
    constexpr qreal kPixelsPerCharacter = 36.0;
    return static_cast<qreal>(m_text.size()) * kPixelsPerCharacter;
}

QRectF TextItem::boundingRect() const {
    // Room for glyph overhang (italic fonts, antialiasing) beyond the metrics box.
    const qreal margin = std::max(2.0, m_font.pixelSize() / 8.0);
    return m_textRect.adjusted(-margin, -margin, margin, margin);
}

bool TextItem::hitTest(const QPointF& point, qreal tolerance) const {
    return m_textRect.adjusted(-tolerance, -tolerance, tolerance, tolerance).contains(point);
}

void TextItem::translate(const QPointF& delta) {
    m_anchor += delta;
    m_textRect.translate(delta);
    for (QPointF& baseline : m_baselines) {
        baseline += delta;
    }
}

} // namespace recrayon
