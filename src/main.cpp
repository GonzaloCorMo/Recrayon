#include "app/Application.h"
#include "app/Translations.h"
#include "config/Settings.h"
#include "recrayon/Version.h"

#include <QApplication>
#include <QLocale>
#include <QSettings>

int main(int argc, char* argv[]) {
#if defined(Q_OS_LINUX)
    // Wayland does not let clients keep a window above all others, place it on a given screen or
    // reliably make it click-through. XWayland does, so prefer it unless the user chose a
    // platform explicitly.
    if (qEnvironmentVariable("XDG_SESSION_TYPE") == QLatin1String("wayland") &&
        !qEnvironmentVariableIsSet("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "xcb");
    }
#endif

    QApplication qtApp(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Recrayon"));
    QApplication::setApplicationDisplayName(QStringLiteral("Recrayon"));
    QApplication::setApplicationVersion(QStringLiteral(RECRAYON_VERSION));
    QApplication::setOrganizationName(QStringLiteral("Recrayon"));
    // Matches packaging/linux/recrayon.desktop: desktops use it for the window icon and name.
    QApplication::setDesktopFileName(QStringLiteral("recrayon"));
    // The app lives in the tray; hiding the toolbar must not quit it.
    QApplication::setQuitOnLastWindowClosed(false);

    // Before any widget exists: texts are read when widgets are built.
    {
        QSettings store;
        const QString chosen = recrayon::Settings::load(store).language;
        recrayon::i18n::install(
            qtApp, recrayon::effectiveLanguage(chosen, QLocale::system().uiLanguages()));
    }

    recrayon::Application app;
    app.start();

    return QApplication::exec();
}
