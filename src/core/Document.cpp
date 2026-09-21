#include "core/Document.h"

#include "core/Commands.h"

#include <algorithm>
#include <iterator>

namespace recrayon {

Document::Document(QObject* parent) : QObject(parent) {}

qsizetype Document::indexOf(const Item* item) const {
    const auto it =
        std::find_if(m_items.cbegin(), m_items.cend(),
                     [item](const ItemPtr& candidate) { return candidate.get() == item; });
    return it == m_items.cend() ? -1 : static_cast<qsizetype>(std::distance(m_items.cbegin(), it));
}

void Document::addItem(ItemPtr item) {
    Q_ASSERT(item);
    m_undoStack.push(new AddItemCommand(*this, std::move(item)));
}

void Document::removeItems(const std::vector<ItemPtr>& items) {
    auto command = std::make_unique<RemoveItemsCommand>(*this, items, tr("Erase"));
    if (!command->isEmpty()) {
        m_undoStack.push(command.release());
    }
}

void Document::clear() {
    if (m_items.empty()) {
        return;
    }
    m_undoStack.push(new RemoveItemsCommand(*this, m_items, tr("Clear")));
}

void Document::recordMove(const ItemPtr& item, const QPointF& totalDelta) {
    Q_ASSERT(item);
    if (totalDelta.isNull() || indexOf(item.get()) < 0) {
        return;
    }
    m_undoStack.push(new MoveItemCommand(*this, item, totalDelta));
}

void Document::insertItem(qsizetype index, ItemPtr item) {
    Q_ASSERT(item);
    Q_ASSERT(index >= 0 && index <= static_cast<qsizetype>(m_items.size()));
    const QRectF dirty = item->boundingRect();
    m_items.insert(m_items.begin() + index, std::move(item));
    emit changed(dirty);
}

ItemPtr Document::takeItem(qsizetype index) {
    Q_ASSERT(index >= 0 && index < static_cast<qsizetype>(m_items.size()));
    ItemPtr item = std::move(m_items[static_cast<std::size_t>(index)]);
    m_items.erase(m_items.begin() + index);
    emit changed(item->boundingRect());
    return item;
}

void Document::translateItem(Item& item, const QPointF& delta) {
    const QRectF before = item.boundingRect();
    item.translate(delta);
    emit changed(before.united(item.boundingRect()));
}

} // namespace recrayon
