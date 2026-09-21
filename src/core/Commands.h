#pragma once

#include "core/Item.h"

#include <QPointF>
#include <QUndoCommand>

#include <vector>

namespace recrayon {

class Document;

/// Appends one item to the document.
class AddItemCommand final : public QUndoCommand {
public:
    AddItemCommand(Document& document, ItemPtr item);

    void redo() override;
    void undo() override;

private:
    Document& m_document;
    ItemPtr m_item;
    qsizetype m_index = -1;
};

/// Removes a set of items (eraser, clear) and restores them at their original positions on undo.
class RemoveItemsCommand final : public QUndoCommand {
public:
    /// Items not present in @p document are ignored.
    RemoveItemsCommand(Document& document, const std::vector<ItemPtr>& items, const QString& text);

    /// True when none of the requested items was in the document; such a command is pointless.
    [[nodiscard]] bool isEmpty() const noexcept { return m_entries.empty(); }

    void redo() override;
    void undo() override;

private:
    struct Entry {
        qsizetype index;
        ItemPtr item;
    };

    Document& m_document;
    std::vector<Entry> m_entries; // sorted by ascending index
};

/// Moves one item. Created after the drag, when the item already sits at its new position, so
/// the first redo() (executed by QUndoStack::push) does nothing.
class MoveItemCommand final : public QUndoCommand {
public:
    MoveItemCommand(Document& document, ItemPtr item, const QPointF& delta);

    void redo() override;
    void undo() override;

private:
    Document& m_document;
    ItemPtr m_item;
    QPointF m_delta;
    bool m_alreadyApplied = true;
};

} // namespace recrayon
