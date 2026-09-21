#include "platform/DesktopGeometry.h"

#include <QGuiApplication>
#include <QScreen>

namespace recrayon::platform {

ScreenMapping screenMapping(const QScreen* screen) {
    const QRect logical = screen->geometry();
    const qreal dpr = screen->devicePixelRatio();
    const QSize nativeSize(qRound(logical.width() * dpr), qRound(logical.height() * dpr));
#if defined(Q_OS_WIN)
    const QPoint nativeTopLeft = logical.topLeft();
#else
    const QPoint nativeTopLeft = (QPointF(logical.topLeft()) * dpr).toPoint();
#endif
    return {logical, QRect(nativeTopLeft, nativeSize), dpr};
}

DesktopLayout currentDesktopLayout() {
    QList<ScreenMapping> mappings;
    const auto screens = QGuiApplication::screens();
    for (const QScreen* screen : screens) {
        mappings.append(screenMapping(screen));
    }
    return DesktopLayout(std::move(mappings));
}

} // namespace recrayon::platform
