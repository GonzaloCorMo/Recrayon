#include "platform/CursorSprite.h"

#include <QCursor>
#include <QGuiApplication>
#include <QPainter>
#include <QPolygonF>
#include <QScreen>

#include <algorithm>

#if defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace recrayon::platform {

namespace {

[[maybe_unused]] qreal devicePixelRatioAtCursor() {
    const QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
    return screen ? screen->devicePixelRatio() : 1.0;
}

[[maybe_unused]] CursorSprite genericArrow() {
    constexpr qreal kScale = 2.0;
    QImage image(QSize(14, 22) * kScale, QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(kScale);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(Qt::black, 1.2));
    painter.setBrush(Qt::white);
    painter.drawPolygon(QPolygonF{QPointF(1, 1), QPointF(1, 17), QPointF(5, 13), QPointF(8, 20),
                                  QPointF(10.5, 19), QPointF(7.5, 12.5), QPointF(12.5, 12.5)});
    return {image, QPointF(1, 1), true};
}

#if defined(Q_OS_WIN)

/// Draws @p cursor with GDI onto an opaque background of color @p background.
QImage renderOn(HCURSOR cursor, const QSize& size, COLORREF background) {
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = size.width();
    info.bmiHeader.biHeight = -size.height(); // top-down rows, like QImage
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    HDC dc = CreateCompatibleDC(nullptr);
    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dc || !bitmap || !bits) {
        if (bitmap) {
            DeleteObject(bitmap);
        }
        if (dc) {
            DeleteDC(dc);
        }
        return {};
    }
    HGDIOBJ previous = SelectObject(dc, bitmap);
    HBRUSH brush = CreateSolidBrush(background);
    RECT area{0, 0, size.width(), size.height()};
    FillRect(dc, &area, brush);
    DeleteObject(brush);
    DrawIconEx(dc, 0, 0, cursor, size.width(), size.height(), 0, nullptr, DI_NORMAL);
    GdiFlush();

    const QImage view(static_cast<const uchar*>(bits), size.width(), size.height(),
                      size.width() * 4, QImage::Format_RGB32);
    QImage copy = view.copy();
    SelectObject(dc, previous);
    DeleteObject(bitmap);
    DeleteDC(dc);
    return copy;
}

/// Converts a cursor to an image with real transparency. QImage::fromHICON() does not handle
/// cursors, and GDI does not produce an alpha channel, so the cursor is drawn over black and
/// over white: where they differ the cursor is translucent. This also turns inverting
/// (monochrome XOR) cursors such as the I-beam into a visible light shape.
QImage cursorToImage(HCURSOR cursor, const QSize& size) {
    const QImage onBlack = renderOn(cursor, size, RGB(0, 0, 0));
    const QImage onWhite = renderOn(cursor, size, RGB(255, 255, 255));
    if (onBlack.isNull() || onWhite.isNull()) {
        return {};
    }
    QImage result(size, QImage::Format_ARGB32);
    for (int y = 0; y < size.height(); ++y) {
        const auto* black = reinterpret_cast<const QRgb*>(onBlack.constScanLine(y));
        const auto* white = reinterpret_cast<const QRgb*>(onWhite.constScanLine(y));
        auto* out = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < size.width(); ++x) {
            const int difference =
                ((qRed(white[x]) - qRed(black[x])) + (qGreen(white[x]) - qGreen(black[x])) +
                 (qBlue(white[x]) - qBlue(black[x]))) /
                3;
            const int alpha = std::clamp(255 - difference, 0, 255);
            if (alpha == 0) {
                out[x] = qRgba(0, 0, 0, 0);
                continue;
            }
            // Over black the color is premultiplied by alpha: undo that.
            const auto unpremultiply = [alpha](int channel) {
                return std::min(255, channel * 255 / alpha);
            };
            out[x] = qRgba(unpremultiply(qRed(black[x])), unpremultiply(qGreen(black[x])),
                           unpremultiply(qBlue(black[x])), alpha);
        }
    }
    return result;
}

/// Size and hotspot of @p cursor, read from its bitmaps.
bool cursorGeometry(HCURSOR cursor, QSize* size, QPoint* hotspot) {
    ICONINFO iconInfo{};
    if (!GetIconInfo(cursor, &iconInfo)) {
        return false;
    }
    BITMAP bitmap{};
    bool ok = false;
    if (iconInfo.hbmColor && GetObject(iconInfo.hbmColor, sizeof(bitmap), &bitmap)) {
        *size = QSize(bitmap.bmWidth, bitmap.bmHeight);
        ok = true;
    } else if (iconInfo.hbmMask && GetObject(iconInfo.hbmMask, sizeof(bitmap), &bitmap)) {
        // Monochrome cursors stack the AND and XOR masks vertically.
        *size = QSize(bitmap.bmWidth, bitmap.bmHeight / 2);
        ok = true;
    }
    *hotspot = QPoint(static_cast<int>(iconInfo.xHotspot), static_cast<int>(iconInfo.yHotspot));
    if (iconInfo.hbmMask) {
        DeleteObject(iconInfo.hbmMask);
    }
    if (iconInfo.hbmColor) {
        DeleteObject(iconInfo.hbmColor);
    }
    return ok && !size->isEmpty();
}

#endif // Q_OS_WIN

} // namespace

CursorSprite currentCursorSprite() {
#if defined(Q_OS_WIN)
    // Converting the cursor is not free and callers poll at display rate: reuse the previous
    // conversion while the handle (i.e. the cursor shape) does not change.
    static HCURSOR cachedHandle = nullptr;
    static CursorSprite cached;

    CURSORINFO info{};
    info.cbSize = sizeof(info);
    const bool haveInfo = GetCursorInfo(&info) != 0;
    const bool showing = haveInfo && (info.flags & CURSOR_SHOWING) != 0;
    HCURSOR handle = haveInfo ? info.hCursor : nullptr;
    if (!handle) {
        // Hidden (e.g. "hide pointer while typing" reports no cursor at all): keep the last
        // shape we saw, or the standard arrow if there is none yet.
        handle = cachedHandle ? cachedHandle : LoadCursor(nullptr, IDC_ARROW);
    }

    const qreal dpr = devicePixelRatioAtCursor();
    if (handle != cachedHandle || !qFuzzyCompare(cached.image.devicePixelRatio(), dpr)) {
        QSize size;
        QPoint hotspot;
        if (!cursorGeometry(handle, &size, &hotspot)) {
            return {};
        }
        QImage image = cursorToImage(handle, size);
        if (image.isNull()) {
            return {};
        }
        // Windows already scales cursors for the display, so the bitmap is in device pixels.
        image.setDevicePixelRatio(dpr);
        cached = {image, QPointF(hotspot) / dpr, true};
        cachedHandle = handle;
    }
    CursorSprite sprite = cached;
    sprite.visible = showing;
    return sprite;
#else
    return genericArrow();
#endif
}

} // namespace recrayon::platform
