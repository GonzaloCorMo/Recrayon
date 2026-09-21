#include "core/Commands.h"

#include "core/Document.h"

#include <QCoreApplication>

#include <algorithm>

namespace recrayon {

AddItemCommand::AddItemCommand(Document& document, ItemPtr item)
    : m_document(document), m_item(std::move(item)) {
    setText(QCoreApplication::translate("recrayon::Document", "Draw"));
}

void AddItemCommand::redo() {
    // The undo stack guarantees the document is in the same state as when the command was first
    // executed, so appending always lands at the same index.
    m_index = static_cast<qsizetype>(m_document.items().size());
    m_document.insertItem(m_index, m_item);
}

void AddItemCommand::undo() {
    m_document.takeItem(m_index);
}

RemoveItemsCommand::RemoveItemsCommand(Document& document, const std::vector<ItemPtr>& items,
                                       const QString& text)
    : m_document(document) {
    setText(text);
    m_entries.reserve(items.size());
    for (const ItemPtr& item : items) {
        const qsizetype index = document.indexOf(item.get());
        if (index >= 0) {
            m_entries.push_back({index, item});
        }
    }
    std::sort(m_entries.begin(), m_entries.end(),
              [](const Entry& a, const Entry& b) { return a.index < b.index; });
    m_entries.erase(std::unique(m_entries.begin(), m_entries.end(),
                                [](const Entry& a, const Entry& b) { return a.index == b.index; }),
                    m_entries.end());
}

void RemoveItemsCommand::redo() {
    // Highest index first so the remaining indices stay valid.
    for (auto it = m_entries.rbegin(); it != m_entries.rend(); ++it) {
        m_document.takeItem(it->index);
    }
}

void RemoveItemsCommand::undo() {
    // Lowest index first: each insertion restores the exact original ordering.
    for (const Entry& entry : m_entries) {
        m_document.insertItem(entry.index, entry.item);
    }
}

MoveItemCommand::MoveItemCommand(Document& document, ItemPtr item, const QPointF& delta)
    : m_document(document), m_item(std::move(item)), m_delta(delta) {
    setText(QCoreApplication::translate("recrayon::Document", "Move"));
}

void MoveItemCommand::redo() {
    if (m_alreadyApplied) {
        m_alreadyApplied = false;
        return;
    }
    m_document.translateItem(*m_item, m_delta);
}

void MoveItemCommand::undo() {
    m_document.translateItem(*m_item, -m_delta);
}

} // namespace recrayon
