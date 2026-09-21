#include "core/DesktopLayout.h"
#include "core/Geometry.h"

#include <QTest>

using namespace recrayon;

namespace {

/// The layout Qt reports on Windows for a 1920x1200 screen at 125 % next to a 1920x1080 one at
/// 100 %: origins stay native, sizes are logical, so there is a gap between 1536 and 1920.
DesktopLayout mixedDpi() {
    return DesktopLayout({
        {QRect(0, 0, 1536, 960), QRect(0, 0, 1920, 1200), 1.25},
        {QRect(1920, 0, 1920, 1080), QRect(1920, 0, 1920, 1080), 1.0},
    });
}

} // namespace

class TestDesktopLayout : public QObject {
    Q_OBJECT

private slots:
    void nativeBoundsCoverAllScreens() {
        QCOMPARE(mixedDpi().nativeBounds(), QRect(0, 0, 3840, 1200));
    }

    void pointsMapThroughTheirOwnScreen() {
        const DesktopLayout layout = mixedDpi();
        QCOMPARE(layout.toNative(QPointF(100, 100)), QPointF(125, 125));
        QCOMPARE(layout.toNative(QPointF(2000, 50)), QPointF(2000, 50));
        QCOMPARE(layout.toLogical(QPointF(125, 125)), QPointF(100, 100));
        QCOMPARE(layout.toLogical(QPointF(2000, 50)), QPointF(2000, 50));
    }

    void pointsInGapsUseNearestScreen() {
        const DesktopLayout layout = mixedDpi();
        QCOMPARE(layout.indexAtLogical(QPointF(1600, 10)), qsizetype{0}); // 64 px from screen 0
        QCOMPARE(layout.indexAtLogical(QPointF(1900, 10)), qsizetype{1}); // 20 px from screen 1
    }

    void rectangleSpanningScreensMapsEachCorner() {
        const DesktopLayout layout = mixedDpi();
        const QRect native = layout.toNative(QRectF(QPointF(1000, 100), QPointF(2100, 300)));
        QCOMPARE(native.topLeft(), QPoint(1250, 125));
        QCOMPARE(native.right() + 1, 2100);
        QCOMPARE(native.bottom() + 1, 300);
    }

    void emptyLayoutIsIdentity() {
        const DesktopLayout layout;
        QCOMPARE(layout.toNative(QPointF(5, 7)), QPointF(5, 7));
        QCOMPARE(layout.indexAtLogical(QPointF(0, 0)), qsizetype{-1});
    }

    void fitCenteredLetterboxes() {
        const QRectF fitted = geometry::fitCentered(QSizeF(1920, 1080), QRectF(0, 0, 1920, 1200));
        QCOMPARE(fitted, QRectF(0, 60, 1920, 1080));
        const QRectF pillar = geometry::fitCentered(QSizeF(100, 200), QRectF(0, 0, 400, 200));
        QCOMPARE(pillar, QRectF(150, 0, 100, 200));
    }

    void evenVideoSizeRules() {
        QCOMPARE(geometry::evenVideoSize(QSize(801, 603)), QSize(800, 602));
        QCOMPARE(geometry::evenVideoSize(QSize(5760, 1080)), QSize(3840, 720));
        QCOMPARE(geometry::evenVideoSize(QSize(3, 3)), QSize(16, 16));
    }
};

QTEST_GUILESS_MAIN(TestDesktopLayout)
#include "tst_desktoplayout.moc"
