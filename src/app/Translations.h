#pragma once

#include <QString>

class QCoreApplication;

namespace recrayon::i18n {

/// Loads the translations for @p language ("es", "en", ...; see kSupportedLanguages): the
/// application's own strings and Qt's (standard dialogs and buttons). Both are embedded as
/// resources under ":/i18n". English is the source language, so it needs no file.
///
/// Must run before any widget is created: texts are read when widgets are built, so changing
/// the language later needs a restart.
void install(QCoreApplication& app, const QString& language);

} // namespace recrayon::i18n
