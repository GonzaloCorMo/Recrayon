#include "ui/TextEditBox.h"

#include "core/Geometry.h"
#include "core/TextItem.h"

#include <QFontMetricsF>
#include <QKeyEvent>

#include <algorithm>

namespace recrayon {

namespace {
constexpr int kPadding = 4;
} // namespace

TextEditBox::TextEditBox(const TextRequest& request, const QPointF& localAnchor, const QFont& font,
                         const QColor& color, QWidget* parent)
    : QPlainTextEdit(parent), m_request(request), m_localAnchor(localAnchor) {
    setFrameStyle(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    document()->setDocumentMargin(0);
    // A stylesheet makes the widget ignore setFont(): the font has to be in the stylesheet.
    setStyleSheet(QStringLiteral("QPlainTextEdit { background: rgba(0, 0, 0, 40); color: %1; "
                                 "border: 1px dashed #8AB4F8; padding: %2px; "
                                 "font-family: \"%3\"; font-size: %4px; }")
                      .arg(color.name())
                      .arg(kPadding)
                      .arg(font.family())
                      .arg(font.pixelSize()));
    setFont(font);
    connect(this, &QPlainTextEdit::textChanged, this, &TextEditBox::updatePlacement);
    updatePlacement();
    show();
    setFocus(Qt::OtherFocusReason);
}

void TextEditBox::updatePlacement() {
    // Measure like the final TextItem does, so the text does not jump when committed.
    const QString text = toPlainText();
    const QFontMetricsF metrics(font());
    QSizeF content = TextItem::measure(text.isEmpty() ? QStringLiteral(" ") : text, font());
    content.setWidth(
        std::max(content.width() + metrics.averageCharWidth(), metrics.averageCharWidth() * 4));
    const QSizeF outer = content + QSizeF(2 * (kPadding + 1), 2 * (kPadding + 1));
    // The editor's padding would shift the text; compensate so glyphs land on the final spot.
    const QRectF textBox =
        geometry::alignedBox(m_localAnchor, content, m_request.alignment, m_request.gap);
    setGeometry(textBox.adjusted(-(kPadding + 1), -(kPadding + 1), kPadding + 1, kPadding + 1)
                    .toAlignedRect());
    resize(outer.toSize());
}

void TextEditBox::commit() {
    if (m_finished) {
        return;
    }
    m_finished = true;
    emit committed(toPlainText());
}

void TextEditBox::keyPressEvent(QKeyEvent* event) {
    const bool enter = event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter;
    if (enter && !(event->modifiers() & Qt::ShiftModifier)) {
        commit();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        if (!m_finished) {
            m_finished = true;
            emit canceled();
        }
        return;
    }
    QPlainTextEdit::keyPressEvent(event);
}

void TextEditBox::focusOutEvent(QFocusEvent* event) {
    QPlainTextEdit::focusOutEvent(event);
    commit();
}

} // namespace recrayon
