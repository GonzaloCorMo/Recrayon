#include "core/FreehandItem.h"
#include "core/Geometry.h"
#include "core/ShapeItem.h"

#include <QTest>

#include <cmath>

using namespace recrayon;

class TestItems : public QObject {
    Q_OBJECT

private slots:
    void distanceToSegment() {
        const QPointF a(0, 0);
        const QPointF b(10, 0);
        QCOMPARE(geometry::distanceToSegment({5, 3}, a, b), 3.0);
        QCOMPARE(geometry::distanceToSegment({-4, 3}, a, b), 5.0);           // beyond the start
        QCOMPARE(geometry::distanceToSegment({1, 1}, a, a), std::sqrt(2.0)); // degenerate
    }

    void freehandIgnoresPointsTooCloseTogether() {
        FreehandItem item{StrokeStyle{}};
        QVERIFY(item.addPoint({0, 0}));
        QVERIFY(!item.addPoint({0.5, 0}));
        QVERIFY(item.addPoint({5, 0}));
        QCOMPARE(item.points().size(), qsizetype{2});
    }

    void freehandBoundsIncludeStrokeWidth() {
        FreehandItem item{StrokeStyle{Qt::red, 10.0}};
        item.addPoint({0, 0});
        item.addPoint({100, 0});

        const QRectF bounds = item.boundingRect();
        QVERIFY(bounds.left() <= -5.0);
        QVERIFY(bounds.right() >= 105.0);
        QVERIFY(bounds.top() <= -5.0);
        QVERIFY(bounds.bottom() >= 5.0);
    }

    void freehandHitTest() {
        FreehandItem item{StrokeStyle{Qt::red, 4.0}};
        item.addPoint({0, 0});
        item.addPoint({50, 0});
        item.addPoint({100, 0});

        QVERIFY(item.hitTest({50, 1}, 0.0));
        QVERIFY(!item.hitTest({50, 5}, 0.0));
        QVERIFY(item.hitTest({50, 5}, 4.0)); // tolerance extends the reach
        QVERIFY(!item.hitTest({50, 40}, 4.0));
    }

    void singlePointFreehandIsHittable() {
        FreehandItem item{StrokeStyle{Qt::red, 4.0}};
        item.addPoint({10, 10});
        QVERIFY(item.hitTest({11, 10}, 0.0));
        QVERIFY(!item.boundingRect().isEmpty());
    }

    void rectangleHitsEdgeNotInterior() {
        const ShapeItem rect(ShapeKind::Rectangle, StrokeStyle{Qt::red, 2.0}, {0, 0}, {100, 100});
        QVERIFY(rect.hitTest({0, 50}, 2.0));
        QVERIFY(rect.hitTest({100, 50}, 2.0));
        QVERIFY(!rect.hitTest({50, 50}, 2.0));
    }

    void ellipseHitsOutlineNotCenter() {
        const ShapeItem ellipse(ShapeKind::Ellipse, StrokeStyle{Qt::red, 2.0}, {0, 0}, {100, 100});
        QVERIFY(ellipse.hitTest({50, 0}, 2.0));
        QVERIFY(!ellipse.hitTest({50, 50}, 2.0));
        QVERIFY(!ellipse.hitTest({2, 2}, 2.0)); // bounding-box corner is outside the ellipse
    }

    void reversedDragIsNormalized() {
        const ShapeItem rect(ShapeKind::Rectangle, StrokeStyle{Qt::red, 2.0}, {100, 100}, {0, 0});
        QVERIFY(rect.hitTest({0, 50}, 2.0));
        QVERIFY(rect.boundingRect().contains(QPointF(50, 50)));
    }

    void arrowBoundsIncludeHead() {
        const ShapeItem arrow(ShapeKind::Arrow, StrokeStyle{Qt::red, 2.0}, {0, 0}, {100, 0});
        const QRectF bounds = arrow.boundingRect();
        QVERIFY(bounds.top() < -5.0);
        QVERIFY(bounds.bottom() > 5.0);
    }

    void freeArrowHasHeadAtLastPoint() {
        FreehandItem item{StrokeStyle{Qt::red, 2.0}, FreehandItem::Ending::Arrow};
        for (int x = 0; x <= 100; x += 10) {
            item.addPoint({static_cast<qreal>(x), 0});
        }
        const auto head = item.arrowHead();
        QVERIFY(head.has_value());
        // Pointing right: barbs sit behind the tip, one above and one below the shaft.
        QVERIFY(head->left.x() < 100.0 && head->right.x() < 100.0);
        QVERIFY(head->left.y() * head->right.y() < 0.0);
        QVERIFY(item.boundingRect().top() < -5.0);
        QVERIFY(item.hitTest(head->left, 1.0));
    }

    void plainFreehandHasNoHead() {
        FreehandItem item{StrokeStyle{}};
        item.addPoint({0, 0});
        item.addPoint({50, 0});
        QVERIFY(!item.arrowHead().has_value());
    }

    void translateMovesFreehandAndShapes() {
        FreehandItem stroke{StrokeStyle{Qt::red, 4.0}};
        stroke.addPoint({0, 0});
        stroke.addPoint({100, 0});
        stroke.translate({10, 20});
        QVERIFY(stroke.hitTest({50, 20}, 0.0));
        QVERIFY(!stroke.hitTest({50, 0}, 0.0));
        QCOMPARE(stroke.points().first(), QPointF(10, 20));

        ShapeItem rect(ShapeKind::Rectangle, StrokeStyle{Qt::red, 2.0}, {0, 0}, {100, 100});
        rect.translate({-50, 0});
        QCOMPARE(rect.start(), QPointF(-50, 0));
        QVERIFY(rect.hitTest({-50, 50}, 2.0));
    }

    void shortDragIsDegenerate() {
        ShapeItem line(ShapeKind::Line, StrokeStyle{}, {10, 10}, {10.5, 10.5});
        QVERIFY(line.isDegenerate());
        line.setEnd({30, 30});
        QVERIFY(!line.isDegenerate());
    }
};

QTEST_GUILESS_MAIN(TestItems)
#include "tst_items.moc"
