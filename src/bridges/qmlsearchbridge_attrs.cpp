// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSearchBridge — release-name attrs + trust fill.

#include "bridges/qmlsearchbridge.h"
#include "bridges/qmlsearchbridge_util.h"
#include "torrent/iengine.h"
#include "services/metadata/audiomode.h"
#include "services/metadata/episodegroup.h"
#include "services/metadata/metadataresolver.h"
#include "services/discovery/discoveryservice.h"
#include "services/downloads/httpdownloadmanager.h"
#include "services/downloads/filehostresolver.h"
#include "services/metadata/nameparser.h"
#include "services/metadata/releasegroup.h"
#include "services/metadata/releasepick.h"
#include "services/metadata/gamereleasepick.h"
#include "services/metadata/searchranker.h"
#include "services/metadata/releasetrust.h"
#include "services/integrations/rssmanager.h"
#include "services/discovery/addonmanager.h"
#include "services/platform/utils.h"
#include "services/platform/contentlanguage.h"
#include "services/platform/translator.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSettings>
#include <QStorageInfo>
#include <QUrl>
#include <algorithm>

QString QmlSearchBridge::detectReleaseGroup(const QString &name)
{
    return ReleaseGroup::detect(name);
}

void QmlSearchBridge::fillMediaAttrs(QVariantMap &m, const QString &name)
{
    // ~30 patterns × every result row — cache compiled regexes (keyed by the
    // literal's pointer; main-thread only).
    auto has = [&](const char *pat) {
        static QHash<const char *, QRegularExpression> cache;
        auto it = cache.find(pat);
        if (it == cache.end())
            it = cache.insert(pat, QRegularExpression(QLatin1String(pat),
                                                      QRegularExpression::CaseInsensitiveOption));
        return name.contains(*it);
    };
    if (m.value(QStringLiteral("quality")).toString().isEmpty()) {
        QString q;
        if (has("2160p|\\b4k\\b|\\buhd\\b")) q = QStringLiteral("4K");
        else if (has("1080p")) q = QStringLiteral("1080p");
        else if (has("720p")) q = QStringLiteral("720p");
        else if (has("480p|360p")) q = QStringLiteral("480p");
        m["quality"] = q;
    }
    QString src;
    if (has("remux")) src = QStringLiteral("Remux");
    else if (has("blu-?ray|\\bbdrip\\b|\\bbrrip\\b")) src = QStringLiteral("BluRay");
    else if (has("web-?dl|web-?rip|\\bweb\\b")) src = QStringLiteral("WEB");
    else if (has("\\bhdtv\\b|\\bpdtv\\b")) src = QStringLiteral("HDTV");
    else if (has("dvdrip|\\bdvd\\b")) src = QStringLiteral("DVD");
    else if (has("\\bcam\\b|hdcam|telesync|\\bts\\b")) src = QStringLiteral("CAM");
    m["source"] = src;
    QString codec;
    if (has("x265|h\\.?265|hevc")) codec = QStringLiteral("HEVC");
    else if (has("x264|h\\.?264|\\bavc\\b")) codec = QStringLiteral("x264");
    else if (has("av1")) codec = QStringLiteral("AV1");
    m["codec"] = codec;
    m["hdr"] = has("\\bhdr\\b|hdr10|dolby ?vision");

    // Spoken languages, parsed from the release name's audio tags. A release can
    // carry several (DUAL/MULTI), so we collect a list and let the search filter
    // match on membership — Torrentio-style. `lang` keeps the primary for the badge.
    QStringList langs;
    auto add = [&](const QString &c) { if (!langs.contains(c)) langs << c; };
    const bool dubbed = has("\\bdublado\\b|\\bdubbed\\b|\\bdual[ ._-]?(a|á)udio\\b|\\bnacional\\b|\\bdub\\b");
    if (has("dublado|nacional|\\bpt[ ._-]?br\\b|\\bptbr\\b|portugu[eê]s|\\btuga\\b|leg(endado)?[ ._-]?pt")) add(QStringLiteral("PT"));
    if (has("\\bcastellano\\b|espa[nñ]ol|\\blatino\\b|\\bspanish\\b|\\besp\\b")) add(QStringLiteral("ES"));
    if (has("\\bgerman\\b|deutsch|\\bger\\b")) add(QStringLiteral("DE"));
    if (has("\\bitalian\\b|\\bita\\b")) add(QStringLiteral("IT"));
    if (has("\\bfrench\\b|\\bfra\\b|\\btruefrench\\b|\\bvostfr\\b|\\bvff\\b")) add(QStringLiteral("FR"));
    static const QRegularExpression cyrillicRe(QStringLiteral("[\\x{0400}-\\x{04FF}]"));
    if (has("\\brus(sian)?\\b|дубляж|русск") || name.contains(cyrillicRe)) add(QStringLiteral("RU"));
    if (has("\\bjapanese\\b|\\bjpn\\b|\\bjap\\b")) add(QStringLiteral("JA"));
    if (has("ukrain|\\bukr\\b")) add(QStringLiteral("UK"));
    if (has("\\bchinese\\b|\\bchs\\b|\\bcht\\b|\\bmandarin\\b")) add(QStringLiteral("ZH"));
    if (has("\\bkorean\\b|\\bkor\\b")) add(QStringLiteral("KO"));
    if (has("\\bhindi\\b|\\bhin\\b")) add(QStringLiteral("HI"));
    if (has("\\benglish\\b|\\beng\\b")) add(QStringLiteral("EN"));
    const bool multi = has("\\bmulti\\b|dual[ ._-]?(a|á)udio|dual[ ._-]?lat");
    if (multi && langs.isEmpty()) add(QStringLiteral("MULTI"));

    m["langs"] = langs;
    m["lang"] = langs.isEmpty() ? QString() : langs.first();
    m["dubbed"] = dubbed || multi;

    // Game builds: one search returns the same title a dozen times, and the only
    // thing separating the rows is the version. Without it the list is 51
    // indistinguishable lines.
    m["version"] = GameReleasePick::parseVersion(name);

    const QString contentLang = ContentLanguage::releaseTag();
    m["native"] = langs.contains(contentLang)
                  || (contentLang != QLatin1String("EN") && (multi || langs.contains(QLatin1String("MULTI"))));
    // Dub/sub/original relative to the user's language — the axis the segmented
    // filter acts on (a dubbed-hater and a dub-lover want opposite results).
    m["audioMode"] = AudioMode::key(AudioMode::classify(name, contentLang));
}

void QmlSearchBridge::fillTrust(QVariantMap &m, const QString &name)
{
    ReleaseTrust::Release r;
    r.name = name;
    r.quality = m.value(QStringLiteral("quality")).toString();
    r.source = m.value(QStringLiteral("source")).toString();
    r.seeders = m.value(QStringLiteral("seedsN")).toInt();
    r.sizeBytes = m.value(QStringLiteral("sizeBytes")).toLongLong();

    const auto v = ReleaseTrust::assess(r);
    m["trust"] = ReleaseTrust::tierKey(v.tier);
    m["trustWhy"] = v.reasons.isEmpty() ? QString() : v.reasons.first();
    m["trustScore"] = v.score;
}

