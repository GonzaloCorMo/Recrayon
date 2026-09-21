#pragma once

#include "core/Item.h"

#include <QObject>
#include <QRectF>
#include <QUndoStack>

#include <vector>

namespace recrayon {

/// Ordered collection of drawn items (back to front) plus its undo history.
///
/// Use addItem(), removeItems(), clear() and recordMove() for user actions: they go through the
/// undo stack. insertItem(), takeItem() and translateItem() are raw mutators for undo commands
/// and for live previews (e.g. dragging an item before the move is recorded).
class Document final : public QObject {
    Q_OBJECT

public:
    explicit Document(QObject* parent = nullptr);

    [[nodiscard]] const std::vector<ItemPtr>& items() const noexcept { return m_items; }
    [[nodiscard]] bool isEmpty() const noexcept { return m_items.empty(); }

    /// Index of @p item, or -1 when it is not part of the document.
    [[nodiscard]] qsizetype indexOf(const Item* item) const;

    [[nodiscard]] QUndoStack* undoStack() noexcept { return &m_undoStack; }

    // ---- Undoable operations ----
    void addItem(ItemPtr item);
    void removeItems(const std::vector<ItemPtr>& items);
    void clear();
    /// Adds an undo step for a move that was already applied live with translateItem().
    void recordMove(const ItemPtr& item, const QPointF& totalDelta);

    // ---- Raw mutators (undo commands and live previews only) ----
    void insertItem(qsizetype index, ItemPtr item);
    ItemPtr takeItem(qsizetype index);
    void translateItem(Item& item, const QPointF& delta);

signals:
    /// Emitted after any change. @p dirtyRect is the affected area in document coordinates.
    void changed(const QRectF& dirtyRect);

private:
    std::vector<ItemPtr> m_items;
    QUndoStack m_undoStack;
};

} // namespace recrayon
