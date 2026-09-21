#include "ui/SettingsDialog.h"

#include "tools/ToolKind.h"
#include "ui/Icons.h"
#include "ui/Toolbar.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>

namespace recrayon {

namespace {

template <typename Enum> void selectData(QComboBox* combo, Enum value) {
    combo->setCurrentIndex(combo->findData(static_cast<int>(value)));
}

template <typename Enum> Enum selectedData(const QComboBox* combo) {
    return static_cast<Enum>(combo->currentData().toInt());
}

QLabel* makeHint(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setEnabled(false); // greyed-out secondary text
    return label;
}

/// Folder field: empty means @p defaultPath (shown as placeholder). "Browse…" picks a folder,
/// "Open" shows the folder currently in use in the file manager.
QLayout* makeFolderRow(QLineEdit* edit, const QString& defaultPath, const QString& title,
                       QWidget* parent) {
    edit->setPlaceholderText(QDir::toNativeSeparators(defaultPath));
    edit->setClearButtonEnabled(true);
    edit->setToolTip(SettingsDialog::tr("Leave empty to use the default folder"));
    edit->setMinimumWidth(280);

    const auto folderInUse = [edit, defaultPath] {
        const QString text = edit->text().trimmed();
        return text.isEmpty() ? defaultPath : QDir::fromNativeSeparators(text);
    };
    auto* browse = new QPushButton(SettingsDialog::tr("Browse…"), parent);
    QObject::connect(browse, &QPushButton::clicked, parent, [edit, folderInUse, title, parent] {
        const QString chosen = QFileDialog::getExistingDirectory(parent, title, folderInUse());
        if (!chosen.isEmpty()) {
            edit->setText(QDir::toNativeSeparators(chosen));
        }
    });
    auto* open = new QPushButton(SettingsDialog::tr("Open"), parent);
    open->setToolTip(SettingsDialog::tr("Show this folder in the file manager"));
    QObject::connect(open, &QPushButton::clicked, parent, [folderInUse] {
        const QString folder = folderInUse();
        QDir().mkpath(folder);
        QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
    });
    // A disabled field (e.g. no recording support) disables the whole row.
    browse->setEnabled(edit->isEnabled());
    open->setEnabled(edit->isEnabled());

    auto* row = new QHBoxLayout;
    row->addWidget(edit, 1);
    row->addWidget(browse);
    row->addWidget(open);
    return row;
}

/// Name of a UI language in that language.
QString languageName(const QString& code) {
    if (code == QLatin1String("en")) {
        return QStringLiteral("English");
    }
    if (code == QLatin1String("es")) {
        return QStringLiteral("Español");
    }
    QString name = QLocale(code).nativeLanguageName();
    if (!name.isEmpty()) {
        name[0] = name[0].toUpper();
    }
    return name;
}

/// The folder typed in @p edit, "" for the default one.
QString folderValue(const QLineEdit* edit) {
    const QString text = edit->text().trimmed();
    return text.isEmpty() ? QString() : QDir::cleanPath(QDir::fromNativeSeparators(text));
}

} // namespace

SettingsDialog::SettingsDialog(const Settings& settings, bool recordingAvailable,
                               const QList<Microphone>& microphones, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Settings")); // Qt appends " - Recrayon" (application display name)
    // Top-most, like the rest of the app, so the overlays never cover it.
    setWindowFlag(Qt::WindowStaysOnTopHint);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    auto* tabs = new QTabWidget(this);
    tabs->addTab(createGeneralPage(), tr("General"));
    tabs->addTab(createShortcutsPage(), tr("Shortcuts"));
    tabs->addTab(createCapturePage(recordingAvailable, microphones), tr("Capture"));
    tabs->addTab(createDrawingPage(), tr("Drawing"));
    tabs->addTab(createToolbarPage(), tr("Toolbar"));

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    connect(buttons->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this,
            [this] { load(Settings{}); });

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(tabs);
    layout->addWidget(buttons);

    load(settings);
    resize(sizeHint().width() + 80, sizeHint().height());
}

QWidget* SettingsDialog::createGeneralPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    auto* form = new QFormLayout;

    m_language = new QComboBox(page);
    m_language->addItem(tr("Automatic (system language)"), QString());
    for (const QString& code : kSupportedLanguages) {
        // Each language in its own words, so it can be found whatever the current one is.
        // Qt's native names include the country ("American English"), hence the table.
        m_language->addItem(languageName(code), code);
    }
    form->addRow(tr("Language:"), m_language);
    layout->addLayout(form);
    layout->addWidget(makeHint(tr("A new language is applied when Recrayon restarts."), page));
    layout->addStretch();
    return page;
}

QWidget* SettingsDialog::createShortcutsPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->addWidget(makeHint(tr("Global shortcuts work even while another application has the "
                                  "focus. Click a field and press the new combination; clear it "
                                  "to disable the shortcut."),
                               page));

    auto* form = new QFormLayout;
    for (const ShortcutId id : kAllShortcuts) {
        auto* edit = new QKeySequenceEdit(page);
        edit->setMaximumSequenceLength(1);
        edit->setClearButtonEnabled(true);
        m_shortcutEdits[static_cast<std::size_t>(id)] = edit;
        form->addRow(shortcutLabel(id), edit);
    }
    layout->addLayout(form);
    layout->addWidget(makeHint(tr("Inside draw mode: Esc leaves it, 1–9 pick a tool, Ctrl+Z / "
                                  "Ctrl+Y undo and redo."),
                               page));
    layout->addStretch();
    return page;
}

QWidget* SettingsDialog::createCapturePage(bool recordingAvailable,
                                           const QList<Microphone>& microphones) {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);

    m_showCursor = new QCheckBox(tr("Show the mouse cursor in screenshots and recordings"), page);
    layout->addWidget(m_showCursor);

    auto* form = new QFormLayout;
    m_screenshotTarget = new QComboBox(page);
    for (const ScreenshotTarget target :
         {ScreenshotTarget::ScreenUnderCursor, ScreenshotTarget::AllScreens,
          ScreenshotTarget::RegionOrWindow}) {
        m_screenshotTarget->addItem(targetLabel(target), static_cast<int>(target));
    }
    form->addRow(tr("Screenshot shortcut captures:"), m_screenshotTarget);

    m_recordingTarget = new QComboBox(page);
    for (const RecordingTarget target :
         {RecordingTarget::ScreenUnderCursor, RecordingTarget::FollowCursor,
          RecordingTarget::AllScreensSeparate, RecordingTarget::AllScreensCombined,
          RecordingTarget::RegionOrWindow}) {
        m_recordingTarget->addItem(targetLabel(target), static_cast<int>(target));
    }
    m_recordingTarget->setEnabled(recordingAvailable);
    form->addRow(tr("Recording shortcut records:"), m_recordingTarget);

    m_videoPreset = new QComboBox(page);
    m_videoPreset->addItem(tr("Maximum: best quality, bigger files"),
                           static_cast<int>(VideoPreset::Maximum));
    m_videoPreset->addItem(tr("High: sharp text, 30 fps"), static_cast<int>(VideoPreset::High));
    m_videoPreset->addItem(tr("Compact: smaller files (75 % size)"),
                           static_cast<int>(VideoPreset::Compact));
    m_videoPreset->setToolTip(
        tr("High suits tutorials and code. Maximum keeps more detail in busy scenes (files about "
           "30 % bigger). Compact files are about a third of the size (slightly softer text), "
           "good for sharing."));
    m_videoPreset->setEnabled(recordingAvailable);
    form->addRow(tr("Video quality:"), m_videoPreset);

    m_recordMicrophone = new QCheckBox(tr("Record audio from:"), page);
    m_microphone = new QComboBox(page);
    m_microphone->addItem(tr("The system default microphone"), QString());
    for (const Microphone& microphone : microphones) {
        m_microphone->addItem(microphone.name, microphone.id);
    }
    m_recordMicrophone->setEnabled(recordingAvailable);
    m_microphone->setEnabled(false);
    connect(m_recordMicrophone, &QCheckBox::toggled, m_microphone, &QWidget::setEnabled);
    form->addRow(m_recordMicrophone, m_microphone);
    if (recordingAvailable && microphones.isEmpty()) {
        form->addRow(QString(), makeHint(tr("No microphone found."), page));
    }

    m_screenshotDir = new QLineEdit(page);
    form->addRow(tr("Save screenshots in:"),
                 makeFolderRow(m_screenshotDir, Settings::defaultScreenshotDirectory(),
                               tr("Folder for screenshots"), page));
    m_recordingDir = new QLineEdit(page);
    m_recordingDir->setEnabled(recordingAvailable);
    form->addRow(tr("Save videos in:"),
                 makeFolderRow(m_recordingDir, Settings::defaultRecordingDirectory(),
                               tr("Folder for videos"), page));
    layout->addLayout(form);
    layout->addWidget(makeHint(tr("Other targets are always one click away: right-click (or "
                                  "press and hold) the screenshot and record buttons of the "
                                  "toolbar, or use the tray menu."),
                               page));

    if (!recordingAvailable) {
        layout->addWidget(
            makeHint(tr("Recording is unavailable: this build has no Qt Multimedia."), page));
    }
    layout->addWidget(makeHint(tr("Screenshots are also copied to the clipboard. Leave a folder "
                                  "empty to use the default one. While recording, a badge with "
                                  "the elapsed time, pause and stop stays on screen (it is not "
                                  "recorded). With several separate screens, the audio goes into "
                                  "the first video."),
                               page));
    layout->addStretch();
    return page;
}

QWidget* SettingsDialog::createDrawingPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    auto* form = new QFormLayout;

    m_textFont = new QFontComboBox(page);
    m_textFont->setWritingSystem(QFontDatabase::Latin);
    m_textSize = new QSpinBox(page);
    m_textSize->setRange(8, 200);
    m_textSize->setSuffix(tr(" px"));
    auto* fontRow = new QHBoxLayout;
    fontRow->addWidget(m_textFont, 1);
    fontRow->addWidget(m_textSize);
    form->addRow(tr("Text font:"), fontRow);

    auto* paletteRow = new QHBoxLayout;
    for (std::size_t i = 0; i < kPaletteSize; ++i) {
        auto* button = new QToolButton(page);
        button->setIconSize(QSize(22, 22));
        button->setToolTip(tr("Click to change this color"));
        connect(button, &QToolButton::clicked, this, [this, i] {
            const QColor chosen =
                QColorDialog::getColor(m_palette[i], this, tr("Palette color %1").arg(i + 1));
            if (chosen.isValid()) {
                m_palette[i] = chosen;
                refreshColorButtons();
            }
        });
        m_paletteButtons[i] = button;
        paletteRow->addWidget(button);
    }
    paletteRow->addStretch();
    form->addRow(tr("Toolbar colors:"), paletteRow);

    m_initialColor = new QComboBox(page);
    for (std::size_t i = 0; i < kPaletteSize; ++i) {
        m_initialColor->addItem(tr("Color %1").arg(i + 1), static_cast<int>(i));
    }
    form->addRow(tr("Color at start-up:"), m_initialColor);

    m_whiteboardButton = new QToolButton(page);
    m_whiteboardButton->setIconSize(QSize(22, 22));
    connect(m_whiteboardButton, &QToolButton::clicked, this, [this] {
        const QColor chosen =
            QColorDialog::getColor(m_whiteboardColor, this, tr("Whiteboard color"));
        if (chosen.isValid()) {
            m_whiteboardColor = chosen;
            refreshColorButtons();
        }
    });
    form->addRow(tr("Whiteboard background:"), m_whiteboardButton);

    m_whiteboardScope = new QComboBox(page);
    m_whiteboardScope->addItem(tr("The screen under the pointer"),
                               static_cast<int>(WhiteboardScope::ScreenUnderCursor));
    m_whiteboardScope->addItem(tr("All screens"), static_cast<int>(WhiteboardScope::AllScreens));
    form->addRow(tr("Whiteboard covers:"), m_whiteboardScope);

    m_replaySpeed = new QComboBox(page);
    m_replaySpeed->addItem(tr("Slow"), static_cast<int>(ReplaySpeed::Slow));
    m_replaySpeed->addItem(tr("Normal"), static_cast<int>(ReplaySpeed::Normal));
    m_replaySpeed->addItem(tr("Fast"), static_cast<int>(ReplaySpeed::Fast));
    form->addRow(tr("Replay speed:"), m_replaySpeed);

    // Drawing tools that hand control back to the mouse once an element is placed.
    m_returnToCursor = new QListWidget(page);
    m_returnToCursor->setIconSize(QSize(18, 18));
    const auto catalog = Toolbar::itemCatalog(palette().color(QPalette::Text));
    for (const ToolKind kind : kAllTools) {
        if (!isDrawingTool(kind)) {
            continue;
        }
        const QString id = QString::fromLatin1(toolId(kind));
        const auto info = std::find_if(catalog.cbegin(), catalog.cend(), [&id](const auto& item) {
            return item.id == QStringLiteral("tool.") + id;
        });
        auto* item = new QListWidgetItem(m_returnToCursor);
        if (info != catalog.cend()) {
            item->setIcon(info->icon);
            item->setText(info->label);
        }
        item->setData(Qt::UserRole, id);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    }
    m_returnToCursor->setMaximumHeight(140);
    form->addRow(tr("After placing, go back\nto the mouse with:"), m_returnToCursor);

    layout->addLayout(form);
    layout->addWidget(makeHint(tr("The font is used by the Text and Arrow-with-text tools. The "
                                  "whiteboard is a separate page: your desktop annotations come "
                                  "back when you leave it. Right-click the whiteboard button to "
                                  "choose the screens just once."),
                               page));
    layout->addStretch();
    return page;
}

QWidget* SettingsDialog::createToolbarPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);

    auto* form = new QFormLayout;
    m_toolbarSize = new QComboBox(page);
    m_toolbarSize->addItem(tr("Small"), static_cast<int>(ToolbarSize::Small));
    m_toolbarSize->addItem(tr("Normal"), static_cast<int>(ToolbarSize::Normal));
    m_toolbarSize->addItem(tr("Large"), static_cast<int>(ToolbarSize::Large));
    form->addRow(tr("Button size:"), m_toolbarSize);

    m_toolbarOrientation = new QComboBox(page);
    m_toolbarOrientation->addItem(tr("Vertical"), static_cast<int>(ToolbarOrientation::Vertical));
    m_toolbarOrientation->addItem(tr("Horizontal"),
                                  static_cast<int>(ToolbarOrientation::Horizontal));
    form->addRow(tr("Orientation:"), m_toolbarOrientation);

    m_toolbarLanes = new QSpinBox(page);
    m_toolbarLanes->setRange(kMinToolbarLanes, kMaxToolbarLanes);
    m_toolbarLanesLabel = new QLabel(page);
    form->addRow(m_toolbarLanesLabel, m_toolbarLanes);
    connect(m_toolbarOrientation, &QComboBox::currentIndexChanged, this,
            &SettingsDialog::updateLanesLabel);
    layout->addLayout(form);

    layout->addWidget(makeHint(tr("Check what the toolbar shows and drag the items (or use the "
                                  "buttons) to change their order. Items of the same kind next "
                                  "to each other share a group. The settings button is always "
                                  "there; everything stays available from the tray menu and "
                                  "the shortcuts."),
                               page));

    auto* listRow = new QHBoxLayout;
    m_toolbarItems = new QListWidget(page);
    m_toolbarItems->setIconSize(QSize(20, 20));
    m_toolbarItems->setDragDropMode(QAbstractItemView::InternalMove);
    m_toolbarItems->setDefaultDropAction(Qt::MoveAction);
    listRow->addWidget(m_toolbarItems, 1);

    auto* buttons = new QVBoxLayout;
    auto* up = new QPushButton(tr("Move up"), page);
    auto* down = new QPushButton(tr("Move down"), page);
    auto* reset = new QPushButton(tr("Default order"), page);
    connect(up, &QPushButton::clicked, this, [this] { moveToolbarItem(-1); });
    connect(down, &QPushButton::clicked, this, [this] { moveToolbarItem(+1); });
    connect(reset, &QPushButton::clicked, this,
            [this] { fillToolbarItems(Toolbar::defaultItemOrder(), uncheckedToolbarItems()); });
    buttons->addWidget(up);
    buttons->addWidget(down);
    buttons->addStretch();
    buttons->addWidget(reset);
    listRow->addLayout(buttons);
    layout->addLayout(listRow, 1);
    return page;
}

void SettingsDialog::fillToolbarItems(const QStringList& order, const QStringList& hidden) {
    const auto catalog = Toolbar::itemCatalog(palette().color(QPalette::Text));
    m_toolbarItems->clear();
    for (const QString& id : order) {
        const auto info =
            std::find_if(catalog.cbegin(), catalog.cend(),
                         [&id](const Toolbar::ItemInfo& entry) { return entry.id == id; });
        if (info == catalog.cend()) {
            continue;
        }
        auto* item = new QListWidgetItem(info->icon, info->label, m_toolbarItems);
        item->setData(Qt::UserRole, info->id);
        // Dropping onto an item would replace it: items only go between others.
        item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);
        if (info->hideable) {
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(hidden.contains(id) ? Qt::Unchecked : Qt::Checked);
        } else {
            item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked);
            item->setToolTip(tr("Always shown"));
        }
    }
}

QStringList SettingsDialog::listedToolbarItems() const {
    QStringList ids;
    for (int row = 0; row < m_toolbarItems->count(); ++row) {
        ids << m_toolbarItems->item(row)->data(Qt::UserRole).toString();
    }
    return ids;
}

QStringList SettingsDialog::uncheckedToolbarItems() const {
    QStringList ids;
    for (int row = 0; row < m_toolbarItems->count(); ++row) {
        const QListWidgetItem* item = m_toolbarItems->item(row);
        if (item->checkState() != Qt::Checked) {
            ids << item->data(Qt::UserRole).toString();
        }
    }
    ids.sort();
    return ids;
}

void SettingsDialog::moveToolbarItem(int offset) {
    const int row = m_toolbarItems->currentRow();
    const int target = row + offset;
    if (row < 0 || target < 0 || target >= m_toolbarItems->count()) {
        return;
    }
    QListWidgetItem* item = m_toolbarItems->takeItem(row);
    m_toolbarItems->insertItem(target, item);
    m_toolbarItems->setCurrentItem(item);
}

void SettingsDialog::updateLanesLabel() {
    const bool vertical =
        selectedData<ToolbarOrientation>(m_toolbarOrientation) == ToolbarOrientation::Vertical;
    m_toolbarLanesLabel->setText(vertical ? tr("Columns:") : tr("Rows:"));
}

void SettingsDialog::refreshColorButtons() {
    for (std::size_t i = 0; i < kPaletteSize; ++i) {
        m_paletteButtons[i]->setIcon(icons::swatch(m_palette[i]));
        m_initialColor->setItemIcon(static_cast<int>(i), icons::swatch(m_palette[i]));
    }
    m_whiteboardButton->setIcon(icons::swatch(m_whiteboardColor));
    m_whiteboardButton->setToolTip(m_whiteboardColor.name());
}

void SettingsDialog::load(const Settings& settings) {
    for (const ShortcutId id : kAllShortcuts) {
        m_shortcutEdits[static_cast<std::size_t>(id)]->setKeySequence(settings.shortcut(id));
    }
    m_language->setCurrentIndex(std::max(0, m_language->findData(settings.language)));
    m_showCursor->setChecked(settings.showCursorInCaptures);
    m_recordMicrophone->setChecked(settings.recordMicrophone);
    m_microphone->setEnabled(settings.recordMicrophone && m_recordMicrophone->isEnabled());
    // A microphone that is not connected now keeps its id until the user picks another one.
    int microphoneIndex = m_microphone->findData(settings.microphoneId);
    if (microphoneIndex < 0) {
        m_microphone->addItem(tr("%1 (not connected)").arg(settings.microphoneId),
                              settings.microphoneId);
        microphoneIndex = m_microphone->count() - 1;
    }
    m_microphone->setCurrentIndex(microphoneIndex);
    selectData(m_screenshotTarget, settings.screenshotTarget);
    selectData(m_recordingTarget, settings.recordingTarget);
    selectData(m_videoPreset, settings.videoPreset);
    m_screenshotDir->setText(QDir::toNativeSeparators(settings.screenshotDirectory));
    m_recordingDir->setText(QDir::toNativeSeparators(settings.recordingDirectory));

    const QString family = settings.textFontFamily.isEmpty()
                               ? QFontDatabase::systemFont(QFontDatabase::GeneralFont).family()
                               : settings.textFontFamily;
    m_textFont->setCurrentFont(QFont(family));
    m_textSize->setValue(settings.textPixelSize);
    m_palette = settings.palette;
    m_initialColor->setCurrentIndex(settings.initialColorIndex);
    m_whiteboardColor = settings.whiteboardColor;
    selectData(m_whiteboardScope, settings.whiteboardScope);
    selectData(m_replaySpeed, settings.replaySpeed);
    for (int row = 0; row < m_returnToCursor->count(); ++row) {
        QListWidgetItem* item = m_returnToCursor->item(row);
        item->setCheckState(
            settings.returnToCursorTools.contains(item->data(Qt::UserRole).toString())
                ? Qt::Checked
                : Qt::Unchecked);
    }
    refreshColorButtons();

    selectData(m_toolbarSize, settings.toolbarSize);
    selectData(m_toolbarOrientation, settings.toolbarOrientation);
    m_toolbarLanes->setValue(settings.toolbarLanes);
    updateLanesLabel();
    fillToolbarItems(orderedToolbarItems(Toolbar::defaultItemOrder(), settings.toolbarOrder),
                     settings.hiddenToolbarItems);
}

Settings SettingsDialog::settings() const {
    Settings result;
    for (const ShortcutId id : kAllShortcuts) {
        result.setShortcut(id, m_shortcutEdits[static_cast<std::size_t>(id)]->keySequence());
    }
    result.language = m_language->currentData().toString();
    result.showCursorInCaptures = m_showCursor->isChecked();
    result.recordMicrophone = m_recordMicrophone->isChecked();
    result.microphoneId = m_microphone->currentData().toString();
    result.screenshotTarget = selectedData<ScreenshotTarget>(m_screenshotTarget);
    result.recordingTarget = selectedData<RecordingTarget>(m_recordingTarget);
    result.videoPreset = selectedData<VideoPreset>(m_videoPreset);
    result.screenshotDirectory = folderValue(m_screenshotDir);
    result.recordingDirectory = folderValue(m_recordingDir);

    result.textFontFamily = m_textFont->currentFont().family();
    result.textPixelSize = m_textSize->value();
    result.palette = m_palette;
    result.initialColorIndex = m_initialColor->currentIndex();
    result.whiteboardColor = m_whiteboardColor;
    result.whiteboardScope = selectedData<WhiteboardScope>(m_whiteboardScope);
    result.replaySpeed = selectedData<ReplaySpeed>(m_replaySpeed);
    for (int row = 0; row < m_returnToCursor->count(); ++row) {
        const QListWidgetItem* item = m_returnToCursor->item(row);
        if (item->checkState() == Qt::Checked) {
            result.returnToCursorTools << item->data(Qt::UserRole).toString();
        }
    }
    result.returnToCursorTools.sort();
    result.hiddenToolbarItems = uncheckedToolbarItems();
    // The default order is saved as "no order", so later versions can place new items.
    const QStringList order = listedToolbarItems();
    if (order != Toolbar::defaultItemOrder()) {
        result.toolbarOrder = order;
    }
    result.toolbarSize = selectedData<ToolbarSize>(m_toolbarSize);
    result.toolbarOrientation = selectedData<ToolbarOrientation>(m_toolbarOrientation);
    result.toolbarLanes = m_toolbarLanes->value();
    return result;
}

void SettingsDialog::accept() {
    const Settings candidate = settings();

    QStringList problems;
    const auto duplicates = candidate.duplicateShortcuts();
    for (const auto& [first, second] : duplicates) {
        problems << tr("“%1” and “%2” use the same shortcut (%3).")
                        .arg(shortcutLabel(first), shortcutLabel(second),
                             candidate.shortcut(first).toString(QKeySequence::NativeText));
    }
    for (const ShortcutId id : kAllShortcuts) {
        if (!isSafeGlobalShortcut(candidate.shortcut(id))) {
            problems << tr("“%1” (%2) needs Ctrl, Alt or Win, otherwise it would block normal "
                           "typing in every application.")
                            .arg(shortcutLabel(id),
                                 candidate.shortcut(id).toString(QKeySequence::NativeText));
        }
    }

    for (const QString& folder : {candidate.screenshotDirectory, candidate.recordingDirectory}) {
        if (!folder.isEmpty() && !QDir().mkpath(folder)) {
            problems << tr("The folder “%1” does not exist and cannot be created.")
                            .arg(QDir::toNativeSeparators(folder));
        }
    }

    if (!problems.isEmpty()) {
        QMessageBox::warning(this, tr("Check the settings"), problems.join(QStringLiteral("\n\n")));
        return;
    }
    QDialog::accept();
}

QString SettingsDialog::shortcutLabel(ShortcutId id) {
    switch (id) {
    case ShortcutId::ToggleDrawing:
        return tr("Toggle draw mode");
    case ShortcutId::ToggleVisibility:
        return tr("Show / hide annotations");
    case ShortcutId::Whiteboard:
        return tr("Whiteboard");
    case ShortcutId::Clear:
        return tr("Clear all");
    case ShortcutId::Spotlight:
        return tr("Spotlight pointer");
    case ShortcutId::Halo:
        return tr("Highlight pointer (halo)");
    case ShortcutId::Screenshot:
        return tr("Screenshot");
    case ShortcutId::ScreenshotRegion:
        return tr("Screenshot of a region or window");
    case ShortcutId::Replay:
        return tr("Replay the drawing");
    case ShortcutId::Recording:
        return tr("Start / stop recording");
    case ShortcutId::RecordRegion:
        return tr("Record a region or window");
    }
    return {};
}

QString SettingsDialog::targetLabel(ScreenshotTarget target) {
    switch (target) {
    case ScreenshotTarget::ScreenUnderCursor:
        return tr("Screen under the pointer");
    case ScreenshotTarget::AllScreens:
        return tr("All screens (one image)");
    case ScreenshotTarget::RegionOrWindow:
        return tr("Region or window…");
    }
    return {};
}

QString SettingsDialog::targetLabel(RecordingTarget target) {
    switch (target) {
    case RecordingTarget::ScreenUnderCursor:
        return tr("Screen under the pointer");
    case RecordingTarget::FollowCursor:
        return tr("Follow the pointer across screens");
    case RecordingTarget::AllScreensSeparate:
        return tr("All screens (one video per screen)");
    case RecordingTarget::AllScreensCombined:
        return tr("All screens (one video)");
    case RecordingTarget::RegionOrWindow:
        return tr("Region or window…");
    }
    return {};
}

} // namespace recrayon
