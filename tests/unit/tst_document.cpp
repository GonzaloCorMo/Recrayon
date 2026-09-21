#include "core/Document.h"
#include "core/FreehandItem.h"

#include <QSignalSpy>
#include <QTest>
#include <QUndoStack>

using namespace recrayon;

namespace {

ItemPtr makeStroke(const QPointF& from, const QPointF& to) {
    auto item = std::make_shared<FreehandItem>(StrokeStyle{});
    item->addPoint(from);
    item->addPoint(to);
    return item;
}

} // namespace

class TestDocument : public QObject {
    Q_OBJECT

private slots:
    void addItemAppendsAndEmitsChanged() {
        Document document;
        QSignalSpy spy(&document, &Document::changed);

        const ItemPtr stroke = makeStroke({0, 0}, {10, 10});
        document.addItem(stroke);

        QCOMPARE(document.items().size(), std::size_t{1});
        QCOMPARE(document.items().front(), stroke);
        QCOMPARE(spy.count(), 1);
        QVERIFY(spy.first().first().toRectF().contains(QPointF(5, 5)));
    }

    void undoAndRedoAdd() {
        Document document;
        document.addItem(makeStroke({0, 0}, {10, 10}));

        document.undoStack()->undo();
        QVERIFY(document.isEmpty());

        document.undoStack()->redo();
        QCOMPARE(document.items().size(), std::size_t{1});
    }

    void clearIsOneUndoableStep() {
        Document document;
        const ItemPtr a = makeStroke({0, 0}, {10, 0});
        const ItemPtr b = makeStroke({0, 10}, {10, 10});
        const ItemPtr c = makeStroke({0, 20}, {10, 20});
        document.addItem(a);
        document.addItem(b);
        document.addItem(c);

        document.clear();
        QVERIFY(document.isEmpty());
        QCOMPARE(document.undoStack()->count(), 4);

        document.undoStack()->undo();
        QCOMPARE(document.items(), (std::vector<ItemPtr>{a, b, c}));
    }

    void clearOnEmptyDocumentPushesNothing() {
        Document document;
        document.clear();
        QCOMPARE(document.undoStack()->count(), 0);
    }

    void removeNonContiguousItemsRestoresOrderOnUndo() {
        Document document;
        const ItemPtr a = makeStroke({0, 0}, {10, 0});
        const ItemPtr b = makeStroke({0, 10}, {10, 10});
        const ItemPtr c = makeStroke({0, 20}, {10, 20});
        const ItemPtr d = makeStroke({0, 30}, {10, 30});
        for (const ItemPtr& item : {a, b, c, d}) {
            document.addItem(item);
        }

        document.removeItems({d, b});
        QCOMPARE(document.items(), (std::vector<ItemPtr>{a, c}));

        document.undoStack()->undo();
        QCOMPARE(document.items(), (std::vector<ItemPtr>{a, b, c, d}));

        document.undoStack()->redo();
        QCOMPARE(document.items(), (std::vector<ItemPtr>{a, c}));
    }

    void removeUnknownItemPushesNothing() {
        Document document;
        document.addItem(makeStroke({0, 0}, {10, 0}));
        const int before = document.undoStack()->count();

        document.removeItems({makeStroke({50, 50}, {60, 60})});

        QCOMPARE(document.undoStack()->count(), before);
        QCOMPARE(document.items().size(), std::size_t{1});
    }

    void recordedMoveIsUndoable() {
        Document document;
        const ItemPtr stroke = makeStroke({0, 0}, {10, 0});
        document.addItem(stroke);

        QSignalSpy spy(&document, &Document::changed);
        document.translateItem(*stroke, {5, 5});
        QCOMPARE(spy.count(), 1);
        document.recordMove(stroke, {5, 5});
        QCOMPARE(document.undoStack()->count(), 2);
        QVERIFY(stroke->hitTest({5, 5}, 0.0)); // push did not move it a second time

        document.undoStack()->undo();
        QVERIFY(stroke->hitTest({0, 0}, 0.0));
        document.undoStack()->redo();
        QVERIFY(stroke->hitTest({5, 5}, 0.0));
    }

    void zeroMoveIsNotRecorded() {
        Document document;
        const ItemPtr stroke = makeStroke({0, 0}, {10, 0});
        document.addItem(stroke);
        document.recordMove(stroke, {0, 0});
        QCOMPARE(document.undoStack()->count(), 1);
    }

    void indexOfReportsPositionOrMinusOne() {
        Document document;
        const ItemPtr a = makeStroke({0, 0}, {10, 0});
        const ItemPtr b = makeStroke({0, 10}, {10, 10});
        document.addItem(a);
        document.addItem(b);

        QCOMPARE(document.indexOf(a.get()), qsizetype{0});
        QCOMPARE(document.indexOf(b.get()), qsizetype{1});
        QCOMPARE(document.indexOf(nullptr), qsizetype{-1});
    }
};

QTEST_GUILESS_MAIN(TestDocument)
#include "tst_document.moc"
