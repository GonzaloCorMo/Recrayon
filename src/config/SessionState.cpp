#include "config/SessionState.h"

#include <QSettings>

namespace recrayon {

namespace {

constexpr auto kToolbarPositionKey = "session/toolbarPosition";
constexpr auto kStrokeWidthKey = "session/strokeWidth";
constexpr auto kToolKey = "session/tool";

// Same range as the custom widths the tool controller accepts; anything else is a corrupt file.
constexpr qreal kMinStrokeWidth = 0.5;
constexpr qreal kMaxStrokeWidth = 100.0;

} // namespace

SessionState SessionState::load(QSettings& store) {
    SessionState state;
    const QVariant position = store.value(QLatin1String(kToolbarPositionKey));
    if (position.typeId() == QMetaType::QPoint) { // QSettings keeps the type of a saved QPoint
        state.toolbarPosition = position.toPoint();
    }
    bool ok = false;
    const qreal width = store.value(QLatin1String(kStrokeWidthKey)).toDouble(&ok);
    if (ok && width >= kMinStrokeWidth && width <= kMaxStrokeWidth) {
        state.strokeWidth = width;
    }
    state.tool = store.value(QLatin1String(kToolKey)).toString();
    return state;
}

void SessionState::save(QSettings& store) const {
    if (toolbarPosition) {
        store.setValue(QLatin1String(kToolbarPositionKey), *toolbarPosition);
    } else {
        store.remove(QLatin1String(kToolbarPositionKey));
    }
    if (strokeWidth) {
        store.setValue(QLatin1String(kStrokeWidthKey), *strokeWidth);
    } else {
        store.remove(QLatin1String(kStrokeWidthKey));
    }
    store.setValue(QLatin1String(kToolKey), tool);
}

} // namespace recrayon
