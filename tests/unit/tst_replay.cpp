#include "core/CalloutItem.h"
#include "core/FreehandItem.h"
#include "core/ReplayTimeline.h"
#include "core/ShapeItem.h"
#include "core/TextItem.h"

#include <QImage>
#include <QPainter>
#include <QTest>

using namespace recrayon;

namespace {

ItemPtr line(qreal length) {
    return std::make_shared<ShapeItem>(ShapeKind::Line, StrokeStyle{Qt::black, 4.0},
                                       QPointF(10, 50), QPointF(10 + length, 50));
}

/// Number of non-transparent pixels after painting @p item at @p progress.
int inkAt(const Item& item, qreal progress) {
    QImage image(400, 200, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    item.paintPartial(painter, progress);
    painter.end();
    int ink = 0;
    for (int y = 0; y < image.height(); ++y) {
        const auto* row = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            ink += qAlpha(row[x]) > 0 ? 1 : 0;
        }
    }
    return ink;
}

} // namespace

class TestReplay : public QObject {
    Q_OBJECT

private slots:
    void itemsPlayOneAfterAnother() {
        const ReplayTimeline timeline({line(300), line(300)}, 1.0);
        QCOMPARE(timeline.progress(0, 0), 0.0);
        QVERIFY(timeline.progress(0, 150) > 0.0);
        QCOMPARE(timeline.progress(1, 150), 0.0); // second waits for the first
        QCOMPARE(timeline.progress(0, timeline.durationMs()), 1.0);
        QCOMPARE(timeline.progress(1, timeline.durationMs()), 1.0);
    }

    void durationFollowsLengthAndSpeed() {
        const ReplayTimeline normal({line(900)}, 1.0);
        QCOMPARE(normal.durationMs(), qint64{1000}); // 900 px at 900 px/s
        const ReplayTimeline fast({line(900)}, 2.0);
        QCOMPARE(fast.durationMs(), qint64{500});
        const ReplayTimeline tiny({line(1)}, 1.0);
        QCOMPARE(tiny.durationMs(), ReplayTimeline::kMinItemMs);
        const ReplayTimeline huge({line(100000)}, 1.0);
        QCOMPARE(huge.durationMs(), ReplayTimeline::kMaxItemMs);
    }

    void emptyTimeline() {
        const ReplayTimeline timeline({}, 1.0);
        QVERIFY(timeline.isEmpty());
        QCOMPARE(timeline.durationMs(), qint64{0});
    }

    void partialPaintGrowsWithProgress() {
        auto stroke = std::make_shared<FreehandItem>(StrokeStyle{Qt::black, 4.0});
        for (int x = 10; x <= 390; x += 10) {
            stroke->addPoint({static_cast<qreal>(x), 100});
        }
        const QList<ItemPtr> items{
            stroke,
            line(350),
            std::make_shared<ShapeItem>(ShapeKind::Ellipse, StrokeStyle{Qt::black, 4.0},
                                        QPointF(20, 20), QPointF(380, 180)),
            std::make_shared<TextItem>(StrokeStyle{Qt::black, 4.0}, QFont(), QPointF(10, 10),
                                       Qt::AlignLeft | Qt::AlignTop, 0.0,
                                       QStringLiteral("Replay me please")),
            std::make_shared<CalloutItem>(StrokeStyle{Qt::black, 4.0}, QFont(), QPointF(200, 150),
                                          QPointF(380, 20), QStringLiteral("Here")),
        };
        for (const ItemPtr& item : items) {
            const int none = inkAt(*item, 0.0);
            const int half = inkAt(*item, 0.5);
            const int full = inkAt(*item, 1.0);
            QCOMPARE(none, 0);
            QVERIFY2(half > 0 && half < full,
                     qPrintable(QStringLiteral("%1 < %2").arg(half).arg(full)));
        }
    }
};

QTEST_MAIN(TestReplay)
#include "tst_replay.moc"
