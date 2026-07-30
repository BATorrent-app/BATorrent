// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/platform/contentlanguage.h"

#include <QLocale>
#include <QSettings>

namespace {
// Absent (or out of range) means "whatever the app is set to" — the behaviour
// every existing install had before this setting existed.
const char *kKey = "contentLanguage";

bool inRange(int v)
{
    return v >= static_cast<int>(Translator::English) && v <= static_cast<int>(Translator::Turkish);
}
}

Translator::Language ContentLanguage::current()
{
    const QVariant v = QSettings().value(QString::fromLatin1(kKey));
    bool ok = false;
    const int i = v.toInt(&ok);
    if (ok && inRange(i)) return static_cast<Translator::Language>(i);
    return Translator::instance().language();
}

void ContentLanguage::set(Translator::Language lang)
{
    QSettings().setValue(QString::fromLatin1(kKey), static_cast<int>(lang));
}

void ContentLanguage::followApp()
{
    QSettings().remove(QString::fromLatin1(kKey));
}

bool ContentLanguage::followsApp()
{
    const QVariant v = QSettings().value(QString::fromLatin1(kKey));
    bool ok = false;
    const int i = v.toInt(&ok);
    return !(ok && inRange(i));
}

QString ContentLanguage::releaseTag(Translator::Language lang)
{
    switch (lang) {
    case Translator::Portuguese: return QStringLiteral("PT");
    case Translator::Spanish:    return QStringLiteral("ES");
    case Translator::German:     return QStringLiteral("DE");
    case Translator::Russian:    return QStringLiteral("RU");
    case Translator::Japanese:   return QStringLiteral("JA");
    case Translator::Chinese:    return QStringLiteral("ZH");
    case Translator::Ukrainian:  return QStringLiteral("UK");
    case Translator::Turkish:    return QStringLiteral("TR");
    case Translator::English:    break;
    }
    return QStringLiteral("EN");
}

QString ContentLanguage::tmdb(Translator::Language lang)
{
    switch (lang) {
    case Translator::Portuguese: return QStringLiteral("pt-BR");
    case Translator::Chinese:    return QStringLiteral("zh-CN");
    case Translator::Japanese:   return QStringLiteral("ja-JP");
    case Translator::Russian:    return QStringLiteral("ru-RU");
    case Translator::Spanish:    return QStringLiteral("es-ES");
    case Translator::German:     return QStringLiteral("de-DE");
    case Translator::Ukrainian:  return QStringLiteral("uk-UA");
    case Translator::Turkish:    return QStringLiteral("tr-TR");
    case Translator::English:    break;
    }
    return QStringLiteral("en-US");
}

QString ContentLanguage::subtitleCode(Translator::Language lang)
{
    switch (lang) {
    case Translator::Portuguese: return QStringLiteral("pt");
    case Translator::Chinese:    return QStringLiteral("zh");
    case Translator::Japanese:   return QStringLiteral("ja");
    case Translator::Russian:    return QStringLiteral("ru");
    case Translator::Spanish:    return QStringLiteral("es");
    case Translator::German:     return QStringLiteral("de");
    case Translator::Ukrainian:  return QStringLiteral("uk");
    case Translator::Turkish:    return QStringLiteral("tr");
    case Translator::English:    break;
    }
    return QStringLiteral("en");
}

QString ContentLanguage::releaseTag()   { return releaseTag(current()); }
QString ContentLanguage::tmdb()         { return tmdb(current()); }
QString ContentLanguage::subtitleCode() { return subtitleCode(current()); }

QString ContentLanguage::region()
{
    // An explicit content language is a deliberate statement about what the user
    // wants to watch; it outranks where the machine happens to be.
    if (followsApp()) {
        const QString sys = QLocale::system().name();       // e.g. "pt_BR"
        const int us = sys.indexOf(QLatin1Char('_'));
        if (us >= 0 && sys.size() >= us + 3) {
            const QString cc = sys.mid(us + 1, 2).toUpper();
            if (cc[0].isLetter() && cc[1].isLetter()) return cc;
        }
    }
    switch (current()) {
    case Translator::Portuguese: return QStringLiteral("BR");
    case Translator::Russian:    return QStringLiteral("RU");
    case Translator::Japanese:   return QStringLiteral("JP");
    case Translator::German:     return QStringLiteral("DE");
    case Translator::Spanish:    return QStringLiteral("ES");
    case Translator::Ukrainian:  return QStringLiteral("UA");
    case Translator::Chinese:    return QStringLiteral("CN");
    case Translator::Turkish:    return QStringLiteral("TR");
    case Translator::English:    break;
    }
    return QStringLiteral("US");
}
