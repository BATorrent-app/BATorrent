// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/platform/translator.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

Translator &Translator::instance()
{
    static Translator t;
    return t;
}

Translator::Translator()
{
    loadLanguage("en", m_englishFallback);
    m_strings = m_englishFallback;
}

void Translator::applySaved()
{
    QSettings s;
    int lang = 0;
    if (s.contains("language")) {
        lang = s.value("language").toInt();
    } else {
        const QString sys = QLocale::system().name().toLower();
        if      (sys.startsWith("pt")) lang = 1;
        else if (sys.startsWith("zh")) lang = 2;
        else if (sys.startsWith("ja")) lang = 3;
        else if (sys.startsWith("ru")) lang = 4;
        else if (sys.startsWith("es")) lang = 5;
        else if (sys.startsWith("de")) lang = 6;
        else if (sys.startsWith("uk")) lang = 7;
        else if (sys.startsWith("tr")) lang = 8;
        else                           lang = 0;
    }
    setLanguage(static_cast<Language>(lang));
}

void Translator::setLanguage(Language lang)
{
    m_lang = lang;
    static const char *codes[] = {"en", "pt", "zh", "ja", "ru", "es", "de", "uk", "tr"};
    m_strings.clear();
    loadLanguage(codes[static_cast<int>(lang)], m_strings);

    // installTranslator/removeTranslator are QCoreApplication members: use the
    // base instance directly so we don't downcast qApp to QApplication (UB when
    // the app is a plain QCoreApplication, e.g. under the test harness).
    QCoreApplication *coreApp = QCoreApplication::instance();
    static QTranslator *qtTr = nullptr;
    if (qtTr) { coreApp->removeTranslator(qtTr); delete qtTr; }
    qtTr = new QTranslator;
    static const QMap<Language, QString> qtLocales = {
        {Portuguese, "pt_BR"}, {Chinese, "zh_CN"}, {Japanese, "ja"},
        {Russian, "ru"}, {Spanish, "es"}, {German, "de"}, {Ukrainian, "uk"},
        {Turkish, "tr"},
    };
    const QString locale = qtLocales.value(lang, "en");
    if (qtTr->load("qt_" + locale,
            QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        coreApp->installTranslator(qtTr);
    }
}

QString Translator::tr(const QString &key) const
{
    auto it = m_strings.constFind(key);
    if (it != m_strings.constEnd()) return it.value();
    auto fb = m_englishFallback.constFind(key);
    if (fb != m_englishFallback.constEnd()) return fb.value();
    return key;
}

void Translator::loadLanguage(const QString &code, QMap<QString, QString> &map)
{
    QFile f(QString(":/translations/%1.json").arg(code));
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
    for (auto it = obj.begin(); it != obj.end(); ++it)
        map[it.key()] = it.value().toString();
}
