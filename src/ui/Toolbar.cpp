#include "ui/Toolbar.h"

#include "tools/ToolController.h"
#include "ui/Icons.h"

#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QColorDialog>
#include <QCoreApplication>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QToolButton>
#include <QTransform>
#include <QVBoxLayout>
#include <QWindow>

#include <algorithm>
#include <array>
#include <optional>

namespace recrayon {

namespace {

using icons::IconId;

constexpr int kGripThickness = 16;
constexpr int kSectionSpacing = 6;
constexpr int kButtonSpacing = 2;
constexpr qreal kCornerRadius = 10.0;

struct ToolEntry {
    ToolKind kind;
    const char* id;
    IconId icon;
    const char* label;
};

// Toolbar ids are stored in the user's settings: never rename one.
const std::array<ToolEntry, kToolCount> kToolEntries{{
    {ToolKind::Pen, "tool.pen", IconId::Pen, QT_TRANSLATE_NOOP("recrayon::Toolbar", "Pen")},
    {ToolKind::Highlighter, "tool.highlighter", IconId::Highlighter,
     QT_TRANSLATE_NOOP("recrayon::Toolbar", "Highlighter")},
    {ToolKind::FreeArrow, "tool.freeArrow", IconId::FreeArrow,
     QT_TRANSLATE_NOOP("recrayon::Toolbar", "Free arrow")},
    {ToolKind::Line, "tool.line", IconId::Line, QT_TRANSLATE_NOOP("recrayon::Toolbar", "Line")},
    {ToolKind::Arrow, "tool.arrow", IconId::Arrow, QT_TRANSLATE_NOOP("recrayon::Toolbar", "Arrow")},
    {ToolKind::Callout, "tool.callout", IconId::Callout,
     QT_TRANSLATE_NOOP("recrayon::Toolbar", "Arrow with text")},
    {ToolKind::Rectangle, "tool.rectangle", IconId::Rectangle,
     QT_TRANSLATE_NOOP("recrayon::Toolbar", "Rectangle")},
    {ToolKind::Ellipse, "tool.ellipse", IconId::Ellipse,
     QT_TRANSLATE_NOOP("recrayon::Toolbar", "Ellipse")},
    {ToolKind::Text, "tool.text", IconId::Text, QT_TRANSLATE_NOOP("recrayon::Toolbar", "Text")},
    {ToolKind::Eraser, "tool.eraser", IconId::Eraser,
     QT_TRANSLATE_NOOP("recrayon::Toolbar", "Eraser")},
    {ToolKind::Move, "tool.move", IconId::Move, QT_TRANSLATE_NOOP("recrayon::Toolbar", "Move")},
}};

const QString kDrawMode = QStringLiteral("drawMode");
const QString kVisibility = QStringLiteral("visibility");
const QString kWhiteboard = QStringLiteral("whiteboard");
const QString kColors = QStringLiteral("colors");
const QString kWidths = QStringLiteral("widths");
const QString kReplay = QStringLiteral("replay");
const QString kSpotlight = QStringLiteral("spotlight");
const QString kHalo = QStringLiteral("halo");
const QString kScreenshot = QStringLiteral("screenshot");
const QString kRecord = QStringLiteral("record");
const QString kUndo = QStringLiteral("undo");
const QString kRedo = QStringLiteral("redo");
const QString kClear = QStringLiteral("clear");
const QString kSettings = QStringLiteral("settings");
const QString kMinimize = QStringLiteral("minimize");
const QString kQuit = QStringLiteral("quit");

/// Consecutive items of the same group share a section; a separator goes between sections.
enum class Group { Modes, Tools, Colors, Widths, Capture, Edit };

Group groupOf(const QString& id) {
    if (id.startsWith(QLatin1String("tool."))) {
        return Group::Tools;
    }
    if (id == kDrawMode || id == kVisibility || id == kWhiteboard) {
        return Group::Modes;
    }
    if (id == kColors) {
        return Group::Colors;
    }
    if (id == kWidths) {
        return Group::Widths;
    }
    if (id == kReplay || id == kSpotlight || id == kHalo || id == kScreenshot || id == kRecord) {
        return Group::Capture;
    }
    return Group::Edit;
}

const ToolEntry* toolEntry(const QString& id) {
    const auto found = std::find_if(kToolEntries.cbegin(), kToolEntries.cend(),
                                    [&id](const ToolEntry& entry) { return id == entry.id; });
    return found == kToolEntries.cend() ? nullptr : &*found;
}

constexpr int kCustomColorId = static_cast<int>(kPaletteSize);
constexpr std::array<qreal, 4> kPresetWidths{2.0, 4.0, 8.0, 14.0};

constexpr auto kStyleSheet = R"(
QToolButton {
    border: none;
    border-radius: 6px;
    background: transparent;
}
QToolButton:hover {
    background: rgba(255, 255, 255, 26);
}
QToolButton:checked {
    background: rgba(138, 180, 248, 90);
}
QToolButton:disabled {
    background: transparent;
}
QFrame#separator {
    background: rgba(255, 255, 255, 40);
}
)";

QString toolLabel(const ToolEntry& entry) {
    return QCoreApplication::translate("recrayon::Toolbar", entry.label);
}

} // namespace

QStringList Toolbar::defaultItemOrder() {
    QStringList ids{kDrawMode, kVisibility, kWhiteboard};
    for (const ToolEntry& entry : kToolEntries) {
        ids << QString::fromLatin1(entry.id);
    }
    ids << kColors << kWidths << kReplay << kSpotlight << kHalo << kScreenshot << kRecord << kUndo
        << kRedo << kClear << kSettings << kMinimize << kQuit;
    return ids;
}

QList<Toolbar::ItemInfo> Toolbar::itemCatalog(const QColor& glyph) {
    const auto describe = [&glyph](const QString& id) -> ItemInfo {
        if (const ToolEntry* entry = toolEntry(id)) {
            return {id, toolLabel(*entry), icons::icon(entry->icon, glyph)};
        }
        if (id == kDrawMode) {
            return {id, tr("Draw mode"), icons::icon(IconId::Cursor, glyph)};
        }
        if (id == kVisibility) {
            return {id, tr("Show / hide annotations"), icons::icon(IconId::Visibility, glyph)};
        }
        if (id == kWhiteboard) {
            return {id, tr("Whiteboard"), icons::icon(IconId::Whiteboard, glyph)};
        }
        if (id == kColors) {
            return {id, tr("Color palette"), icons::swatch(Settings::defaultPalette()[0])};
        }
        if (id == kWidths) {
            return {id, tr("Stroke widths"), icons::strokeWidth(8.0, glyph)};
        }
        if (id == kReplay) {
            return {id, tr("Replay the drawing"), icons::icon(IconId::Play, glyph)};
        }
        if (id == kSpotlight) {
            return {id, tr("Spotlight cursor"), icons::icon(IconId::Spotlight, glyph)};
        }
        if (id == kHalo) {
            return {id, tr("Highlight cursor"), icons::icon(IconId::Halo, glyph)};
        }
        if (id == kScreenshot) {
            return {id, tr("Screenshot"), icons::icon(IconId::Screenshot, glyph)};
        }
        if (id == kRecord) {
            return {id, tr("Record screen"), icons::icon(IconId::Record, glyph)};
        }
        if (id == kUndo) {
            return {id, tr("Undo"), icons::icon(IconId::Undo, glyph)};
        }
        if (id == kRedo) {
            return {id, tr("Redo"), icons::icon(IconId::Redo, glyph)};
        }
        if (id == kClear) {
            return {id, tr("Clear all"), icons::icon(IconId::Clear, glyph)};
        }
        if (id == kSettings) {
            // Always shown: it is the way back to this configuration.
            return {id, tr("Settings"), icons::icon(IconId::Settings, glyph), false};
        }
        if (id == kMinimize) {
            return {id, tr("Collapse to the edge"), icons::icon(IconId::Collapse, glyph)};
        }
        return {id, tr("Quit"), icons::icon(IconId::Quit, glyph)};
    };
    QList<ItemInfo> items;
    const QStringList ids = defaultItemOrder();
    for (const QString& id : ids) {
        items.append(describe(id));
    }
    return items;
}

Toolbar::Toolbar(ToolController& tools, const AppActions& actions, const Settings& settings,
                 QWidget* parent)
    // A normal window, not Qt::Tool: tool windows have no taskbar button, and a minimized
    // toolbar comes back from there.
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                          Qt::WindowMinimizeButtonHint | Qt::NoDropShadowWindowHint),
      m_tools(tools), m_palette(settings.palette), m_drawAction(actions.toggleDrawing),
      m_metrics(toolbarMetrics(settings.toolbarSize)),
      m_menuOnLeftClick(settings.toolbarMenuTrigger == ToolbarMenuTrigger::LeftClick),
      m_vertical(settings.toolbarOrientation == ToolbarOrientation::Vertical),
      m_lanes(std::clamp(settings.toolbarLanes, kMinToolbarLanes, kMaxToolbarLanes)) {
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowTitle(tr("Recrayon"));
    setStyleSheet(QString::fromLatin1(kStyleSheet));

    // The grip strip is at the start: above the sections when vertical, left of them otherwise.
    // Outer layout: the toolbar itself, or the small tab it collapses into.
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);
    outer->setSizeConstraint(QLayout::SetFixedSize);
    m_body = new QWidget(this);
    outer->addWidget(m_body);
    m_handle = makeButton(icons::icon(IconId::Collapse), tr("Show the toolbar"));
    m_handle->hide();
    m_handle->installEventFilter(this); // a click expands, a drag moves the collapsed tab
    outer->addWidget(m_handle);

    const int margin = m_metrics.buttonSize / 4;
    auto* root =
        new QBoxLayout(m_vertical ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight, m_body);
    root->setContentsMargins(m_vertical ? margin : kGripThickness,
                             m_vertical ? kGripThickness : margin, margin, margin);
    root->setSpacing(kSectionSpacing);

    // Not exclusive: "no tool" (the mouse) must be a valid state. Exclusivity is kept by
    // syncToolSelection(). The group exists even with every tool hidden, for toolChanged().
    m_toolGroup = new QButtonGroup(this);
    m_toolGroup->setExclusive(false);
    connect(m_toolGroup, &QButtonGroup::idClicked, this, [this](int id) {
        const auto kind = static_cast<ToolKind>(id);
        if (m_drawAction->isChecked() && kind == m_tools.currentTool()) {
            emit toolDeselected();
        } else {
            m_tools.setCurrentTool(kind);
            emit toolPicked();
        }
        syncToolSelection(); // the click toggled the button; show the real state
    });

    const auto actionFor = [&actions](const QString& id) -> std::pair<QAction*, QMenu*> {
        if (id == kDrawMode) {
            return {actions.toggleDrawing, nullptr};
        }
        if (id == kVisibility) {
            return {actions.toggleVisibility, nullptr};
        }
        if (id == kWhiteboard) {
            return {actions.toggleWhiteboard, actions.whiteboardMenu};
        }
        if (id == kReplay) {
            return {actions.toggleReplay, nullptr};
        }
        if (id == kSpotlight) {
            return {actions.toggleSpotlight, nullptr};
        }
        if (id == kHalo) {
            return {actions.toggleHalo, nullptr};
        }
        if (id == kScreenshot) {
            return {actions.screenshot, actions.screenshotMenu};
        }
        if (id == kRecord) {
            return {actions.toggleRecording, actions.recordingMenu};
        }
        if (id == kUndo) {
            return {actions.undo, nullptr};
        }
        if (id == kRedo) {
            return {actions.redo, nullptr};
        }
        if (id == kClear) {
            return {actions.clear, nullptr};
        }
        if (id == kSettings) {
            return {actions.settings, nullptr};
        }
        if (id == kMinimize) {
            return {actions.collapseToolbar, nullptr};
        }
        if (id == kQuit) {
            return {actions.quit, nullptr};
        }
        return {nullptr, nullptr};
    };

    // Sections are created when their first item is placed, so hidden items leave no empty space
    // or double separators behind.
    QGridLayout* section = nullptr;
    std::optional<Group> sectionGroup;
    const auto sectionFor = [&](Group group) {
        if (section && sectionGroup == group) {
            return section;
        }
        if (section) {
            auto* line = new QFrame(m_body);
            line->setObjectName(QStringLiteral("separator"));
            if (m_vertical) {
                line->setFixedHeight(1);
            } else {
                line->setFixedWidth(1);
            }
            root->addWidget(line);
        }
        section = new QGridLayout;
        section->setContentsMargins(0, 0, 0, 0);
        section->setSpacing(kButtonSpacing);
        root->addLayout(section);
        sectionGroup = group;
        return section;
    };

    const QStringList order = orderedToolbarItems(defaultItemOrder(), settings.toolbarOrder);
    for (const QString& id : order) {
        if (id != kSettings && !settings.isToolbarItemVisible(id)) {
            continue;
        }
        const Group group = groupOf(id);
        if (const ToolEntry* entry = toolEntry(id)) {
            place(sectionFor(group), makeToolButton(entry->kind, entry->icon, toolLabel(*entry)));
        } else if (id == kColors) {
            buildColorSection(sectionFor(group));
        } else if (id == kWidths) {
            buildWidthSection(sectionFor(group));
        } else if (const auto [action, menu] = actionFor(id); action) {
            place(sectionFor(group), makeMenuButton(action, menu));
        }
    }

    connect(&m_tools, &ToolController::toolChanged, this, &Toolbar::syncToolSelection);
    connect(m_drawAction, &QAction::toggled, this, &Toolbar::syncToolSelection);
    syncToolSelection();
    if (m_colorGroup) {
        connect(&m_tools, &ToolController::colorChanged, this, &Toolbar::syncColorSelection);
        syncColorSelection(m_tools.color());
    }
    if (m_widthGroup) {
        connect(&m_tools, &ToolController::widthChanged, this, &Toolbar::syncWidthSelection);
        syncWidthSelection(m_tools.width());
    }
}

void Toolbar::place(QGridLayout* grid, QWidget* widget) const {
    // Fill across the bar first (m_lanes buttons), then along it.
    const int index = grid->count();
    const int along = index / m_lanes;
    const int across = index % m_lanes;
    if (m_vertical) {
        grid->addWidget(widget, along, across);
    } else {
        grid->addWidget(widget, across, along);
    }
}

QToolButton* Toolbar::makeButton(const QIcon& icon, const QString& toolTip) {
    auto* button = new QToolButton(this);
    button->setIcon(icon);
    button->setIconSize(QSize(m_metrics.iconSize, m_metrics.iconSize));
    button->setFixedSize(m_metrics.buttonSize, m_metrics.buttonSize);
    button->setToolTip(toolTip);
    button->setAccessibleName(toolTip); // icon-only buttons: screen readers need a name
    button->setFocusPolicy(Qt::NoFocus);
    button->setAutoRaise(true);
    return button;
}

QToolButton* Toolbar::makeActionButton(QAction* action) {
    Q_ASSERT(action);
    QToolButton* button = makeButton(action->icon(), action->toolTip());
    button->setDefaultAction(action);
    return button;
}

QToolButton* Toolbar::makeMenuButton(QAction* action, QMenu* menu) {
    QToolButton* button = makeActionButton(action);
    if (!menu) {
        return button;
    }
    button->setMenu(menu);
    // Right-click (and press and hold) always opens the menu, whatever the left click does.
    button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(button, &QWidget::customContextMenuRequested, button,
            [button, menu](const QPoint& pos) { menu->popup(button->mapToGlobal(pos)); });

    if (!m_menuOnLeftClick) {
        button->setPopupMode(QToolButton::DelayedPopup); // click = default target
        return button;
    }
    // Click = menu, except while the action is on: then the click has to switch it off (stop the
    // recording, leave the whiteboard), which InstantPopup would make impossible.
    const auto applyPopupMode = [button](bool on) {
        button->setPopupMode(on ? QToolButton::DelayedPopup : QToolButton::InstantPopup);
    };
    applyPopupMode(action->isChecked());
    if (action->isCheckable()) {
        connect(action, &QAction::toggled, button, applyPopupMode);
    }
    return button;
}

QToolButton* Toolbar::makeToolButton(ToolKind kind, icons::IconId icon, const QString& label) {
    const QString key = QKeySequence(shortcutKey(kind)).toString(QKeySequence::NativeText);
    QToolButton* button = makeButton(icons::icon(icon), QStringLiteral("%1 (%2)").arg(label, key));
    button->setCheckable(true);
    m_toolGroup->addButton(button, static_cast<int>(kind));
    return button;
}

void Toolbar::syncToolSelection() {
    if (!m_toolGroup) {
        return;
    }
    const bool drawing = m_drawAction && m_drawAction->isChecked();
    const auto buttons = m_toolGroup->buttons();
    for (QAbstractButton* button : buttons) {
        button->setChecked(drawing &&
                           m_toolGroup->id(button) == static_cast<int>(m_tools.currentTool()));
    }
}

void Toolbar::buildColorSection(QGridLayout* grid) {
    m_colorGroup = new QButtonGroup(this);
    m_colorGroup->setExclusive(true);
    for (std::size_t i = 0; i < m_palette.size(); ++i) {
        const QColor& color = m_palette[i];
        QToolButton* button = makeButton(icons::swatch(color), color.name());
        button->setCheckable(true);
        m_colorGroup->addButton(button, static_cast<int>(i));
        place(grid, button);
    }
    m_customColorButton = makeButton(icons::icon(IconId::CustomColor), tr("Custom color…"));
    m_customColorButton->setCheckable(true);
    m_colorGroup->addButton(m_customColorButton, kCustomColorId);
    place(grid, m_customColorButton);

    connect(m_colorGroup, &QButtonGroup::idClicked, this, [this](int id) {
        if (id == kCustomColorId) {
            pickCustomColor();
            return;
        }
        m_tools.setColor(m_palette[static_cast<std::size_t>(id)]);
    });
}

void Toolbar::buildWidthSection(QGridLayout* grid) {
    m_widthGroup = new QButtonGroup(this);
    m_widthGroup->setExclusive(true);
    for (std::size_t i = 0; i < kPresetWidths.size(); ++i) {
        const qreal presetWidth = kPresetWidths[i];
        QToolButton* button =
            makeButton(icons::strokeWidth(presetWidth), tr("Width %1 px").arg(presetWidth));
        button->setCheckable(true);
        m_widthGroup->addButton(button, static_cast<int>(i));
        place(grid, button);
    }
    connect(m_widthGroup, &QButtonGroup::idClicked, this,
            [this](int id) { m_tools.setWidth(kPresetWidths[static_cast<std::size_t>(id)]); });
}

void Toolbar::pickCustomColor() {
    // Parented to the toolbar (a top-most window) so the dialog is not hidden behind overlays.
    const QColor chosen = QColorDialog::getColor(m_tools.color(), this, tr("Pick a color"));
    if (chosen.isValid()) {
        m_tools.setColor(chosen);
    }
    syncColorSelection(m_tools.color());
}

void Toolbar::syncColorSelection(const QColor& color) {
    for (std::size_t i = 0; i < m_palette.size(); ++i) {
        if (m_palette[i].rgb() == color.rgb()) {
            m_colorGroup->button(static_cast<int>(i))->setChecked(true);
            return;
        }
    }
    m_customColorButton->setChecked(true);
}

void Toolbar::syncWidthSelection(qreal value) {
    for (std::size_t i = 0; i < kPresetWidths.size(); ++i) {
        if (qFuzzyCompare(kPresetWidths[i], value)) {
            m_widthGroup->button(static_cast<int>(i))->setChecked(true);
            return;
        }
    }
    // Width set elsewhere: clear the selection.
    m_widthGroup->setExclusive(false);
    const auto buttons = m_widthGroup->buttons();
    for (QAbstractButton* button : buttons) {
        button->setChecked(false);
    }
    m_widthGroup->setExclusive(true);
}

void Toolbar::setCollapsed(bool collapsed) {
    if (m_collapsed == collapsed) {
        return;
    }
    m_collapsed = collapsed;
    if (collapsed) {
        m_expandedPos = pos();
        m_collapsedMoved = false;
        m_edge = nearestEdge();
        m_body->hide();
        updateHandle();
        m_handle->show();
        adjustSize();
        moveCollapsedToEdge();
    } else {
        m_handle->hide();
        m_body->show();
        adjustSize();
        // Back where it was, unless the tab was dragged somewhere else meanwhile.
        if (!m_collapsedMoved) {
            move(m_expandedPos);
        }
        keepOnScreen();
    }
    update();
    emit collapsedChanged(collapsed);
}

Qt::Edge Toolbar::nearestEdge() const {
    const QRect frame(pos(), size());
    const QScreen* screen = QGuiApplication::screenAt(frame.center());
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen) {
        return Qt::RightEdge;
    }
    // A column collapses sideways and a row up or down: the tab then keeps the place the
    // toolbar had along its long axis, instead of jumping to a corner.
    const QRect area = screen->availableGeometry();
    if (m_vertical) {
        return frame.left() - area.left() <= area.right() - frame.right() ? Qt::LeftEdge
                                                                          : Qt::RightEdge;
    }
    return frame.top() - area.top() <= area.bottom() - frame.bottom() ? Qt::TopEdge
                                                                      : Qt::BottomEdge;
}

void Toolbar::updateHandle() {
    // The arrow points away from the edge: that is where the toolbar comes back from.
    const bool horizontalEdge = m_edge == Qt::TopEdge || m_edge == Qt::BottomEdge;
    const int thickness = std::max(14, m_metrics.buttonSize / 2);
    const int length = m_metrics.buttonSize * 3 / 2;
    m_handle->setFixedSize(horizontalEdge ? length : thickness,
                           horizontalEdge ? thickness : length);
    m_handle->setIconSize(QSize(m_metrics.iconSize, m_metrics.iconSize));

    const int rotation = m_edge == Qt::RightEdge  ? 0
                         : m_edge == Qt::LeftEdge ? 180
                         : m_edge == Qt::TopEdge  ? 270
                                                  : 90;
    QPixmap pixmap =
        icons::icon(IconId::Collapse).pixmap(QSize(m_metrics.iconSize, m_metrics.iconSize));
    if (rotation != 0) {
        pixmap = pixmap.transformed(QTransform().rotate(rotation), Qt::SmoothTransformation);
    }
    m_handle->setIcon(QIcon(pixmap));
}

void Toolbar::moveCollapsedToEdge() {
    const QScreen* screen = QGuiApplication::screenAt(m_expandedPos + QPoint(1, 1));
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen) {
        return;
    }
    const QRect area = screen->availableGeometry();
    const QRect was(m_expandedPos, m_body->sizeHint());
    switch (m_edge) {
    case Qt::LeftEdge:
        move(area.left(), was.center().y() - height() / 2);
        break;
    case Qt::RightEdge:
        move(area.right() + 1 - width(), was.center().y() - height() / 2);
        break;
    case Qt::TopEdge:
        move(was.center().x() - width() / 2, area.top());
        break;
    case Qt::BottomEdge:
        move(was.center().x() - width() / 2, area.bottom() + 1 - height());
        break;
    }
    keepOnScreen();
}

void Toolbar::keepOnScreen() {
    const QRect frame(pos(), size());
    const QScreen* screen = QGuiApplication::screenAt(frame.center());
    if (!screen) {
        screen = QGuiApplication::screenAt(frame.topLeft());
    }
    if (!screen) {
        return;
    }
    const QRect area = screen->availableGeometry();
    move(
        std::clamp(frame.x(), area.left(), std::max(area.left(), area.right() + 1 - frame.width())),
        std::clamp(frame.y(), area.top(),
                   std::max(area.top(), area.bottom() + 1 - frame.height())));
}

void Toolbar::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(QPen(QColor(255, 255, 255, 40), 1));
    painter.setBrush(QColor(32, 33, 36, 235));
    painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), kCornerRadius,
                            kCornerRadius);

    if (m_collapsed) {
        return; // the tab is just the arrow
    }
    // Grip dots hint that the palette can be dragged.
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 90));
    const qreal middle = (m_vertical ? width() : height()) / 2.0;
    for (int i = -1; i <= 1; ++i) {
        const QPointF dot = m_vertical ? QPointF(middle + i * 7.0, kGripThickness / 2.0)
                                       : QPointF(kGripThickness / 2.0, middle + i * 7.0);
        painter.drawEllipse(dot, 1.6, 1.6);
    }
}

bool Toolbar::eventFilter(QObject* watched, QEvent* event) {
    if (watched != m_handle) {
        return QWidget::eventFilter(watched, event);
    }
    auto* mouse = dynamic_cast<QMouseEvent*>(event);
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        if (mouse && mouse->button() == Qt::LeftButton) {
            m_handlePressPos = mouse->globalPosition().toPoint();
            m_handleDragging = false;
        }
        return true;
    case QEvent::MouseMove:
        if (mouse && !m_handleDragging && windowHandle() &&
            (mouse->globalPosition().toPoint() - m_handlePressPos).manhattanLength() >=
                QApplication::startDragDistance()) {
            m_handleDragging = true; // from here the window manager moves the window
            m_collapsedMoved = true; // expand where the user leaves it, not where it was
            windowHandle()->startSystemMove();
        }
        return true;
    case QEvent::MouseButtonRelease:
        if (!m_handleDragging) {
            setCollapsed(false);
        }
        m_handleDragging = false;
        return true;
    default:
        return QWidget::eventFilter(watched, event);
    }
}

void Toolbar::keyPressEvent(QKeyEvent* event) {
    // Clicking a button leaves the focus here, so the overlay would never see these keys.
    if (!(event->modifiers() & ~Qt::KeyboardModifiers(Qt::KeypadModifier))) {
        for (const ToolKey& entry : kToolKeys) {
            if (event->key() == entry.key) {
                m_tools.setCurrentTool(entry.kind);
                emit toolPicked(); // draw mode, and the focus goes back to the overlay
                syncToolSelection();
                return;
            }
        }
    }
    QWidget::keyPressEvent(event);
}

void Toolbar::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        emit minimizedChanged(isMinimized());
    }
    QWidget::changeEvent(event);
}

void Toolbar::closeEvent(QCloseEvent* event) {
    // Closed by the user from outside (taskbar "Close window", Alt+F4): that means quitting.
    // Closes made by Qt itself while quitting are not spontaneous.
    if (event->spontaneous()) {
        event->ignore();
        emit quitRequested();
        return;
    }
    QWidget::closeEvent(event);
}

void Toolbar::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && windowHandle()) {
        windowHandle()->startSystemMove();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

} // namespace recrayon
