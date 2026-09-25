#pragma once

#include "config/Settings.h"
#include "tools/ToolKind.h"
#include "ui/AppActions.h"
#include "ui/Icons.h"

#include <QIcon>
#include <QList>
#include <QString>
#include <QWidget>

#include <array>

class QAbstractButton;
class QButtonGroup;
class QGridLayout;
class QMenu;
class QToolButton;

namespace recrayon {

class ToolController;

/// Floating palette: mode toggles, tools, colors, widths, capture and edit actions.
///
/// Everything about it is user-configurable: which items appear (Settings::hiddenToolbarItems)
/// and in which order (Settings::toolbarOrder), the button size, the orientation and how many
/// buttons go across it. Consecutive items of the same kind (tools, capture...) share a section;
/// a separator goes between sections. The color swatches come from Settings::palette. The
/// settings button is always present so the configuration can never be lost. Rebuild the toolbar
/// to apply new settings.
///
/// It is a frameless, always-on-top window so it stays above the overlays in both modes, with a
/// taskbar button so it can be minimized and restored. Drag it by its background (or the grip).
class Toolbar final : public QWidget {
    Q_OBJECT

public:
    /// One configurable toolbar entry, for the settings dialog.
    struct ItemInfo {
        QString id;
        QString label;
        QIcon icon;
        bool hideable = true; ///< false for the settings button
    };

    Toolbar(ToolController& tools, const AppActions& actions, const Settings& settings,
            QWidget* parent = nullptr);

    /// Shrinks the toolbar to a small tab against the nearest screen edge (or expands it again).
    /// Useful where the toolbar cannot be kept out of captures (Linux, macOS).
    void setCollapsed(bool collapsed);
    [[nodiscard]] bool isCollapsed() const noexcept { return m_collapsed; }

    /// Every toolbar item, in the default order. Icons are drawn in @p glyph (the toolbar's light
    /// color by default; pass the palette's text color for light backgrounds).
    [[nodiscard]] static QList<ItemInfo> itemCatalog(const QColor& glyph = icons::kGlyphColor);
    /// Ids of itemCatalog(), the default order for orderedToolbarItems().
    [[nodiscard]] static QStringList defaultItemOrder();

signals:
    /// The user picked a drawing tool; the application switches to draw mode.
    void toolPicked();
    /// The user clicked the selected tool again: back to the mouse (interact mode).
    void toolDeselected();
    /// Minimized or restored, by the minimize button, the taskbar or the tray.
    void minimizedChanged(bool minimized);
    /// The system asked to close the toolbar (taskbar "Close window", Alt+F4).
    void quitRequested();
    /// Collapsed against a screen edge, or expanded again.
    void collapsedChanged(bool collapsed);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void changeEvent(QEvent* event) override;
    /// Drag the collapsed tab with the mouse; a click without dragging expands it.
    bool eventFilter(QObject* watched, QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    QToolButton* makeButton(const QIcon& icon, const QString& toolTip);
    QToolButton* makeActionButton(QAction* action);
    QToolButton* makeMenuButton(QAction* action, QMenu* menu);
    QToolButton* makeToolButton(ToolKind kind, icons::IconId icon, const QString& label);
    /// Screen edge the toolbar is closest to, and where the collapsed tab sticks.
    [[nodiscard]] Qt::Edge nearestEdge() const;
    void moveCollapsedToEdge();
    void updateHandle();
    void keepOnScreen();
    /// Adds @p widget to the next cell of a section, filling m_lanes cells across the toolbar.
    void place(QGridLayout* grid, QWidget* widget) const;
    void buildColorSection(QGridLayout* grid);
    void buildWidthSection(QGridLayout* grid);
    void syncColorSelection(const QColor& color);
    void syncWidthSelection(qreal value);
    /// A tool button is checked only while drawing with it; none checked = the mouse.
    void syncToolSelection();
    void pickCustomColor();

    ToolController& m_tools;
    std::array<QColor, kPaletteSize> m_palette;
    QButtonGroup* m_toolGroup = nullptr;
    QAction* m_drawAction = nullptr; ///< checked while in draw mode
    QButtonGroup* m_colorGroup = nullptr;
    QButtonGroup* m_widthGroup = nullptr;
    QToolButton* m_customColorButton = nullptr;
    QWidget* m_body = nullptr;       ///< everything except the collapsed tab
    QToolButton* m_handle = nullptr; ///< the arrow that expands the toolbar again
    bool m_collapsed = false;
    Qt::Edge m_edge = Qt::RightEdge; ///< edge the collapsed tab sticks to
    QPoint m_expandedPos;            ///< where to go back to when expanding
    QPoint m_handlePressPos;         ///< where a drag of the collapsed tab started
    bool m_handleDragging = false;
    bool m_collapsedMoved = false; ///< the tab was dragged while collapsed
    ToolbarMetrics m_metrics;
    bool m_menuOnLeftClick = true;
    bool m_vertical = true;
    int m_lanes = 2;
};

} // namespace recrayon
