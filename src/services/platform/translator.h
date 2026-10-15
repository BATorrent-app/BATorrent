// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <QString>
#include <QLocale>
#include <QMap>

class Translator
{
public:
    enum Language { English, Portuguese, Chinese, Japanese, Russian, Spanish, German, Ukrainian, Turkish };

    static Translator &instance();

    void setLanguage(Language lang);
    // The app's language as a locale, for dates and numbers: the system's
    // locale is not the one the user picked (an English Windows, a Portuguese UI).
    QLocale locale() const;
    QString code() const;   // "en", "pt", ... the app language's two letters
    // The language the user picked (QSettings "language"), or the system
    // locale's when they never picked one. Both processes call this: the UI,
    // and the engine child, which builds the state labels the UI shows.
    void applySaved();
    // The language applySaved() picks: the saved choice, else the system's.
    // Readable before it runs, which the addon seeding at startup needs.
    static Language savedLanguage();
    Language language() const { return m_lang; }
    bool languageApplied() const { return m_applied; }   // setLanguage has run
    QString tr(const QString &key) const;

private:
    Translator();
    void loadLanguage(const QString &code, QMap<QString, QString> &map);

    Language m_lang = English;
    bool m_applied = false;
    QMap<QString, QString> m_strings;
    QMap<QString, QString> m_englishFallback;
};

// Shortcut
inline QString tr_(const QString &key) { return Translator::instance().tr(key); }

#endif
