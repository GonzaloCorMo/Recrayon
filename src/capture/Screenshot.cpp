#include "capture/Screenshot.h"

#include "platform/DesktopGeometry.h"

#include <QDateTime>
#include <QDir>
#include <QGuiApplication>
#include <QPainter>
#include <QPixmap>
#include <QScreen>

namespace recrayon::capture {

namespace {

QString newBasePath(const QString& directory) {
    QDir().mkpath(directory); // if it fails, writing the file reports the error
    const QString stamp =
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss-zzz"));
    return QDir(directory).filePath(QStringLiteral("Recrayon_%1").arg(stamp));
}

} // namespace

Grab grabNativeArea(const QRect& nativeArea) {
    if (nativeArea.isEmpty()) {
        return {};
    }
    QImage canvas;
    const auto screens = QGuiApplication::screens();
    for (QScreen* screen : screens) {
        const QRect screenNative = platform::screenMapping(screen).native;
        if (!screenNative.intersects(nativeArea)) {
            continue;
        }
        // Window id 0 = the whole screen as composed by the OS, overlays included.
        QImage image = screen->grabWindow(0).toImage();
        if (image.isNull()) {
            continue;
        }
        image.setDevicePixelRatio(1.0);
        if (screenNative == nativeArea) {
            return {image, nativeArea}; // the common single-screen case needs no copy
        }
        if (canvas.isNull()) {
            canvas = QImage(nativeArea.size(), QImage::Format_ARGB32_Premultiplied);
            canvas.fill(Qt::black);
        }
        QPainter painter(&canvas);
        painter.drawImage(screenNative.topLeft() - nativeArea.topLeft(), image);
    }
    if (canvas.isNull()) {
        return {};
    }
    return {canvas, nativeArea};
}

void paintCursor(Grab& grab, const QImage& cursor, const QPointF& hotspot, const QPointF& cursorPos,
                 const DesktopLayout& layout) {
    const qsizetype index = layout.indexAtLogical(cursorPos);
    if (grab.image.isNull() || cursor.isNull() || index < 0) {
        return;
    }
    const qreal dpr = layout.screens().at(index).devicePixelRatio;
    const QPointF tip = layout.toNative(cursorPos) - QPointF(grab.nativeArea.topLeft());
    const QRectF target(tip - hotspot * dpr, cursor.deviceIndependentSize() * dpr);
    QPainter painter(&grab.image);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(target, cursor);
}

QString newScreenshotPath(const QString& directory) {
    return newBasePath(directory) + QStringLiteral(".png");
}

QString newRecordingBasePath(const QString& directory) {
    return newBasePath(directory);
}

} // namespace recrayon::capture
