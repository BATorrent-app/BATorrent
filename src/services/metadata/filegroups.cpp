// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/metadata/filegroups.h"
#include "services/metadata/nameparser.h"

#include <QMap>
#include <QRegularExpression>
#include <algorithm>

namespace FileGroups {

namespace {

QString baseName(const QString &path)
{
    const int s = std::max(path.lastIndexOf(QLatin1Char('/')), path.lastIndexOf(QLatin1Char('\\')));
    return s >= 0 ? path.mid(s + 1) : path;
}

QString lowerExt(const QString &name)
{
    const int dot = name.lastIndexOf(QLatin1Char('.'));
    return dot >= 0 ? name.mid(dot + 1).toLower() : QString();
}

bool isVideo(const QString &ext)
{
    static const QStringList v = { "mkv", "mp4", "avi", "mov", "wmv", "m4v", "ts", "m2ts", "webm" };
    return v.contains(ext);
}

// installer pieces that are tiny and always wanted
bool isInstaller(const QString &lowerName)
{
    return lowerName == QLatin1String("setup.exe") || lowerName.startsWith(QLatin1String("setup-"))
        || lowerName == QLatin1String("isdone.dll") || lowerName == QLatin1String("unarc.dll")
        || (lowerName.endsWith(QLatin1String(".dll")) && lowerName.size() < 16);
}

Chip chipFor(const File &f, const QString &label, const QString &lang = QString())
{
    Chip c;
    c.label = label;
    c.lang = lang;
    c.files = { f.index };
    c.size = f.size;
    return c;
}

void add(Group &g, const File &f) { g.files << f.index; g.size += f.size; }

QString extraLabel(const QString &token)
{
    const QString t = token.toLower();
    if (t.contains(QLatin1String("soundtrack")) || t.contains(QLatin1String("ost"))) return QStringLiteral("Soundtrack");
    if (t.contains(QLatin1String("artbook")) || t.contains(QLatin1String("art"))) return QStringLiteral("Artbook");
    if (t.contains(QLatin1String("video")) || t.contains(QLatin1String("movie"))) return QStringLiteral("Videos");
    if (t.contains(QLatin1String("bonus"))) return QStringLiteral("Bonus");
    QString s = token;
    s.replace(QLatin1Char('_'), QLatin1Char(' ')).replace(QLatin1Char('-'), QLatin1Char(' '));
    return s.isEmpty() ? s : s.left(1).toUpper() + s.mid(1);
}

// FitGirl: fg-selective-<lang>.bin, fg-optional-<what>.bin, fg-NN.bin
QList<Group> fitgirl(const QList<File> &files)
{
    static const QRegularExpression sel(QStringLiteral("^fg-selective-([a-z_-]+?)(?:-\\d+)?\\.bin$"),
                                        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression opt(QStringLiteral("^fg-optional-([a-z0-9_-]+?)(?:-\\d+)?\\.bin$"),
                                        QRegularExpression::CaseInsensitiveOption);
    Group game{ "game", "grp_game" }, langs{ "lang", "grp_lang" }, extras{ "extras", "grp_extras" },
          setup{ "setup", "grp_setup" };
    QMap<QString, Chip> langChips, extraChips;
    for (const File &f : files) {
        const QString name = baseName(f.path);
        const QString lower = name.toLower();
        if (auto m = sel.match(name); m.hasMatch()) {
            const Lang l = languageFromToken(m.captured(1));
            const QString key = l.code.isEmpty() ? m.captured(1).toLower() : l.code;
            Chip &c = langChips[key];
            if (c.label.isEmpty()) { c.label = l.label.isEmpty() ? extraLabel(m.captured(1)) : l.label; c.lang = l.code; }
            c.files << f.index; c.size += f.size;
            add(langs, f);
        } else if (auto o = opt.match(name); o.hasMatch()) {
            const QString label = extraLabel(o.captured(1));
            Chip &c = extraChips[label];
            if (c.label.isEmpty()) c.label = label;
            c.files << f.index; c.size += f.size;
            add(extras, f);
        } else if (isInstaller(lower)) {
            add(setup, f);
        } else {
            add(game, f);
        }
    }
    if (langChips.isEmpty() && extraChips.isEmpty()) return {};
    langs.chips = langChips.values();
    extras.chips = extraChips.values();
    // FitGirl ships English in the game data; say so instead of leaving it out
    if (!langChips.isEmpty() && !langChips.contains(QStringLiteral("en"))) {
        Chip en; en.label = QStringLiteral("English"); en.lang = QStringLiteral("en"); en.inBase = true;
        langs.chips.prepend(en);
    }
    QList<Group> out;
    if (!game.files.isEmpty()) out << game;
    if (!langs.files.isEmpty()) out << langs;
    if (!extras.files.isEmpty()) out << extras;
    if (!setup.files.isEmpty()) out << setup;
    return out;
}

// DODI: <Language>.doi language files, bonus / OST folders
QList<Group> dodi(const QList<File> &files)
{
    Group game{ "game", "grp_game" }, langs{ "lang", "grp_lang" }, extras{ "extras", "grp_extras" },
          setup{ "setup", "grp_setup" };
    QMap<QString, Chip> langChips, extraChips;
    for (const File &f : files) {
        const QString name = baseName(f.path);
        const QString lower = name.toLower();
        const QString lowerPath = f.path.toLower();
        const QString ext = lowerExt(name);
        if (ext == QLatin1String("doi")) {
            const QString token = name.left(name.size() - 4);
            const Lang l = languageFromToken(token);
            if (!l.code.isEmpty()) {
                Chip &c = langChips[l.code];
                if (c.label.isEmpty()) { c.label = l.label; c.lang = l.code; }
                c.files << f.index; c.size += f.size;
                add(langs, f);
                continue;
            }
        }
        if (lowerPath.contains(QLatin1String("bonus")) || lowerPath.contains(QLatin1String("soundtrack"))
            || lowerPath.contains(QLatin1String("/ost/")) || lowerPath.contains(QLatin1String("artbook"))) {
            const QString label = extraLabel(lowerPath.contains(QLatin1String("soundtrack")) || lowerPath.contains(QLatin1String("/ost/"))
                                             ? QStringLiteral("soundtrack")
                                             : lowerPath.contains(QLatin1String("artbook")) ? QStringLiteral("artbook")
                                                                                            : QStringLiteral("bonus"));
            Chip &c = extraChips[label];
            if (c.label.isEmpty()) c.label = label;
            c.files << f.index; c.size += f.size;
            add(extras, f);
        } else if (isInstaller(lower)) {
            add(setup, f);
        } else {
            add(game, f);
        }
    }
    if (langChips.isEmpty() && extraChips.isEmpty()) return {};
    langs.chips = langChips.values();
    extras.chips = extraChips.values();
    if (!langChips.isEmpty() && !langChips.contains(QStringLiteral("en"))) {
        Chip en; en.label = QStringLiteral("English"); en.lang = QStringLiteral("en"); en.inBase = true;
        langs.chips.prepend(en);
    }
    QList<Group> out;
    if (!game.files.isEmpty()) out << game;
    if (!langs.files.isEmpty()) out << langs;
    if (!extras.files.isEmpty()) out << extras;
    if (!setup.files.isEmpty()) out << setup;
    return out;
}

// a season pack: one group per season, episodes as chips
QList<Group> seasons(const QList<File> &files)
{
    QMap<int, Group> bySeason;
    int videos = 0, placed = 0;
    for (const File &f : files) {
        const QString name = baseName(f.path);
        if (!isVideo(lowerExt(name))) continue;
        ++videos;
        const ParsedName pn = NameParser::parse(name);
        if (pn.episode <= 0) continue;
        const int season = pn.season > 0 ? pn.season : 1;
        Group &g = bySeason[season];
        if (g.kind.isEmpty()) { g.kind = QStringLiteral("season"); g.season = season;
                                g.label = QStringLiteral("S%1").arg(season, 2, 10, QLatin1Char('0')); }
        g.chips << chipFor(f, QStringLiteral("E%1").arg(pn.episode, 2, 10, QLatin1Char('0')));
        add(g, f);
        ++placed;
    }
    // a pack is several episodes, and nearly all of its videos are episodes
    if (placed < 2 || placed < videos * 0.8) return {};
    QList<Group> out = bySeason.values();
    for (Group &g : out)
        std::sort(g.chips.begin(), g.chips.end(), [](const Chip &a, const Chip &b) { return a.label < b.label; });
    // whatever else came along (subtitles, extras) rides with the first season
    for (const File &f : files) {
        bool used = false;
        for (const Group &g : out) if (g.files.contains(f.index)) { used = true; break; }
        if (!used && !out.isEmpty()) add(out.first(), f);
    }
    return out.size() > 1 || (out.size() == 1 && out.first().chips.size() > 1) ? out : QList<Group>{};
}

// a film with separate subtitle and audio files
QList<Group> filmExtras(const QList<File> &files)
{
    static const QStringList subs = { "srt", "ass", "ssa", "sub", "idx", "sup", "vtt" };
    static const QStringList audio = { "mka", "ac3", "eac3", "dts", "aac", "flac" };
    Group main{ "game", "grp_main" }, side{ "subs", "grp_subs" };
    int videos = 0;
    for (const File &f : files) {
        const QString name = baseName(f.path);
        const QString ext = lowerExt(name);
        if (subs.contains(ext) || audio.contains(ext)) {
            // "Movie.2023.pt-BR.srt" → the language part as the chip
            QString label = name.left(name.size() - ext.size() - 1);
            const int dot = label.lastIndexOf(QLatin1Char('.'));
            if (dot >= 0) label = label.mid(dot + 1);
            const Lang l = languageFromToken(label);
            side.chips << chipFor(f, (l.label.isEmpty() ? label : l.label) + QStringLiteral(" · ") + ext, l.code);
            add(side, f);
        } else {
            if (isVideo(ext)) ++videos;
            add(main, f);
        }
    }
    if (videos != 1 || side.files.size() < 2) return {};
    return { main, side };
}

} // namespace

Lang languageFromToken(const QString &token)
{
    static const QList<std::pair<QStringList, Lang>> table = {
        { { "english", "eng", "en" },                                  { "en", "English" } },
        { { "brazilian", "portuguese_br", "portuguese-br", "ptbr", "pt-br", "pt_br", "brazil", "portugues_br" },
                                                                        { "pt-BR", QString::fromUtf8("Português (BR)") } },
        { { "portuguese", "portugues", "por", "pt", "pt-pt" },         { "pt-PT", QString::fromUtf8("Português (PT)") } },
        { { "spanish_latam", "latam", "spanish-latam", "es-419", "latino", "spanish_la", "mexican" },
                                                                        { "es-419", QString::fromUtf8("Español (LA)") } },
        { { "spanish", "spa", "es", "castellano", "es-es" },           { "es-ES", QString::fromUtf8("Español (ES)") } },
        { { "french", "fre", "fra", "fr" },                            { "fr", QString::fromUtf8("Français") } },
        { { "german", "ger", "deu", "de" },                            { "de", QStringLiteral("Deutsch") } },
        { { "italian", "ita", "it" },                                  { "it", QStringLiteral("Italiano") } },
        { { "russian", "rus", "ru" },                                  { "ru", QString::fromUtf8("Русский") } },
        { { "polish", "pol", "pl" },                                   { "pl", QStringLiteral("Polski") } },
        { { "japanese", "jpn", "ja", "jp" },                           { "ja", QString::fromUtf8("日本語") } },
        { { "chinese", "chinese_simplified", "schinese", "chs", "zh", "zh-cn" },
                                                                        { "zh", QString::fromUtf8("中文") } },
        { { "tchinese", "chinese_traditional", "cht", "zh-tw" },       { "zh-TW", QString::fromUtf8("中文 (繁體)") } },
        { { "korean", "kor", "ko" },                                   { "ko", QString::fromUtf8("한국어") } },
        { { "turkish", "tur", "tr" },                                  { "tr", QString::fromUtf8("Türkçe") } },
        { { "ukrainian", "ukr", "uk" },                                { "uk", QString::fromUtf8("Українська") } },
        { { "czech", "cze", "cs" },                                    { "cs", QString::fromUtf8("Čeština") } },
        { { "hungarian", "hun", "hu" },                                { "hu", QStringLiteral("Magyar") } },
        { { "dutch", "dut", "nl" },                                    { "nl", QStringLiteral("Nederlands") } },
        { { "arabic", "ara", "ar" },                                   { "ar", QString::fromUtf8("العربية") } },
        { { "thai", "tha", "th" },                                     { "th", QString::fromUtf8("ไทย") } },
    };
    const QString t = token.trimmed().toLower();
    for (const auto &[names, lang] : table)
        if (names.contains(t)) return lang;
    return {};
}

QList<Group> group(const QList<File> &files, const QString &torrentName)
{
    if (files.size() < 2) return {};
    const QString n = torrentName.toLower();
    bool fg = n.contains(QLatin1String("fitgirl")), doi = n.contains(QLatin1String("dodi"));
    for (const File &f : files) {
        const QString b = baseName(f.path).toLower();
        if (b.startsWith(QLatin1String("fg-"))) fg = true;
        if (b.endsWith(QLatin1String(".doi"))) doi = true;
    }
    if (fg) { auto g = fitgirl(files); if (!g.isEmpty()) return g; }
    if (doi) { auto g = dodi(files); if (!g.isEmpty()) return g; }
    if (auto g = seasons(files); !g.isEmpty()) return g;
    if (auto g = filmExtras(files); !g.isEmpty()) return g;
    return {};
}

}
