// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <QString>
#include <QMap>

class Translator
{
public:
    enum Language { English, Portuguese, Chinese, Japanese, Russian, Spanish, German };

    static Translator &instance();

    void setLanguage(Language lang);
    Language language() const { return m_lang; }
    QString tr(const QString &key) const;

private:
    Translator();
    void loadEnglish();
    void loadPortuguese();
    void loadChinese();
    void loadJapanese();
    void loadRussian();
    void loadSpanish();
    void loadGerman();

    Language m_lang = English;
    QMap<QString, QString> m_strings;
};

// Shortcut
inline QString tr_(const QString &key) { return Translator::instance().tr(key); }

#endif
