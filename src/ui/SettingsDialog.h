#pragma once

#include "config/Settings.h"

#include <QDialog>

#include <array>

class QCheckBox;
class QComboBox;
class QFontComboBox;
class QLineEdit;
class QLabel;
class QListWidget;
class QSpinBox;
class QToolButton;
class QKeySequenceEdit;

namespace recrayon {

/// Preferences window: global shortcuts and capture options. Works on a copy of the settings;
/// the caller reads settings() after the dialog is accepted.
class SettingsDialog final : public QDialog {
    Q_OBJECT

public:
    /// An audio input the user can pick (QAudioDevice data, kept Multimedia-free here).
    struct Microphone {
        QString id;
        QString name;
    };

    SettingsDialog(const Settings& settings, bool recordingAvailable,
                   const QList<Microphone>& microphones, QWidget* parent = nullptr);

    [[nodiscard]] Settings settings() const;

    /// Validates (no duplicated or unsafe shortcuts) before closing.
    void accept() override;

    /// Human-readable names, shared with the toolbar and tray menus.
    [[nodiscard]] static QString shortcutLabel(ShortcutId id);
    [[nodiscard]] static QString targetLabel(ScreenshotTarget target);
    [[nodiscard]] static QString targetLabel(RecordingTarget target);

private:
    [[nodiscard]] QWidget* createGeneralPage();
    [[nodiscard]] QWidget* createShortcutsPage();
    [[nodiscard]] QWidget* createCapturePage(bool recordingAvailable,
                                             const QList<Microphone>& microphones);
    [[nodiscard]] QWidget* createDrawingPage();
    [[nodiscard]] QWidget* createToolbarPage();
    void refreshColorButtons();
    /// Fills the toolbar item list in @p order; items in @p hidden are unchecked.
    void fillToolbarItems(const QStringList& order, const QStringList& hidden);
    [[nodiscard]] QStringList listedToolbarItems() const;
    [[nodiscard]] QStringList uncheckedToolbarItems() const;
    void moveToolbarItem(int offset);
    void updateLanesLabel();
    void load(const Settings& settings);

    std::array<QKeySequenceEdit*, kShortcutCount> m_shortcutEdits{};
    QCheckBox* m_showCursor = nullptr;
    QComboBox* m_screenshotTarget = nullptr;
    QComboBox* m_recordingTarget = nullptr;
    QComboBox* m_videoPreset = nullptr;
    QComboBox* m_language = nullptr;
    QCheckBox* m_recordMicrophone = nullptr;
    QComboBox* m_microphone = nullptr;
    QLineEdit* m_screenshotDir = nullptr;
    QLineEdit* m_recordingDir = nullptr;

    QFontComboBox* m_textFont = nullptr;
    QSpinBox* m_textSize = nullptr;
    std::array<QColor, kPaletteSize> m_palette{};
    std::array<QToolButton*, kPaletteSize> m_paletteButtons{};
    QComboBox* m_initialColor = nullptr;
    QColor m_whiteboardColor;
    QToolButton* m_whiteboardButton = nullptr;
    QComboBox* m_whiteboardScope = nullptr;
    QListWidget* m_returnToCursor = nullptr;
    QComboBox* m_replaySpeed = nullptr;
    QListWidget* m_toolbarItems = nullptr;
    QComboBox* m_toolbarSize = nullptr;
    QComboBox* m_toolbarOrientation = nullptr;
    QSpinBox* m_toolbarLanes = nullptr;
    QLabel* m_toolbarLanesLabel = nullptr;
};

} // namespace recrayon
