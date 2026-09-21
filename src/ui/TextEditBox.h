#pragma once

#include "tools/Tool.h"

#include <QColor>
#include <QFont>
#include <QPlainTextEdit>

namespace recrayon {

/// In-place editor for the text and callout tools, shown on an overlay where the text will go.
///
/// Enter commits, Shift+Enter inserts a line break, Esc cancels, and clicking elsewhere
/// (losing focus) commits. It grows with its content, keeping the side described by the
/// TextRequest attached to the anchor, so what you type appears exactly where it will stay.
class TextEditBox final : public QPlainTextEdit {
    Q_OBJECT

public:
    /// @p localAnchor is the request anchor in the parent widget's coordinates.
    TextEditBox(const TextRequest& request, const QPointF& localAnchor, const QFont& font,
                const QColor& color, QWidget* parent);

    /// Commits the current text (no-op if already finished).
    void commit();

signals:
    void committed(const QString& text);
    void canceled();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    void updatePlacement();

    TextRequest m_request;
    QPointF m_localAnchor;
    bool m_finished = false;
};

} // namespace recrayon
