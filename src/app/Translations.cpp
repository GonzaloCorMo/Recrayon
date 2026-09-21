#include "app/Translations.h"

#include <QCoreApplication>
#include <QLocale>
#include <QLoggingCategory>
#include <QTranslator>

Q_LOGGING_CATEGORY(lcI18n, "recrayon.i18n")

namespace recrayon::i18n {

namespace {

/// Installs ":/i18n/<name>_<language>.qm" if it exists (the translator is owned by @p app).
void load(QCoreApplication& app, const QString& name, const QString& language) {
    auto* translator = new QTranslator(&app);
    if (translator->load(QStringLiteral(":/i18n/%1_%2.qm").arg(name, language))) {
        QCoreApplication::installTranslator(translator);
        qCDebug(lcI18n) << "Loaded" << name << language;
    } else {
        qCDebug(lcI18n) << "No translation" << name << language;
        delete translator;
    }
}

} // namespace

void install(QCoreApplication& app, const QString& language) {
    QLocale::setDefault(QLocale(language));
    if (language == QLatin1String("en")) {
        return; // source language
    }
    load(app, QStringLiteral("qtbase"), language);
    load(app, QStringLiteral("qtmultimedia"), language);
    load(app, QStringLiteral("recrayon"), language);
}

} // namespace recrayon::i18n
