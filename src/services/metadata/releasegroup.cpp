// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/metadata/releasegroup.h"

#include <QHash>
#include <QRegularExpression>
#include <QSet>

namespace {

// Every spelling a group is published under → the one name we show. Keys are
// lower-case; the aliases exist because release names are written by people, not
// by an API: "OFME", "Online_Fix" and "onlinefix" are all online-fix.me.
//
// Ordered, not a hash: detect() scans it in order, so when a name carries two
// known groups the answer has to be the same on every run. Hash iteration order
// isn't stable across runs, which would make the chip flicker between builds.
using AliasTable = QList<QPair<QString, QString>>;

const AliasTable &aliases()
{
    static const AliasTable map = {
        { QStringLiteral("ofme"), QStringLiteral("Online-Fix") },
        { QStringLiteral("online-fix"), QStringLiteral("Online-Fix") },
        { QStringLiteral("online_fix"), QStringLiteral("Online-Fix") },
        { QStringLiteral("onlinefix"), QStringLiteral("Online-Fix") },
        { QStringLiteral("steamrip"), QStringLiteral("SteamRIP") },
        { QStringLiteral("steam-rip"), QStringLiteral("SteamRIP") },
        { QStringLiteral("fitgirl"), QStringLiteral("FitGirl") },
        { QStringLiteral("dodi"), QStringLiteral("DODI") },
        { QStringLiteral("elamigos"), QStringLiteral("ElAmigos") },
        { QStringLiteral("xatab"), QStringLiteral("Xatab") },
        { QStringLiteral("rgmechanics"), QStringLiteral("R.G. Mechanics") },
        { QStringLiteral("r.g.mechanics"), QStringLiteral("R.G. Mechanics") },
        { QStringLiteral("kaoskrew"), QStringLiteral("KaOsKrew") },
        { QStringLiteral("tinyrepacks"), QStringLiteral("Tiny Repacks") },
        { QStringLiteral("0xdeadc0de"), QStringLiteral("0xdeadc0de") },
        { QStringLiteral("0xdeadcode"), QStringLiteral("0xdeadc0de") },
        { QStringLiteral("razor1911"), QStringLiteral("Razor1911") },
        { QStringLiteral("gog"), QStringLiteral("GOG") },
        { QStringLiteral("codex"), QStringLiteral("CODEX") },
        { QStringLiteral("plaza"), QStringLiteral("PLAZA") },
        { QStringLiteral("skidrow"), QStringLiteral("SKIDROW") },
        { QStringLiteral("tenoke"), QStringLiteral("TENOKE") },
        { QStringLiteral("empress"), QStringLiteral("EMPRESS") },
        { QStringLiteral("cpy"), QStringLiteral("CPY") },
        { QStringLiteral("rune"), QStringLiteral("RUNE") },
        { QStringLiteral("flt"), QStringLiteral("FLT") },
        { QStringLiteral("reloaded"), QStringLiteral("RELOADED") },
        { QStringLiteral("tinyiso"), QStringLiteral("TiNYiSO") },
        { QStringLiteral("hoodlum"), QStringLiteral("HOODLUM") },
        { QStringLiteral("darksiders"), QStringLiteral("DARKSiDERS") },
        { QStringLiteral("simplex"), QStringLiteral("SiMPLEX") },
        { QStringLiteral("steampunks"), QStringLiteral("STEAMPUNKS") },
        { QStringLiteral("3dm"), QStringLiteral("3DM") },
        { QStringLiteral("ali213"), QStringLiteral("ALI213") },
        { QStringLiteral("prophet"), QStringLiteral("PROPHET") },
        { QStringLiteral("igruha"), QStringLiteral("Igruha") },
        { QStringLiteral("pioneer"), QStringLiteral("Pioneer") },
        { QStringLiteral("chovka"), QStringLiteral("Chovka") },
        { QStringLiteral("goldberg"), QStringLiteral("Goldberg") },
        { QStringLiteral("masquerade"), QStringLiteral("Masquerade") },
    };
    return map;
}

// Same table, keyed for canonical()'s O(1) lookup. Built from aliases() so the
// two can't drift.
const QHash<QString, QString> &aliasIndex()
{
    static const QHash<QString, QString> idx = [] {
        QHash<QString, QString> h;
        for (const auto &e : aliases()) h.insert(e.first, e.second);
        return h;
    }();
    return idx;
}

// Format/quality tokens that sit exactly where a group sits. Without this,
// "Movie.2024.WEB-DL" reports the group "DL" and "…x264-1080p" reports "1080p".
bool isFormatToken(const QString &lower)
{
    static const QSet<QString> tokens = {
        QStringLiteral("dl"), QStringLiteral("web"), QStringLiteral("webrip"),
        QStringLiteral("bluray"), QStringLiteral("bdrip"), QStringLiteral("brrip"),
        QStringLiteral("hdrip"), QStringLiteral("dvdrip"), QStringLiteral("hdtv"),
        QStringLiteral("rip"), QStringLiteral("remux"), QStringLiteral("cam"),
        QStringLiteral("ts"), QStringLiteral("hdcam"), QStringLiteral("telesync"),
        QStringLiteral("x264"), QStringLiteral("x265"), QStringLiteral("h264"),
        QStringLiteral("h265"), QStringLiteral("hevc"), QStringLiteral("avc"),
        QStringLiteral("av1"), QStringLiteral("xvid"), QStringLiteral("divx"),
        QStringLiteral("aac"), QStringLiteral("ac3"), QStringLiteral("eac3"),
        QStringLiteral("dts"), QStringLiteral("atmos"), QStringLiteral("truehd"),
        QStringLiteral("flac"), QStringLiteral("mp3"), QStringLiteral("ddp"),
        QStringLiteral("hdr"), QStringLiteral("hdr10"), QStringLiteral("dv"),
        QStringLiteral("dovi"), QStringLiteral("sdr"), QStringLiteral("imax"),
        QStringLiteral("proper"), QStringLiteral("repack"), QStringLiteral("extended"),
        QStringLiteral("unrated"), QStringLiteral("multi"), QStringLiteral("dual"),
        QStringLiteral("dublado"), QStringLiteral("legendado"), QStringLiteral("nacional"),
        QStringLiteral("1080p"), QStringLiteral("720p"), QStringLiteral("480p"),
        QStringLiteral("2160p"), QStringLiteral("576p"), QStringLiteral("4k"),
        QStringLiteral("uhd"), QStringLiteral("10bit"), QStringLiteral("bit"),
        QStringLiteral("dlc"), QStringLiteral("update"), QStringLiteral("patch"),
        QStringLiteral("incl"), QStringLiteral("build"), QStringLiteral("edition"),
    };
    return tokens.contains(lower);
}

QString stripExtension(const QString &name)
{
    static const QRegularExpression extRe(
        QStringLiteral("\\.(?:bin|mkv|avi|mp4|mov|wmv|flv|webm|m4v|ts|iso|rar|zip|7z|torrent)$"),
        QRegularExpression::CaseInsensitiveOption);
    QString out = name;
    out.remove(extRe);
    return out;
}

// A group tag can't be a bare number ("-2160"), a single letter, or a format
// token. Everything else is taken at face value — the scene invents names
// faster than any table can track, and showing "STARCKFILMES" beats showing
// nothing at all.
bool looksLikeGroup(const QString &tag)
{
    if (tag.size() < 2 || tag.size() > 24) return false;
    bool hasLetter = false;
    for (const QChar &c : tag)
        if (c.isLetter()) { hasLetter = true; break; }
    if (!hasLetter) return false;
    return !isFormatToken(tag.toLower());
}

}

QString ReleaseGroup::canonical(const QString &tag)
{
    QString t = tag.trimmed();
    // "SteamRIP.com(1)" — the copy index a browser appends, and the domain tail
    static const QRegularExpression trailingCopy(QStringLiteral("\\s*\\(\\d+\\)\\s*$"));
    t.remove(trailingCopy);
    static const QRegularExpression edgeJunk(QStringLiteral("^[\\s.\\-_\\[\\]()]+|[\\s.\\-_\\[\\]()]+$"));
    t.remove(edgeJunk);
    if (t.isEmpty()) return QString();

    const QString lower = t.toLower();
    const auto it = aliasIndex().constFind(lower);
    if (it != aliasIndex().cend()) return it.value();

    // "steamrip.com" / "online-fix.me" — the domain form of a known group
    const qsizetype dot = lower.indexOf(QLatin1Char('.'));
    if (dot > 0) {
        const auto domainIt = aliasIndex().constFind(lower.left(dot));
        if (domainIt != aliasIndex().cend()) return domainIt.value();
    }

    return looksLikeGroup(t) ? t : QString();
}

QString ReleaseGroup::detect(const QString &releaseName)
{
    const QString name = stripExtension(releaseName);
    if (name.isEmpty()) return QString();

    // A known group anywhere in the name wins: it's the one answer we're sure
    // about, and it survives whatever the uploader appended after it.
    static QHash<QString, QRegularExpression> cache;   // parse() runs per search row
    for (const auto &entry : aliases()) {
        auto re = cache.find(entry.first);
        if (re == cache.end())
            re = cache.insert(entry.first, QRegularExpression(
                QStringLiteral("(?:^|[.\\s\\-_\\[\\]()])%1(?:$|[.\\s\\-_\\[\\]()])")
                    .arg(QRegularExpression::escape(entry.first)),
                QRegularExpression::CaseInsensitiveOption));
        if (re->match(name).hasMatch()) return entry.second;
    }

    // "… by Pioneer", "… from SomeGroup" — the uploader credit games carry.
    static const QRegularExpression byRe(
        QStringLiteral("[.\\s\\-_]+(?:by|from)[.\\s\\-_]+([\\w.\\-']{2,24})\\s*$"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch bm = byRe.match(name);
    if (bm.hasMatch()) {
        const QString g = canonical(bm.captured(1));
        if (!g.isEmpty()) return g;
    }

    // The scene tail: everything after the last dash is the group, by convention
    // ("Obsessao.2026.WEB-DL.1080p.x264-STARCKFILMES").
    static const QRegularExpression tailRe(
        QStringLiteral("-\\s*([A-Za-z0-9][\\w.'\\-]{1,23})\\s*(?:\\(\\d+\\))?$"));
    const QRegularExpressionMatch tm = tailRe.match(name);
    if (tm.hasMatch()) return canonical(tm.captured(1));

    return QString();
}

const QStringList &ReleaseGroup::gameSignals()
{
    static const QStringList list = {
        QStringLiteral("FitGirl"), QStringLiteral("DODI"), QStringLiteral("CODEX"),
        QStringLiteral("PLAZA"), QStringLiteral("RUNE"), QStringLiteral("EMPRESS"),
        QStringLiteral("ElAmigos"), QStringLiteral("Xatab"), QStringLiteral("R.G.Mechanics"),
        QStringLiteral("GOG"), QStringLiteral("SKIDROW"), QStringLiteral("RELOADED"),
        QStringLiteral("TiNYiSO"), QStringLiteral("HOODLUM"), QStringLiteral("DARKSiDERS"),
        QStringLiteral("TENOKE"), QStringLiteral("SiMPLEX"), QStringLiteral("RAZOR1911"),
        QStringLiteral("CPY"), QStringLiteral("STEAMPUNKS"), QStringLiteral("3DM"),
        QStringLiteral("ALI213"), QStringLiteral("PROPHET"), QStringLiteral("Igruha"),
        QStringLiteral("KaOsKrew"), QStringLiteral("Chovka"), QStringLiteral("Pioneer"),
        // online-fix ships its releases as OFME far more often than spelled out
        QStringLiteral("Online-Fix"), QStringLiteral("OFME")
    };
    return list;
}
