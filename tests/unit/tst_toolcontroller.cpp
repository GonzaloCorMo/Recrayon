#include "core/Document.h"
#include "core/FreehandItem.h"
#include "core/Item.h"
#include "tools/ToolController.h"

#include <QSignalSpy>
#include <QTest>
#include <QUndoStack>

using namespace recrayon;

namespace {

void drawStroke(ToolController& tools, const QPointF& from, const QPointF& to) {
    tools.press(from);
    tools.move((from + to) / 2.0);
    tools.release(to);
}

} // namespace

class TestToolController : public QObject {
    Q_OBJECT

private slots:
    void toolIdsRoundTrip() {
        for (const ToolKind kind : kAllTools) {
            QCOMPARE(toolFromId(toolId(kind)), std::optional<ToolKind>(kind));
        }
        QVERIFY(!toolFromId("laser"));
        QVERIFY(!toolFromId(""));
    }

    void penGestureCommitsOneItem() {
        Document document;
        ToolController tools(document);

        drawStroke(tools, {0, 0}, {30, 30});

        QCOMPARE(document.items().size(), std::size_t{1});
        QCOMPARE(document.undoStack()->count(), 1);
        QVERIFY(tools.preview() == nullptr);
        QVERIFY(!tools.isGestureActive());
    }

    void previewExistsOnlyDuringGesture() {
        Document document;
        ToolController tools(document);
        QSignalSpy spy(&tools, &ToolController::previewChanged);

        tools.press({0, 0});
        tools.move({20, 20});

        QVERIFY(tools.preview() != nullptr);
        QVERIFY(document.isEmpty());
        QVERIFY(spy.count() >= 2);
    }

    void itemKeepsStyleFromGestureStart() {
        Document document;
        ToolController tools(document);
        tools.setColor(Qt::blue);

        tools.press({0, 0});
        tools.setColor(Qt::green); // mid-gesture change must not affect this stroke
        tools.release({30, 0});

        QCOMPARE(document.items().front()->style().color, QColor(Qt::blue));
    }

    void tinyShapeIsDiscarded() {
        Document document;
        ToolController tools(document);
        tools.setCurrentTool(ToolKind::Rectangle);

        tools.press({10, 10});
        tools.release({10.5, 10.5});

        QVERIFY(document.isEmpty());
        QCOMPARE(document.undoStack()->count(), 0);
    }

    void switchingToolCancelsGesture() {
        Document document;
        ToolController tools(document);

        tools.press({0, 0});
        tools.move({20, 20});
        tools.setCurrentTool(ToolKind::Line);

        QVERIFY(!tools.isGestureActive());
        QVERIFY(tools.preview() == nullptr);
        QVERIFY(document.isEmpty());
    }

    void eraserRemovesHitItemsAsOneUndoStep() {
        Document document;
        ToolController tools(document);
        drawStroke(tools, {0, 0}, {100, 0});
        drawStroke(tools, {0, 100}, {100, 100});
        QCOMPARE(document.undoStack()->count(), 2);

        tools.setCurrentTool(ToolKind::Eraser);
        tools.press({50, -50});
        tools.move({50, 150}); // crosses both strokes in a single fast move
        tools.release({50, 150});

        QVERIFY(document.isEmpty());
        QCOMPARE(document.undoStack()->count(), 3);

        document.undoStack()->undo();
        QCOMPARE(document.items().size(), std::size_t{2});
    }

    void eraserMissLeavesNoHistory() {
        Document document;
        ToolController tools(document);
        drawStroke(tools, {0, 0}, {100, 0});

        tools.setCurrentTool(ToolKind::Eraser);
        drawStroke(tools, {0, 500}, {100, 500});

        QCOMPARE(document.items().size(), std::size_t{1});
        QCOMPARE(document.undoStack()->count(), 1);
    }

    void freeArrowCommitsStrokeWithHead() {
        Document document;
        ToolController tools(document);
        tools.setCurrentTool(ToolKind::FreeArrow);
        drawStroke(tools, {0, 0}, {100, 40});

        QCOMPARE(document.items().size(), std::size_t{1});
        const auto* stroke = dynamic_cast<const FreehandItem*>(document.items().front().get());
        QVERIFY(stroke != nullptr);
        QVERIFY(stroke->arrowHead().has_value());
    }

    void moveToolDragsTopmostItemAsOneUndoStep() {
        Document document;
        ToolController tools(document);
        drawStroke(tools, {0, 0}, {100, 0});
        drawStroke(tools, {0, 0}, {100, 0}); // on top of the first one
        const ItemPtr bottom = document.items().at(0);
        const ItemPtr top = document.items().at(1);

        tools.setCurrentTool(ToolKind::Move);
        tools.press({50, 0});
        tools.move({50, 30});
        tools.release({50, 60});

        QVERIFY(top->hitTest({50, 60}, 0.0));
        QVERIFY(bottom->hitTest({50, 0}, 0.0)); // untouched
        QCOMPARE(document.undoStack()->count(), 3);

        document.undoStack()->undo();
        QVERIFY(top->hitTest({50, 0}, 0.0));
    }

    void moveToolCancelRestoresPosition() {
        Document document;
        ToolController tools(document);
        drawStroke(tools, {0, 0}, {100, 0});
        const ItemPtr stroke = document.items().front();

        tools.setCurrentTool(ToolKind::Move);
        tools.press({50, 0});
        tools.move({50, 80});
        tools.cancel();

        QVERIFY(stroke->hitTest({50, 0}, 0.0));
        QCOMPARE(document.undoStack()->count(), 1);
    }

    void moveToolOnEmptySpaceDoesNothing() {
        Document document;
        ToolController tools(document);
        drawStroke(tools, {0, 0}, {100, 0});

        tools.setCurrentTool(ToolKind::Move);
        drawStroke(tools, {50, 300}, {80, 400});

        QCOMPARE(document.undoStack()->count(), 1);
    }

    void highlighterIsWiderAndTranslucent() {
        Document document;
        ToolController tools(document);
        tools.setCurrentTool(ToolKind::Highlighter);

        const StrokeStyle style = tools.effectiveStyle();
        QVERIFY(style.color.alpha() < 255);
        QVERIFY(style.width > tools.width());
    }

    void widthIsClamped() {
        Document document;
        ToolController tools(document);
        tools.setWidth(1000.0);
        QCOMPARE(tools.width(), ToolController::kMaxWidth);
        tools.setWidth(0.0);
        QCOMPARE(tools.width(), ToolController::kMinWidth);
    }
};

QTEST_GUILESS_MAIN(TestToolController)
#include "tst_toolcontroller.moc"
