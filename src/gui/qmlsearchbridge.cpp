// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "qmlposterbridge.h"
#include "../torrent/sessionmanager.h"
#include "../app/metadataresolver.h"
#include "../app/discoveryservice.h"
#include "../app/nameparser.h"
#include "../app/releasepick.h"
#include "../app/rssmanager.h"
#include "../app/addonmanager.h"
#include "../app/logger.h"
#include "../app/qrcodegen.h"
#include "../app/utils.h"
#include "../app/translator.h"
#include "../app/geoip.h"
#include "../app/discordrpc.h"
#include "../app/updater.h"
#include "../app/notifier.h"
#include "../app/secretstore.h"
#include "../webui/webserver.h"
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDateTime>

#include <QNetworkInterface>

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QApplication>
#include <QWindow>
#include <QEvent>
#ifdef Q_OS_WIN
#  include <windows.h>
#  include <dwmapi.h>
#  ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#    define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#  endif
#endif
#include <QCoreApplication>
#include <QStyleHints>
#include <QPainter>
#include <QSvgRenderer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <cstring>
#include <algorithm>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

#include <libtorrent/torrent_info.hpp>
#include <libtorrent/file_storage.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/version.hpp>
#include <openssl/opensslv.h>
#include <boost/version.hpp>
#include <memory>
#include <sstream>

QmlSearchBridge::QmlSearchBridge(IEngine *session, QObject *parent)
    : QObject(parent), m_session(session), m_mode("torrent")
{
    auto &mgr = AddonManager::instance();
    connect(&mgr, &AddonManager::catalogResults, this, [this](const QList<CatalogItem> &items) {
        m_catalogCache = items;
        if (m_mode != "catalog") return;
        m_results.clear();
        for (const auto &it : items) {
            QVariantMap m;
            m["name"] = it.name;
            m["sub"] = it.type;
            m["sizeStr"] = it.year > 0 ? QString::number(it.year) : QString();
            m["seeds"] = ""; m["leech"] = ""; m["repacker"] = "";
            m["poster"] = it.poster; m["coverHash"] = "";
            m["seedsN"] = 0; m["sizeBytes"] = 0;
            fillMediaAttrs(m, it.name);
            m_results << m;
        }
        emit resultsChanged();
    });
    connect(&mgr, &AddonManager::catalogFinished, this, [this]() {
        setSearching(false);
        setStatus(tr_("search_results_n").arg(m_catalogCache.size()));
    });
    connect(&mgr, &AddonManager::streamResults, this, [this](const QList<StreamResult> &streams) {
        m_streamCache = streams;
        if (m_mode != "streams") return;
        m_results.clear();
        for (const auto &s : streams) {
            QVariantMap m;
            m["name"] = s.title;
            m["sub"] = s.addonName;
            m["provider"] = s.addonName;
            m["sizeStr"] = s.size > 0 ? formatSize(s.size) : QString();
            m["seeds"] = ""; m["leech"] = ""; m["repacker"] = "";
            m["poster"] = m_streamHintPoster; m["coverHash"] = "";
            m["quality"] = s.quality;
            m["seedsN"] = 0; m["sizeBytes"] = s.size;
            fillMediaAttrs(m, s.title);
            m_results << m;
        }
        emit resultsChanged();
    });
    connect(&mgr, &AddonManager::streamFinished, this, [this]() {
        setSearching(false);
        setStatus(tr_("search_streams_n").arg(m_streamCache.size()));
    });
    connect(&mgr, &AddonManager::torrentSearchResults, this,
            [this](const QList<TorrentSearchResult> &results) {
        if (m_mode != "torrent" && m_mode != "games" && m_mode != "all") return;
        if (!m_aggregate) {   // single source replaces; aggregate appends each batch
            m_results.clear(); m_resultMagnets.clear(); m_torrentCache.clear();
        }
        appendTorrentRows(results);
    });
    connect(&mgr, &AddonManager::torrentSearchFinished, this, [this]() {
        if (m_aggregate) { finishAggregateSource(); return; }
        setSearching(false);
        setStatus(tr_("search_results_n").arg(m_results.size()));
    });
    connect(&mgr, &AddonManager::torrentSearchError, this, [this](const QString &err) {
        if (m_aggregate) { finishAggregateSource(); return; }   // one provider failing ≠ whole search
        setSearching(false);
        setStatus(err);
    });

    connect(&mgr, &AddonManager::torrentSummaryReady, this,
            [this](const QString &query, int count, qint64 bestSize, int maxSeeds) {
        const QString key = query.toLower().trimmed();
        m_srcSummaryInFlight.remove(key);
        QVariantList v; v << count << QVariant::fromValue(bestSize) << maxSeeds;
        m_srcSummaryCache.insert(key, v);
        emit sourceSummary(query, count, bestSize, maxSeeds);
    });

    connect(&GameSourceManager::instance(), &GameSourceManager::refreshed, this, [this](int count) {
        emit gameSourcesChanged();
        if (m_pendingGameQuery.isEmpty()) return;
        const QString q = m_pendingGameQuery;
        m_pendingGameQuery.clear();
        if (m_aggregate) {
            if (count > 0) appendGameRows(GameSourceManager::instance().search(q));
            finishAggregateSource();
            return;
        }
        if (m_mode != "games") return;
        if (count > 0) runGameSearch(q);
        else { setSearching(false); setStatus(tr_("search_no_games")); }
    });

    QSettings s;
    m_savePath = s.value(QStringLiteral("lastSavePath")).toString();
    if (m_savePath.isEmpty() || !QDir(m_savePath).exists())
        m_savePath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
}

QString QmlSearchBridge::detectRepacker(const QString &name)
{
    const QString lower = name.toLower();
    if (lower.contains("fitgirl")) return "FitGirl";
    if (lower.contains("dodi")) return "DODI";
    if (lower.contains("online-fix") || lower.contains("onlinefix")) return "Online-Fix";
    if (lower.contains("elamigos")) return "ElAmigos";
    if (lower.contains("xatab")) return "Xatab";
    if (lower.contains("r.g. mechanics") || lower.contains("rg mechanics")) return "R.G. Mechanics";
    if (lower.contains("gog")) return "GOG";
    if (lower.contains("codex")) return "CODEX";
    if (lower.contains("plaza")) return "PLAZA";
    if (lower.contains("skidrow")) return "SKIDROW";
    if (lower.contains("kaoskrew") || lower.contains("kaos krew")) return "KaOsKrew";
    if (lower.contains("tenoke")) return "TENOKE";
    if (lower.contains("empress")) return "EMPRESS";
    if (lower.contains("razor1911") || lower.contains("razor 1911")) return "Razor1911";
    if (lower.contains("goldberg")) return "Goldberg";
    if (lower.contains("0xdeadc0de") || lower.contains("0xdeadcode")) return "0xdeadc0de";
    if (lower.contains("masquerade")) return "Masquerade";
    if (lower.contains("chovka")) return "Chovka";
    if (lower.contains("tinyrepacks") || lower.contains("tiny repacks")) return "Tiny Repacks";
    if (lower.contains("cpy")) return "CPY";
    if (lower.contains("-flt") || lower.contains("flt]")) return "FLT";
    if (lower.contains("-rune") || lower.contains("rune]")) return "RUNE";
    return "";
}

void QmlSearchBridge::fillMediaAttrs(QVariantMap &m, const QString &name)
{
    auto has = [&](const char *pat) {
        return name.contains(QRegularExpression(QLatin1String(pat),
                                                QRegularExpression::CaseInsensitiveOption));
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
    if (has("\\brus(sian)?\\b|дубляж|русск")
        || name.contains(QRegularExpression(QStringLiteral("[\\x{0400}-\\x{04FF}]")))) add(QStringLiteral("RU"));
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

    const QString appLang = appLangCode();
    m["native"] = langs.contains(appLang)
                  || (appLang != QLatin1String("EN") && (multi || langs.contains(QLatin1String("MULTI"))));
}

QString QmlSearchBridge::appLangCode()
{
    switch (Translator::instance().language()) {
    case Translator::Portuguese: return QStringLiteral("PT");
    case Translator::Spanish:    return QStringLiteral("ES");
    case Translator::German:     return QStringLiteral("DE");
    case Translator::Russian:    return QStringLiteral("RU");
    case Translator::Japanese:   return QStringLiteral("JA");
    case Translator::Chinese:    return QStringLiteral("ZH");
    case Translator::Ukrainian:  return QStringLiteral("UK");
    default:                     return QStringLiteral("EN");
    }
}

void QmlSearchBridge::setResolver(MetadataResolver *r)
{
    m_resolver = r;
    if (!m_resolver) return;
    connect(m_resolver, &MetadataResolver::metadataReady, this,
            [this](const QString &infoHash, const MetadataResult &meta) {
        if (!meta.valid || meta.posterPath.isEmpty()) return;
        for (auto &v : m_results) {
            QVariantMap m = v.toMap();
            if (m.value(QStringLiteral("coverHash")).toString() == infoHash
                && m.value(QStringLiteral("poster")).toString().isEmpty()) {
                m["poster"] = meta.posterPath;
                v = m;
            }
        }
        emit coverReady(infoHash, meta.posterPath);
    });
}

void QmlSearchBridge::resolveCover(int index)
{
    if (!m_resolver || index < 0 || index >= m_results.size()) return;
    const QVariantMap m = m_results[index].toMap();
    if (!m.value(QStringLiteral("poster")).toString().isEmpty()) return;
    const QString hash = m.value(QStringLiteral("coverHash")).toString();
    if (hash.isEmpty()) return;
    if (m_resolver->hasCached(hash)) {
        const auto meta = m_resolver->cached(hash);
        if (meta.valid && !meta.posterPath.isEmpty()) {
            QVariantMap mm = m;
            mm["poster"] = meta.posterPath;
            m_results[index] = mm;
            emit coverReady(hash, meta.posterPath);
        }
        return;
    }
    m_resolver->resolve(hash, m.value(QStringLiteral("name")).toString());
}

void QmlSearchBridge::setDiscovery(DiscoveryService *d)
{
    m_discovery = d;
    if (!m_discovery) return;
    connect(m_discovery, &DiscoveryService::titleResults, this,
            [this](const QString &query, const QVariantList &works) {
        if (query != m_titleQuery || m_mode != QLatin1String("titles")) return;   // stale
        m_results.clear();
        m_resultMagnets.clear();
        m_resultTitles.clear();
        for (const QVariant &v : works) {
            const QVariantMap w = v.toMap();
            QVariantMap row;
            row["name"]    = w.value(QStringLiteral("title"));
            row["title"]   = w.value(QStringLiteral("title"));
            row["sub"]     = w.value(QStringLiteral("type"));
            row["sizeStr"] = w.value(QStringLiteral("year"));
            row["year"]    = w.value(QStringLiteral("year"));
            row["type"]    = w.value(QStringLiteral("type"));
            row["poster"]  = w.value(QStringLiteral("poster"));
            row["rating"]  = w.value(QStringLiteral("rating"));
            row["overview"] = w.value(QStringLiteral("overview"));
            row["coverHash"] = QString();
            row["isTitle"] = true;
            m_results << row;
        }
        m_titleCache = m_results;
        setSearching(false);
        // Stay in the grid even when empty — the page shows an empty state with a
        // "raw results" escape, so the flow is consistent (never silently flips).
        setStatus(m_results.isEmpty() ? tr_("search_no_titles")
                                      : tr_("search_titles_n").arg(m_results.size()));
        emit resultsChanged();
    });
}

void QmlSearchBridge::searchSourcesForWork(const QString &title, const QString &year, const QString &type)
{
    m_results.clear();
    m_resultMagnets.clear();
    m_resultTitles.clear();
    m_torrentCache.clear();
    m_gameCache.clear();
    m_pendingGameQuery.clear();
    emit resultsChanged();

    m_activeQuery = title;
    auto &mgr = AddonManager::instance();
    const bool isGame = (type == QLatin1String("game"));
    m_aggregate = true;
    m_titleSources = true;          // rows are one picked title → page drops per-row covers
    m_isGameSearch = isGame;
    setMode("all");
    setSearching(true);
    setStatus(tr_("search_searching2"));
    m_pendingSources = 0;

    if (isGame) {
        auto &gsm = GameSourceManager::instance();
        if (gsm.gameCount() > 0) appendGameRows(gsm.search(title));
        else if (!gsm.sources().isEmpty()) { m_pendingGameQuery = title; ++m_pendingSources; gsm.refresh(); }
    }
    // movies disambiguate well with a year; games/series search cleaner by title
    const QString q = (type == QLatin1String("movie") && !year.isEmpty())
                    ? title + QLatin1Char(' ') + year : title;
    const int cat = isGame ? 400 : 200;   // 400 = games, 200 = video
    const auto providers = mgr.searchProviders();
    for (int i = 0; i < providers.size(); ++i)
        if (providers[i].enabled) { ++m_pendingSources; mgr.searchWithProvider(i, q, cat); }
    if (mgr.torrentSearchEnabled()) { ++m_pendingSources; mgr.searchTorrents(q, cat); }
    if (m_pendingSources == 0) {
        setSearching(false);
        setStatus(tr_("search_results_n").arg(m_results.size()));
    }
}

static QString btihFromMagnet(const QString &magnet);   // defined below

int QmlSearchBridge::pickBestResult() const
{
    QList<ReleasePick::Candidate> cands;
    cands.reserve(m_results.size());
    for (const QVariant &v : m_results) {
        const QVariantMap m = v.toMap();
        cands.append({ m.value(QStringLiteral("quality")).toString(),
                       m.value(QStringLiteral("native")).toBool(),
                       m.value(QStringLiteral("seedsN")).toInt(),
                       m.value(QStringLiteral("sizeBytes")).toLongLong() });
    }
    const QSettings s(QStringLiteral("BATorrent"), QStringLiteral("BATorrent"));
    // select index → quality token (matches the SettingsWindow "Reprodução" options)
    static const char *qmap[] = { "Auto", "1080p", "720p", "4K" };
    const int qi = s.value(QStringLiteral("preferredQuality"), 1).toInt();
    const QString prefQ = QString::fromLatin1((qi >= 0 && qi < 4) ? qmap[qi] : "1080p");
    const qint64 maxBytes = s.value(QStringLiteral("preferMaxSize"), 0).toLongLong() * 1024 * 1024;
    const bool preferNative = s.value(QStringLiteral("preferNativeLang"), true).toBool();
    return ReleasePick::best(cands, prefQ, maxBytes, preferNative);
}

void QmlSearchBridge::getAndWatch(const QString &title, const QString &year, const QString &type)
{
    if (type == QLatin1String("game")) return;   // games are not stream-to-watch (Tema 4)
    m_gwActive = true;
    m_gwCancelled = false;
    m_gwTitle = title;
    m_gwType = type;
    emit watchSearching(title);
    searchSourcesForWork(title, year, type);      // gwResolve() runs when this settles
    // If the search had nothing to wait on (no providers), it finished inline.
    if (m_gwActive && !m_searching) gwResolve();
}

void QmlSearchBridge::cancelGetAndWatch()
{
    m_gwActive = false;
    m_gwCancelled = true;
}

void QmlSearchBridge::summarizeSources(const QString &title)
{
    const QString key = title.toLower().trimmed();
    if (key.isEmpty()) return;
    if (m_srcSummaryCache.contains(key)) {
        const QVariantList v = m_srcSummaryCache.value(key);
        emit sourceSummary(title, v.value(0).toInt(), v.value(1).toLongLong(), v.value(2).toInt());
        return;
    }
    if (m_srcSummaryInFlight.contains(key)) return;
    m_srcSummaryInFlight.insert(key);
    AddonManager::instance().summarizeTorrents(title, 0);
}

void QmlSearchBridge::gwResolve()
{
    m_gwActive = false;
    if (m_gwCancelled) { m_gwCancelled = false; return; }   // user backed out during the search
    const int idx = pickBestResult();
    if (idx < 0 || idx >= m_resultMagnets.size() || m_resultMagnets[idx].isEmpty()) {
        emit watchNoRelease(m_gwTitle);
        return;
    }
    const QString magnet = m_resultMagnets[idx];
    const QVariantMap rm = m_results[idx].toMap();
    // Prefer Get&Watch's known title/type as the cover hint, so the player and
    // library show the real movie/series poster instead of the raw torrent name
    // (which is empty until the magnet's metadata resolves → placeholder). Fall
    // back to the per-row game title for the game flow.
    QString hint = idx < m_resultTitles.size() ? m_resultTitles[idx] : QString();
    int type = hint.isEmpty() ? -1 : static_cast<int>(ContentType::Game);
    if (hint.isEmpty() && !m_gwTitle.isEmpty()) {
        hint = m_gwTitle;
        type = m_gwType == QLatin1String("series") ? static_cast<int>(ContentType::Series)
             : m_gwType == QLatin1String("game")   ? static_cast<int>(ContentType::Game)
             : static_cast<int>(ContentType::Movie);
    }
    m_session->addMagnet(magnet, m_savePath, hint, type);
    QString hash = rm.value(QStringLiteral("coverHash")).toString();
    if (hash.isEmpty()) hash = btihFromMagnet(magnet);
    emit prepareAndWatch(hash, m_gwTitle);
}

void QmlSearchBridge::copyMagnet(int index)
{
    if (index < 0 || index >= m_resultMagnets.size()) return;
    const QString magnet = m_resultMagnets[index];
    if (magnet.isEmpty()) return;
    QGuiApplication::clipboard()->setText(magnet);
    setStatus(tr_("search_magnet_copied"));
}

void QmlSearchBridge::searchRaw()
{
    if (m_titleQuery.isEmpty()) return;
    // keep the title context so "back" still returns to the titles grid
    m_fromTitles = !m_titleCache.isEmpty();
    rawAggregateSearch(m_titleQuery, 0);
}

void QmlSearchBridge::rawAggregateSearch(const QString &q, int categoryCode)
{
    m_results.clear();
    m_resultMagnets.clear();
    m_resultTitles.clear();
    m_torrentCache.clear();
    m_gameCache.clear();
    m_pendingGameQuery.clear();
    emit resultsChanged();

    m_activeQuery = q;
    auto &mgr = AddonManager::instance();
    m_aggregate = true;
    m_titleSources = false;         // raw mixed list → keep per-row covers
    m_isGameSearch = false;
    setMode("all");
    setSearching(true);
    setStatus(tr_("search_searching2"));
    m_pendingSources = 0;
    auto &gsm = GameSourceManager::instance();
    if (gsm.gameCount() > 0) {
        appendGameRows(gsm.search(q));
    } else if (!gsm.sources().isEmpty()) {
        m_pendingGameQuery = q;          // catalogs load async; counts as a pending source
        ++m_pendingSources;
        gsm.refresh();
    }
    const auto providers = mgr.searchProviders();
    for (int i = 0; i < providers.size(); ++i)
        if (providers[i].enabled) { ++m_pendingSources; mgr.searchWithProvider(i, q); }
    if (mgr.torrentSearchEnabled()) { ++m_pendingSources; mgr.searchTorrents(q, categoryCode); }
    if (m_pendingSources == 0) {
        setSearching(false);
        setStatus(tr_("search_results_n").arg(m_results.size()));
    }
}

QVariantList QmlSearchBridge::sources() const
{
    QVariantList out;
    auto add = [&out](const QString &key, const QString &label) {
        QVariantMap m; m["key"] = key; m["label"] = label; out << m;
    };
    add("all", tr_("search_source_all"));                  // default: search every source at once
    add("stremio", tr_("search_source_stremio"));
    auto &mgr = AddonManager::instance();
    if (mgr.torrentSearchEnabled())
        add("legacy", tr_("search_source_torrents"));
    // Games search is independent of the torrent provider: show it whenever a
    // game catalog is configured (a default is seeded on first run), or as a
    // fallback when the torrent provider is on (TPB Games category).
    if (!GameSourceManager::instance().sources().isEmpty() || mgr.torrentSearchEnabled())
        add("games", tr_("search_source_games"));
    const auto providers = mgr.searchProviders();
    for (int i = 0; i < providers.size(); ++i)
        if (providers[i].enabled)
            add(QString("provider:%1").arg(i), providers[i].name);
    return out;
}

QVariantList QmlSearchBridge::categories() const
{
    QVariantList out;
    auto add = [&out](int code, const QString &label) {
        QVariantMap m; m["code"] = code; m["label"] = label; out << m;
    };
    add(0, tr_("search_cat_all")); add(100, tr_("search_cat_audio")); add(200, tr_("search_cat_video"));
    add(300, tr_("search_cat_apps")); add(400, tr_("search_cat_games")); add(500, tr_("search_cat_other"));
    return out;
}

QVariantList QmlSearchBridge::results() const
{
    // Stamp each row's index into the data itself. QML used to add `_idx` by
    // mutating the map (o._idx = i), but a QVariantMap handed to QML is a copy —
    // the mutation didn't always stick, leaving srcIndex undefined and breaking
    // activateResult()/openDetail() ("no source" on every pick).
    QVariantList out;
    out.reserve(m_results.size());
    for (int i = 0; i < m_results.size(); ++i) {
        QVariantMap m = m_results.at(i).toMap();
        m[QStringLiteral("_idx")] = i;
        out.append(m);
    }
    return out;
}
QString QmlSearchBridge::activeQuery() const { return m_activeQuery; }
QString QmlSearchBridge::mode() const { return m_mode; }
bool QmlSearchBridge::inStreams() const { return m_mode == "streams"; }
bool QmlSearchBridge::canGoBack() const { return m_mode == "streams" || m_fromTitles; }
bool QmlSearchBridge::singleTitleView() const { return m_titleSources || m_mode == "streams"; }
bool QmlSearchBridge::searching() const { return m_searching; }
QString QmlSearchBridge::statusText() const { return m_status; }

void QmlSearchBridge::setSearching(bool on) { if (m_searching == on) return; m_searching = on; emit searchingChanged(); }
void QmlSearchBridge::setStatus(const QString &s) { if (m_status == s) return; m_status = s; emit statusChanged(); }
void QmlSearchBridge::setMode(const QString &m) { if (m_mode == m) return; m_mode = m; emit modeChanged(); }

void QmlSearchBridge::refreshSources() { emit sourcesChanged(); }

void QmlSearchBridge::search(const QString &sourceKey, const QString &query, int categoryCode)
{
    const QString q = query.trimmed();
    if (q.isEmpty()) return;
    m_lastQuery = q;
    m_activeQuery = q;
    m_aggregate = false;
    m_titleSources = false;
    m_pendingGameQuery.clear();
    m_results.clear();
    m_resultMagnets.clear();
    m_resultTitles.clear();
    m_torrentCache.clear();
    m_gameCache.clear();
    emit resultsChanged();

    auto &mgr = AddonManager::instance();
    if (sourceKey == "all") {
        // Title-first: resolve the query to real works (TMDB/IGDB), then let the
        // user drill into one title's torrents. Only when a metadata service with
        // keys is available — otherwise go straight to the flat aggregate.
        if (!m_discovery || !m_discovery->hasMetadataKeys()) {
            rawAggregateSearch(q, categoryCode);
            return;
        }
        m_fromTitles = false;
        m_titleCache.clear();
        m_titleQuery = q;
        setMode("titles");
        setSearching(true);
        setStatus(tr_("search_searching_titles"));
        m_discovery->searchTitles(q);
        return;
    } else if (sourceKey == "games") {
        m_isGameSearch = true;
        setMode("games");
        auto &gsm = GameSourceManager::instance();
        if (gsm.gameCount() == 0 && !gsm.sources().isEmpty()) {
            m_pendingGameQuery = q;          // search once the catalogs finish loading
            setSearching(true);
            setStatus(tr_("search_loading_game_catalogs"));
            gsm.refresh();
            return;
        }
        if (gsm.gameCount() > 0) { runGameSearch(q); return; }
        // No game catalogs configured → fall back to the bundled torrent provider's
        // Games category so the search isn't empty out of the box.
        m_gameCache.clear();
        setSearching(true);
        setStatus(tr_("search_searching2"));
        mgr.searchTorrents(q, 400);
    } else if (sourceKey.startsWith("provider:")) {
        m_isGameSearch = false;
        setMode("torrent");
        setSearching(true);
        setStatus(tr_("search_searching2"));
        mgr.searchWithProvider(sourceKey.mid(9).toInt(), q);
    } else if (sourceKey == "legacy") {
        m_isGameSearch = false;
        setMode("torrent");
        setSearching(true);
        setStatus(tr_("search_searching2"));
        mgr.searchTorrents(q, categoryCode);
    } else {
        if (!mgr.hasCatalogAddon()) { setStatus(tr_("search_no_catalog_addon")); return; }
        setMode("catalog");
        setSearching(true);
        setStatus(tr_("search_searching2"));
        mgr.searchCatalog(q);
    }
}

static QString btihFromMagnet(const QString &magnet)
{
    static const QRegularExpression re(QStringLiteral("xt=urn:btih:([A-Za-z0-9]+)"),
                                       QRegularExpression::CaseInsensitiveOption);
    const auto m = re.match(magnet);
    return m.hasMatch() ? m.captured(1) : QString();
}

static QString resultDedupeKey(const QString &magnet, const QString &name, qlonglong size)
{
    const QString h = btihFromMagnet(magnet).toLower();
    if (!h.isEmpty()) return h;
    return name.toLower() + QLatin1Char('|') + QString::number(size);
}

QSet<QString> QmlSearchBridge::currentResultKeys() const
{
    QSet<QString> seen;
    seen.reserve(m_results.size());
    for (int i = 0; i < m_results.size() && i < m_resultMagnets.size(); ++i) {
        const auto rm = m_results[i].toMap();
        seen.insert(resultDedupeKey(m_resultMagnets[i], rm.value(QStringLiteral("name")).toString(),
                                    rm.value(QStringLiteral("sizeBytes")).toLongLong()));
    }
    return seen;
}

void QmlSearchBridge::appendGameRows(const QList<GameDownload> &games)
{
    QSet<QString> seen = currentResultKeys();
    for (const auto &g : games) {
        const QString key = resultDedupeKey(g.magnet, g.cleanTitle.isEmpty() ? g.title : g.cleanTitle, 0);
        if (seen.contains(key)) continue;
        seen.insert(key);
        QVariantMap m;
        m["name"] = g.cleanTitle.isEmpty() ? g.title : g.cleanTitle;
        m["sub"] = g.source;
        m["provider"] = g.source;
        m["sizeStr"] = g.fileSize;
        m["seeds"] = ""; m["leech"] = ""; m["hasSeeds"] = false;
        m["repacker"] = detectRepacker(g.title);
        m["poster"] = ""; m["coverHash"] = "";
        m["seedsN"] = 0; m["sizeBytes"] = 0;
        fillMediaAttrs(m, g.title);
        m_results << m;
        m_resultMagnets << g.magnet;
        m_resultTitles << (g.cleanTitle.isEmpty() ? g.title : g.cleanTitle);
    }
    emit resultsChanged();
}

void QmlSearchBridge::appendTorrentRows(const QList<TorrentSearchResult> &results)
{
    auto sorted = results;
    std::sort(sorted.begin(), sorted.end(),
              [](const TorrentSearchResult &a, const TorrentSearchResult &b) { return a.seeders > b.seeders; });
    QSet<QString> seen = currentResultKeys();
    for (const auto &r : sorted) {
        const QString key = resultDedupeKey(r.magnet, r.name, static_cast<qlonglong>(r.size));
        if (seen.contains(key)) continue;
        seen.insert(key);
        QVariantMap m;
        m["name"] = r.name;
        m["sub"] = r.provider;
        m["provider"] = r.provider;
        m["sizeStr"] = r.size > 0 ? formatSize(r.size) : QString();
        m["seeds"] = QString::number(r.seeders);
        m["leech"] = QString::number(r.leechers);
        m["hasSeeds"] = r.seeders > 0;
        m["repacker"] = detectRepacker(r.name);
        m["poster"] = ""; m["coverHash"] = r.infoHash;
        m["seedsN"] = r.seeders; m["sizeBytes"] = static_cast<qlonglong>(r.size);
        fillMediaAttrs(m, r.name);
        m_results << m;
        m_resultMagnets << r.magnet;
        m_resultTitles << QString();        // torrent rows have no game cover hint
    }
    emit resultsChanged();
}

void QmlSearchBridge::finishAggregateSource()
{
    if (--m_pendingSources > 0) return;
    setSearching(false);
    setStatus(tr_("search_results_n").arg(m_results.size()));
    if (m_gwActive) gwResolve();
}

void QmlSearchBridge::runGameSearch(const QString &query)
{
    m_results.clear();
    m_resultMagnets.clear();
    m_resultTitles.clear();
    m_gameCache = GameSourceManager::instance().search(query);
    appendGameRows(m_gameCache);
    setSearching(false);
    setStatus(tr_("search_results_n").arg(m_results.size()));
}

QVariantList QmlSearchBridge::gameSources() const
{
    QVariantList out;
    for (const auto &s : GameSourceManager::instance().sources()) {
        QVariantMap m; m["name"] = s.first; m["url"] = s.second; out << m;
    }
    return out;
}

void QmlSearchBridge::addGameSource(const QString &name, const QString &url)
{
    auto &gsm = GameSourceManager::instance();
    gsm.addSource(name, url);
    emit gameSourcesChanged();
    if (!gsm.sources().isEmpty()) { setStatus(tr_("search_loading_game_catalogs")); gsm.refresh(false); }
}

void QmlSearchBridge::removeGameSource(const QString &url)
{
    auto &gsm = GameSourceManager::instance();
    gsm.removeSource(url);
    emit gameSourcesChanged();
    gsm.refresh(false);   // re-index remaining from cache
}

void QmlSearchBridge::refreshGames()
{
    if (GameSourceManager::instance().sources().isEmpty()) { emit gameSourcesChanged(); return; }
    setStatus(tr_("search_loading_game_catalogs"));
    GameSourceManager::instance().refresh(true);   // manual refresh → bypass cache
}

void QmlSearchBridge::activateResult(int index)
{
    auto &mgr = AddonManager::instance();
    if (m_mode == "titles") {
        if (index < 0 || index >= m_results.size()) return;
        const QVariantMap w = m_results[index].toMap();
        m_fromTitles = true;
        searchSourcesForWork(w.value(QStringLiteral("name")).toString(),
                             w.value(QStringLiteral("year")).toString(),
                             w.value(QStringLiteral("type")).toString());
        return;
    }
    if (m_mode == "catalog") {
        if (index < 0 || index >= m_catalogCache.size()) return;
        const auto &it = m_catalogCache[index];
        // Carry the catalog item's clean title + type into the stream add, so the
        // cover resolves from Stremio's metadata, not the messy torrent title.
        m_streamHintTitle = it.year > 0 ? QString("%1 %2").arg(it.name).arg(it.year) : it.name;
        m_streamHintType = it.type == QLatin1String("series") ? static_cast<int>(ContentType::Series)
                         : it.type == QLatin1String("movie")  ? static_cast<int>(ContentType::Movie) : -1;
        m_streamHintPoster = it.poster;
        setMode("streams");
        m_results.clear();
        emit resultsChanged();
        if (!mgr.hasStreamAddon()) { setStatus(tr_("search_no_stream_addon")); return; }
        setSearching(true);
        setStatus(tr_("search_loading_streams_from").arg(it.name));
        mgr.getStreams(it.type, it.id);
    } else if (m_mode == "streams") {
        if (index < 0 || index >= m_streamCache.size()) return;
        const auto &s = m_streamCache[index];
        if (s.magnet.startsWith("magnet:")) {
            m_session->addMagnet(s.magnet, m_savePath, m_streamHintTitle, m_streamHintType);
            setStatus(tr_("search_added_name").arg(s.title));
            emit addedTorrent(btihFromMagnet(s.magnet));
        }
    } else {   // torrent / games / all → every flat row carries its own magnet
        if (index < 0 || index >= m_resultMagnets.size()) return;
        const QString magnet = m_resultMagnets[index];
        if (magnet.isEmpty()) return;
        const QString hint = index < m_resultTitles.size() ? m_resultTitles[index] : QString();
        const int type = hint.isEmpty() ? -1 : static_cast<int>(ContentType::Game);
        m_session->addMagnet(magnet, m_savePath, hint, type);   // hint = clean game title, "" for torrents
        const QVariantMap rm = index < m_results.size() ? m_results[index].toMap() : QVariantMap();
        const QString name = rm.value(QStringLiteral("name")).toString();
        setStatus(name.isEmpty() ? tr_("search_added") : tr_("search_added_name").arg(name));
        QString hash = rm.value(QStringLiteral("coverHash")).toString();   // torrent rows carry the hash
        if (hash.isEmpty()) hash = btihFromMagnet(magnet);
        emit addedTorrent(hash);
    }
}

void QmlSearchBridge::back()
{
    if (m_fromTitles && m_mode != "streams") {   // sources view → back to the titles grid
        m_fromTitles = false;
        m_aggregate = false;
        m_results = m_titleCache;
        m_resultMagnets.clear();
        m_resultTitles.clear();
        setMode("titles");
        setSearching(false);
        setStatus(tr_("search_titles_n").arg(m_results.size()));
        emit resultsChanged();
        return;
    }
    if (m_mode != "streams") return;
    setMode("catalog");
    m_results.clear();
    for (const auto &it : m_catalogCache) {
        QVariantMap m;
        m["name"] = it.name;
        m["sub"] = it.type;
        m["sizeStr"] = it.year > 0 ? QString::number(it.year) : QString();
        m["seeds"] = ""; m["leech"] = ""; m["repacker"] = "";
        m["poster"] = it.poster; m["coverHash"] = "";
        m["seedsN"] = 0; m["sizeBytes"] = 0;
        fillMediaAttrs(m, it.name);
        m_results << m;
    }
    emit resultsChanged();
    setStatus(tr_("search_results_n").arg(m_catalogCache.size()));
}

// ===================== QmlAddonBridge =====================

