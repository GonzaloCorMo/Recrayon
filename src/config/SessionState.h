#pragma once

#include <QPoint>
#include <QString>

#include <optional>

class QSettings;

namespace recrayon {

/// What the user left as it was when Recrayon closed: where the toolbar was, the stroke width
/// and the tool. Unlike Settings it is not edited in the settings dialog; the application saves
/// it on exit and whenever it changes.
struct SessionState {
    /// Top-left corner of the toolbar in desktop coordinates; empty = default placement.
    std::optional<QPoint> toolbarPosition;
    /// Stroke width in pixels; empty = the tool controller's default.
    std::optional<qreal> strokeWidth;
    /// toolId() of the current tool; empty = the default tool.
    QString tool;
    /// The toolbar was left collapsed against a screen edge.
    bool toolbarCollapsed = false;

    /// Reads the state; missing or invalid values stay empty.
    [[nodiscard]] static SessionState load(QSettings& store);
    void save(QSettings& store) const;

    friend bool operator==(const SessionState&, const SessionState&) = default;
};

} // namespace recrayon
