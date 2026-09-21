#include "core/CalloutItem.h"
#include "core/Document.h"
#include "core/Geometry.h"
#include "core/ShapeItem.h"
#include "core/TextItem.h"
#include "tools/ToolController.h"

#include <QSignalSpy>
#include <QTest>
#include <QUndoStack>

using namespace recrayon;

namespace {

QFont testFont() {
    QFont font;
    font.setPixelSize(20);
    return font;
}

} // namespace

/// Text items, callouts and the two-phase text tools. Needs a QGuiApplication (fonts), which
/// the offscreen platform provides in CI.
class TestText : public QObject {
    Q_OBJECT

private slots:
    void alignedBoxPlacesEachSide() {
        const QPointF anchor(100, 100);
        const QSizeF size(40, 20);
        QCOMPARE(geometry::alignedBox(anchor, size, Qt::AlignLeft | Qt::AlignTop),
                 QRectF(100, 100, 40, 20));
        QCOMPARE(geometry::alignedBox(anchor, size, Qt::AlignRight | Qt::AlignVCenter, 5),
                 QRectF(55, 90, 40, 20));
        QCOMPARE(geometry::alignedBox(anchor, size, Qt::AlignBottom | Qt::AlignHCenter, 5),
                 QRectF(80, 75, 40, 20));
    }

    void calloutLabelGoesAwayFromTheTip() {
        // Arrow pointing right: label on the left of the tail (its right edge faces the tail).
        QCOMPARE(geometry::calloutLabelAlignment({100, 100}, {300, 110}),
                 Qt::AlignRight | Qt::AlignVCenter);
        QCOMPARE(geometry::calloutLabelAlignment({300, 100}, {100, 90}),
                 Qt::AlignLeft | Qt::AlignVCenter);
        QCOMPARE(geometry::calloutLabelAlignment({100, 100}, {110, 300}),
                 Qt::AlignBottom | Qt::AlignHCenter);
    }

    void textUsesTheChosenFont() {
        QFont mono(QStringLiteral("Courier New"));
        mono.setPixelSize(20);
        TextItem item(StrokeStyle{}, mono, {0, 0}, Qt::AlignLeft | Qt::AlignTop, 0,
                      QStringLiteral("iiii"));
        QCOMPARE(item.font().family(), mono.family());
        // Monospace: "iiii" is as wide as "WWWW" in the item's font.
        const QSizeF narrow = TextItem::measure(QStringLiteral("iiii"), item.font());
        const QSizeF wide = TextItem::measure(QStringLiteral("WWWW"), item.font());
        QCOMPARE(narrow.width(), wide.width());
    }

    void textItemGeometry() {
        TextItem item(StrokeStyle{Qt::red, 4}, testFont(), {10, 10}, Qt::AlignLeft | Qt::AlignTop,
                      0, QStringLiteral("Hello\nworld"));
        const QRectF rect = item.textRect();
        QCOMPARE(rect.topLeft(), QPointF(10, 10));
        QVERIFY(rect.height() > 30); // two lines of a 20 px font
        QVERIFY(item.boundingRect().contains(rect));
        QVERIFY(item.hitTest(rect.center(), 0));
        QVERIFY(!item.hitTest(rect.bottomRight() + QPointF(50, 50), 0));

        item.translate({5, 7});
        QCOMPARE(item.textRect().topLeft(), QPointF(15, 17));
    }

    void calloutCombinesArrowAndLabel() {
        const CalloutItem callout(StrokeStyle{Qt::red, 4}, testFont(), {100, 100}, {300, 100},
                                  QStringLiteral("Look here"));
        QVERIFY(callout.label().textRect().right() <= 100); // left of the tail
        QVERIFY(callout.hitTest({200, 100}, 2));            // on the arrow
        QVERIFY(callout.hitTest(callout.label().textRect().center(), 0));
        QVERIFY(callout.boundingRect().contains(callout.label().textRect()));
    }

    void textToolAsksForTextAndCommits() {
        Document document;
        ToolController tools(document);
        tools.setTextFont(testFont());
        tools.setCurrentTool(ToolKind::Text);
        QSignalSpy requested(&tools, &ToolController::textRequested);
        QSignalSpy closed(&tools, &ToolController::textInputClosed);

        tools.press({50, 60});
        tools.release({50, 60});
        QCOMPARE(requested.count(), 1);
        QVERIFY(tools.isAwaitingText());
        QVERIFY(document.isEmpty());

        tools.commitText(QStringLiteral("Note"));
        QCOMPARE(closed.count(), 1);
        QCOMPARE(document.items().size(), std::size_t{1});
        const auto* text = dynamic_cast<const TextItem*>(document.items().front().get());
        QVERIFY(text);
        QCOMPARE(text->text(), QStringLiteral("Note"));
        QCOMPARE(text->textRect().topLeft(), QPointF(50, 60));
    }

    void emptyTextAddsNothing() {
        Document document;
        ToolController tools(document);
        tools.setCurrentTool(ToolKind::Text);
        tools.press({0, 0});
        tools.release({0, 0});
        tools.commitText(QStringLiteral("   "));
        QVERIFY(document.isEmpty());
        QVERIFY(!tools.isAwaitingText());
    }

    void calloutToolCreatesCallout() {
        Document document;
        ToolController tools(document);
        tools.setCurrentTool(ToolKind::Callout);
        QSignalSpy requested(&tools, &ToolController::textRequested);

        tools.press({100, 100});
        tools.move({200, 100});
        tools.release({300, 100});
        QCOMPARE(requested.count(), 1);
        const auto request = requested.first().first().value<TextRequest>();
        QCOMPARE(request.anchor, QPointF(100, 100));
        QVERIFY(tools.preview() != nullptr); // the arrow stays visible while typing

        tools.commitText(QStringLiteral("Important"));
        QCOMPARE(document.items().size(), std::size_t{1});
        QVERIFY(dynamic_cast<const CalloutItem*>(document.items().front().get()));
        QCOMPARE(document.undoStack()->count(), 1);
    }

    void cancelledCalloutKeepsTheArrow() {
        Document document;
        ToolController tools(document);
        tools.setCurrentTool(ToolKind::Callout);
        tools.press({100, 100});
        tools.release({300, 100});
        tools.cancelText();
        QCOMPARE(document.items().size(), std::size_t{1});
        QVERIFY(dynamic_cast<const ShapeItem*>(document.items().front().get()));
    }

    void clickWithCalloutToolIsIgnored() {
        Document document;
        ToolController tools(document);
        tools.setCurrentTool(ToolKind::Callout);
        QSignalSpy requested(&tools, &ToolController::textRequested);
        tools.press({100, 100});
        tools.release({100.5, 100});
        QCOMPARE(requested.count(), 0);
        QVERIFY(document.isEmpty());
    }

    void switchingToolKeepsTypedText() {
        Document document;
        ToolController tools(document);
        tools.setCurrentTool(ToolKind::Text);
        // Stand-in for the on-screen editor: answers commit requests with what was typed.
        connect(&tools, &ToolController::textCommitRequested, &tools,
                [&tools] { tools.commitText(QStringLiteral("typed")); });
        tools.press({10, 10});
        tools.release({10, 10});
        tools.setCurrentTool(ToolKind::Pen);
        QVERIFY(!tools.isAwaitingText());
        QCOMPARE(document.items().size(), std::size_t{1});
        const auto* text = dynamic_cast<const TextItem*>(document.items().front().get());
        QVERIFY(text);
        QCOMPARE(text->text(), QStringLiteral("typed"));
    }

    void cancelBeforeReplayKeepsTypedText() {
        Document document;
        ToolController tools(document);
        tools.setCurrentTool(ToolKind::Callout);
        connect(&tools, &ToolController::textCommitRequested, &tools,
                [&tools] { tools.commitText(QStringLiteral("label")); });
        tools.press({100, 100});
        tools.release({300, 100});
        tools.cancel(); // what a replay does before starting
        QCOMPARE(document.items().size(), std::size_t{1});
        QVERIFY(dynamic_cast<const CalloutItem*>(document.items().front().get()));
    }

    void switchingToolClosesPendingText() {
        Document document;
        ToolController tools(document);
        tools.setCurrentTool(ToolKind::Callout);
        QSignalSpy closed(&tools, &ToolController::textInputClosed);
        tools.press({100, 100});
        tools.release({300, 100});
        tools.setCurrentTool(ToolKind::Pen);
        QCOMPARE(closed.count(), 1);
        QVERIFY(!tools.isAwaitingText());
        QCOMPARE(document.items().size(), std::size_t{1}); // arrow kept
    }

    void gestureCommittedOnlyWhenSomethingWasPlaced() {
        Document document;
        ToolController tools(document);
        QSignalSpy committed(&tools, &ToolController::gestureCommitted);

        tools.press({0, 0}); // pen stroke: placed
        tools.release({50, 50});
        QCOMPARE(committed.count(), 1);
        QCOMPARE(committed.last().first().value<ToolKind>(), ToolKind::Pen);

        tools.setCurrentTool(ToolKind::Rectangle); // click without drag: nothing placed
        tools.press({10, 10});
        tools.release({10.5, 10.5});
        QCOMPARE(committed.count(), 1);

        tools.setCurrentTool(ToolKind::Text); // text: only once the text is committed
        tools.press({5, 5});
        tools.release({5, 5});
        QCOMPARE(committed.count(), 1);
        tools.commitText(QStringLiteral("hi"));
        QCOMPARE(committed.count(), 2);

        tools.press({5, 5}); // empty text: nothing placed
        tools.release({5, 5});
        tools.commitText(QString());
        QCOMPARE(committed.count(), 2);
    }

    void newItemsGoToTheActiveDocument() {
        Document desktop;
        Document whiteboard;
        ToolController tools(desktop);
        QSignalSpy switched(&tools, &ToolController::documentChanged);
        tools.press({0, 0});
        tools.release({50, 50});
        tools.setDocument(whiteboard);
        tools.press({0, 0});
        tools.release({50, 50});
        tools.press({0, 100});
        tools.release({50, 150});
        QCOMPARE(desktop.items().size(), std::size_t{1});
        QCOMPARE(whiteboard.items().size(), std::size_t{2});
        QCOMPARE(switched.count(), 1);
        tools.setDocument(whiteboard); // same page: no signal
        QCOMPARE(switched.count(), 1);
    }
};

QTEST_MAIN(TestText)
#include "tst_text.moc"
