// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/qmlsearchbridge.h"
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
#include "webui/webserver.h"
#include <QCryptographicHash>
#include <QStorageInfo>
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
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QUrl>
#include <algorithm>
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
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/version.hpp>
#include <sstream>
#include <openssl/opensslv.h>
#include <boost/version.hpp>

QmlSearchBridge::QmlSearchBridge(IEngine *session, QObject *parent)
    : QObject(parent), m_session(session), m_mode("torrent")
{
    auto &mgr = AddonManager::instance();
    connect(&mgr, &AddonManager::catalogResults, this, [this](const QList<CatalogItem> &items) {
        m_catalogCache = items;
        if (m_mode != "catalog") return;
        rebuildCatalogRows();
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
            m["seeds"] = ""; m["leech"] = ""; m["releaseGroup"] = "";
            m["poster"] = m_streamHintPoster; m["coverHash"] = "";
            m["quality"] = s.quality;
            m["seedsN"] = 0; m["sizeBytes"] = s.size;
            fillMediaAttrs(m, s.title);
            fillTrust(m, s.title);
            m_results << m;
        }
        emit resultsChanged();
    });
    connect(&mgr, &AddonManager::streamFinished, this, [this]() {
        setSearching(false);
        setStatus(tr_("search_streams_n").arg(m_streamCache.size()));
    });
    connect(&mgr, &AddonManager::metaVideos, this, [this](const QString &id, const QVariantList &videos) {
        if (m_mode != "episodes" || id != m_epId) return;   // stale or user moved on
        setSearching(false);
        if (videos.isEmpty()) {   // no episode meta → old bare-id lookup is better than nothing
            setMode("streams");
            auto &am = AddonManager::instance();
            if (!am.hasStreamAddon()) { setStatus(tr_("search_no_stream_addon")); return; }
            setSearching(true);
            setStatus(tr_("search_loading_streams_from").arg(m_streamHintTitle));
            am.getStreams(m_epType, m_epId);
            return;
        }
        m_episodeCache = videos;
        showEpisodeRows();
    });
    connect(&mgr, &AddonManager::torrentSearchResults, this,
            [this](const QList<TorrentSearchResult> &results) {
        if (m_mode != "torrent" && m_mode != "games" && m_mode != "all") return;
        if (!m_aggregate) {   // single source replaces; aggregate appends each batch
            m_results.clear(); m_resultMagnets.clear(); m_resultHttp.clear(); m_torrentCache.clear();
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

namespace {
// Two titles are "the same" when they differ only by accents, case or
// punctuation — "Shang-Chi" vs "Shang Chi". Without folding, an English app
// would fire the identical query twice for every English film.
bool sameTitle(const QString &a, const QString &b)
{
    auto fold = [](const QString &s) {
        const QString d = s.normalized(QString::NormalizationForm_D).toLower();
        QString out;
        for (const QChar &c : d)
            if (c.isLetterOrNumber() && c.category() != QChar::Mark_NonSpacing)
                out.append(c);
        return out;
    };
    return fold(a) == fold(b);
}
}

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

void QmlSearchBridge::setResolver(MetadataResolver *r)
{
    m_resolver = r;
    if (!m_resolver) return;
    // Poster fills mutate m_results WITHOUT resultsChanged() on purpose: QML
    // treats that signal as "new result set" (closes the detail panel, resets
    // the view); delegates repaint targeted via coverReady instead.
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

void QmlSearchBridge::setWorkContext(const QVariantMap &work)
{
    m_workType = work.value(QStringLiteral("type")).toString();
    m_workTitle = work.value(QStringLiteral("title")).toString();
    if (m_workTitle.isEmpty()) m_workTitle = work.value(QStringLiteral("name")).toString();
    m_workPoster = work.value(QStringLiteral("poster")).toString();
    m_workYear = work.value(QStringLiteral("year")).toString();
    m_workTmdbId = work.value(QStringLiteral("tmdbId")).toInt();
    m_workStills = work.value(QStringLiteral("stills")).toStringList();
    m_workStillsRequested = false;
    emit workChanged();
    emit workStillsChanged();
}

void QmlSearchBridge::clearWorkContext()
{
    if (m_workType.isEmpty() && m_workTitle.isEmpty() && m_workTmdbId == 0 && m_workStills.isEmpty()) return;
    m_workType.clear();
    m_workTitle.clear();
    m_workPoster.clear();
    m_workYear.clear();
    m_workTmdbId = 0;
    m_workStills.clear();
    m_workStillsRequested = false;
    emit workChanged();
    emit workStillsChanged();
}

void QmlSearchBridge::fetchWorkStills()
{
    if (m_workStillsRequested || !m_workStills.isEmpty()) return;   // inline (games) or already asked
    if (m_workTmdbId <= 0 || !m_discovery) return;
    m_workStillsRequested = true;
    m_discovery->fetchBackdrops(m_workTmdbId, m_workType);
}

void QmlSearchBridge::setDiscovery(DiscoveryService *d)
{
    m_discovery = d;
    if (!m_discovery) return;
    connect(m_discovery, &DiscoveryService::backdropsReady, this,
            [this](int tmdbId, const QStringList &urls) {
        if (tmdbId != m_workTmdbId || urls.isEmpty()) return;   // stale reply for a former title
        m_workStills = urls;
        emit workStillsChanged();
    });
    connect(m_discovery, &DiscoveryService::titleResults, this,
            [this](const QString &query, const QVariantList &works) {
        if (query != m_titleQuery || m_mode != QLatin1String("titles")) return;   // stale
        m_results.clear();
        m_resultMagnets.clear();
        m_resultTitles.clear();
    m_resultHttp.clear();
        for (const QVariant &v : works) {
            const QVariantMap w = v.toMap();
            QVariantMap row;
            row["name"]    = w.value(QStringLiteral("title"));
            row["title"]   = w.value(QStringLiteral("title"));
            row["originalTitle"] = w.value(QStringLiteral("originalTitle"));
            row["sub"]     = w.value(QStringLiteral("type"));
            row["sizeStr"] = w.value(QStringLiteral("year"));
            row["year"]    = w.value(QStringLiteral("year"));
            row["type"]    = w.value(QStringLiteral("type"));
            row["poster"]  = w.value(QStringLiteral("poster"));
            row["rating"]  = w.value(QStringLiteral("rating"));
            row["overview"] = w.value(QStringLiteral("overview"));
            row["tmdbId"]  = w.value(QStringLiteral("tmdbId"));
            row["stills"]  = w.value(QStringLiteral("stills"));
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

void QmlSearchBridge::searchSourcesForWork(const QString &title, const QString &year,
                                           const QString &type, const QString &originalTitle)
{
    m_results.clear();
    m_resultMagnets.clear();
    m_resultTitles.clear();
    m_resultHttp.clear();
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
    // A work is released under different names per language, and the uploader
    // picks one: a Portuguese dub is "Shang-Chi e a Lenda dos Dez Anéis", the
    // original is "Shang-Chi and the Legend of the Ten Rings". Since our TMDB
    // requests carry language=, `title` is already localised — so searching it
    // alone found the dubs and missed everything published under the original
    // name (usually the majority, and the better-seeded half). Hunt under both.
    //
    // Capped at two on purpose. Adding TMDB's alternative_titles here would
    // multiply requests per provider, and on a private tracker that is how an
    // account gets banned.
    QStringList titles{ title };
    if (!originalTitle.isEmpty() && !sameTitle(originalTitle, title))
        titles << originalTitle;
    m_activeTitles = titles;

    const int cat = isGame ? 400 : 200;   // 400 = games, 200 = video
    const auto providers = mgr.searchProviders();
    for (const QString &t : titles) {
        // movies disambiguate well with a year; games/series search cleaner by title
        const QString q = (type == QLatin1String("movie") && !year.isEmpty())
                        ? t + QLatin1Char(' ') + year : t;
        for (int i = 0; i < providers.size(); ++i)
            if (providers[i].enabled) { ++m_pendingSources; mgr.searchWithProvider(i, q, cat); }
        if (mgr.torrentSearchEnabled()) { ++m_pendingSources; mgr.searchTorrents(q, cat); }
    }
    if (m_pendingSources == 0) {
        setSearching(false);
        setStatus(tr_("search_results_n").arg(m_results.size()));
    }
}

static QString btihFromMagnet(const QString &magnet);   // defined below

QStringList QmlSearchBridge::queryWords() const
{
    return SearchRanker::significantWords(m_activeQuery);
}

int QmlSearchBridge::relevance(const QString &name, const QStringList &words) const
{
    return SearchRanker::relevanceScore(name, words);
}

QVariantList QmlSearchBridge::queryWordSets() const
{
    QVariantList out;
    // the names this drill-down was actually searched under; falls back to the
    // typed query for a flat/raw search, where there is no picked work
    const QStringList titles = m_activeTitles.isEmpty() ? QStringList{ m_activeQuery }
                                                        : m_activeTitles;
    for (const QString &t : titles) {
        const QStringList w = SearchRanker::significantWords(t);
        if (!w.isEmpty()) out << QVariant(w);
    }
    return out;
}

int QmlSearchBridge::relevanceMulti(const QString &name, const QVariantList &sets) const
{
    QList<QStringList> ws;
    ws.reserve(sets.size());
    for (const QVariant &v : sets) ws << v.toStringList();
    return SearchRanker::bestRelevance(name, ws);
}

static GameReleasePick::Candidate gameCandFromRow(const QVariantMap &m, bool hasUri)
{
    return { m.value(QStringLiteral("fromCatalog")).toBool(),
             m.value(QStringLiteral("version")).toString(),
             m.value(QStringLiteral("uploadDate")).toString(),
             m.value(QStringLiteral("seedsN")).toInt(),
             hasUri };
}

int QmlSearchBridge::pickBestResult() const
{
    if (m_isGameSearch || m_workType == QLatin1String("game")) {
        QList<GameReleasePick::Candidate> cands;
        cands.reserve(m_results.size());
        for (int i = 0; i < m_results.size(); ++i) {
            const bool hasUri = (i < m_resultMagnets.size() && !m_resultMagnets[i].isEmpty())
                             || (i < m_resultHttp.size() && !m_resultHttp[i].isEmpty());
            cands.append(gameCandFromRow(m_results[i].toMap(), hasUri));
        }
        return GameReleasePick::best(cands);
    }

    QList<ReleasePick::Candidate> cands;
    cands.reserve(m_results.size());
    for (const QVariant &v : m_results) {
        const QVariantMap m = v.toMap();
        const QString mode = m.value(QStringLiteral("audioMode")).toString();
        const int audioRank = mode == QLatin1String("dub") ? 2
                            : mode == QLatin1String("sub") ? 1 : 0;
        cands.append({ m.value(QStringLiteral("quality")).toString(),
                       m.value(QStringLiteral("native")).toBool() || audioRank > 0,
                       audioRank,
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

int QmlSearchBridge::compareBuildVersions(const QString &a, const QString &b) const
{
    return GameReleasePick::compareVersions(a, b);
}

int QmlSearchBridge::compareGameReleases(const QVariantMap &a, const QVariantMap &b) const
{
    // strcmp-style: positive means a ranks above b (matches GameReleasePick::compareCandidates).
    return GameReleasePick::compareCandidates(
        gameCandFromRow(a, a.value(QStringLiteral("hasUri"), true).toBool()),
        gameCandFromRow(b, b.value(QStringLiteral("hasUri"), true).toBool()));
}

void QmlSearchBridge::getAndWatch(const QString &title, const QString &year, const QString &type)
{
    m_gwActive = true;
    m_gwCancelled = false;
    m_gwTitle = title;
    m_gwType = type.isEmpty() ? QStringLiteral("movie") : type;
    emit getFlowChanged();
    emit watchSearching(title);
    setWorkContext({ { QStringLiteral("title"), title },
                     { QStringLiteral("type"), type },
                     { QStringLiteral("year"), year } });
    searchSourcesForWork(title, year, type);      // gwResolve() runs when this settles
    // If the search had nothing to wait on (no providers), it finished inline.
    if (m_gwActive && !m_searching) gwResolve();
}

void QmlSearchBridge::cancelGetAndWatch()
{
    m_gwActive = false;
    m_gwCancelled = true;
    emit getFlowChanged();
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
    emit getFlowChanged();
    if (m_gwCancelled) { m_gwCancelled = false; return; }   // user backed out during the search

    auto hasMagnet = [this](int i) {
        return i >= 0 && i < m_resultMagnets.size() && !m_resultMagnets[i].isEmpty();
    };

    // Prefer the ranked pick; if it's HTTP-only, re-rank among magnet rows so
    // Get & Install / Get & Watch always get an info-hash they can poll.
    int idx = pickBestResult();
    if (!hasMagnet(idx)
        && (m_gwType == QLatin1String("game") || m_isGameSearch
            || m_workType == QLatin1String("game"))) {
        QList<GameReleasePick::Candidate> cands;
        QList<int> idxs;
        for (int i = 0; i < m_results.size(); ++i) {
            if (!hasMagnet(i)) continue;
            idxs.append(i);
            cands.append(gameCandFromRow(m_results[i].toMap(), true));
        }
        const int local = GameReleasePick::best(cands);
        idx = (local >= 0 && local < idxs.size()) ? idxs[local] : -1;
    }
    if (!hasMagnet(idx)) {
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
    } else if (m_gwType == QLatin1String("game")) {
        type = static_cast<int>(ContentType::Game);
        if (hint.isEmpty()) hint = m_gwTitle;
    }
    m_session->addMagnet(magnet, m_savePath, hint, type);
    QString hash = rm.value(QStringLiteral("coverHash")).toString();
    if (hash.isEmpty()) hash = btihFromMagnet(magnet);
    if (m_gwType == QLatin1String("game"))
        emit prepareAndInstall(hash, m_gwTitle);
    else
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

QString QmlSearchBridge::magnetAt(int index) const
{
    if (index < 0 || index >= m_resultMagnets.size()) return {};
    return m_resultMagnets[index];
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
    m_resultHttp.clear();
    m_torrentCache.clear();
    m_gameCache.clear();
    m_pendingGameQuery.clear();
    emit resultsChanged();

    m_activeQuery = q;
    clearWorkContext();
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
bool QmlSearchBridge::canGoBack() const { return m_mode == "streams" || m_mode == "episodes" || m_fromTitles; }
bool QmlSearchBridge::singleTitleView() const { return m_titleSources || m_mode == "streams" || m_mode == "episodes"; }
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
    m_fromEpisodes = false;
    clearWorkContext();
    m_pendingGameQuery.clear();
    m_results.clear();
    m_resultMagnets.clear();
    m_resultTitles.clear();
    m_resultHttp.clear();
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
    // Prefer libtorrent's parser so Base32 xt=urn:btih: tokens become the same
    // hex info-hash SessionManager indexes — otherwise installWhenReady /
    // watchWhenReady poll a hash that never matches and time out.
    lt::error_code ec;
    const lt::add_torrent_params atp = lt::parse_magnet_uri(magnet.toStdString(), ec);
    if (!ec) {
        const QString hex = QString::fromStdString(
            (std::ostringstream() << atp.info_hashes.get_best()).str());
        if (!hex.isEmpty()) return hex;
    }
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
        m["releaseGroup"] = detectReleaseGroup(g.title);
        const QString ih = infoHashFromMagnet(g.magnet);
        m["poster"] = ""; m["coverHash"] = ih;
        m["seedsN"] = 0;
        m["sizeBytes"] = parseSizeToBytes(g.fileSize);
        m["fromCatalog"] = true;
        m["uploadDate"] = g.uploadDate;
        m["hasUri"] = !g.magnet.isEmpty() || !g.httpUrl.isEmpty();
        fillMediaAttrs(m, g.title);
        fillTrust(m, g.title);
        m_results << m;
        m_resultMagnets << g.magnet;
        m_resultHttp << g.httpUrl;
        m_resultTitles << (g.cleanTitle.isEmpty() ? g.title : g.cleanTitle);
    }
    emit resultsChanged();
}

void QmlSearchBridge::appendTorrentRows(const QList<TorrentSearchResult> &results)
{
    auto sorted = results;
    std::sort(sorted.begin(), sorted.end(),
              [](const TorrentSearchResult &a, const TorrentSearchResult &b) { return a.seeders > b.seeders; });
    // Season/episode grouping only makes sense inside one picked series' releases.
    const bool groupEpisodes = m_titleSources && m_workType == QLatin1String("series");
    QSet<QString> seen = currentResultKeys();
    for (const auto &r : sorted) {
        const QString key = resultDedupeKey(r.magnet, r.name, static_cast<qlonglong>(r.size));
        if (seen.contains(key)) continue;
        seen.insert(key);
        QVariantMap m;
        m["name"] = r.name;
        if (groupEpisodes) {
            const EpisodeTag tag = EpisodeGroup::classify(r.name);
            m["season"] = tag.season;
            m["episode"] = tag.episode;
            m["pack"] = tag.pack;
        }
        m["sub"] = r.provider;
        m["provider"] = r.provider;
        m["sizeStr"] = r.size > 0 ? formatSize(r.size) : QString();
        m["seeds"] = QString::number(r.seeders);
        m["leech"] = QString::number(r.leechers);
        m["hasSeeds"] = r.seeders > 0;
        m["releaseGroup"] = detectReleaseGroup(r.name);
        QString ih = r.infoHash.toLower();
        if (ih.size() != 40)
            ih = infoHashFromMagnet(r.magnet);
        m["poster"] = ""; m["coverHash"] = ih;
        m["seedsN"] = r.seeders; m["sizeBytes"] = static_cast<qlonglong>(r.size);
        m["fromCatalog"] = false;
        m["uploadDate"] = QString();
        m["hasUri"] = !r.magnet.isEmpty();
        fillMediaAttrs(m, r.name);
        fillTrust(m, r.name);
        m_results << m;
        m_resultMagnets << r.magnet;
        m_resultHttp << QString();          // torrent rows download via magnet
        m_resultTitles << QString();        // torrent rows have no game cover hint
    }
    mergeCatalogSwarms();
    emit resultsChanged();
}

void QmlSearchBridge::finishAggregateSource()
{
    if (--m_pendingSources > 0) return;
    setSearching(false);
    mergeCatalogSwarms();
    setStatus(tr_("search_results_n").arg(m_results.size()));
    requestCatalogSeedEnrichment();
    if (m_gwActive) gwResolve();
}

QString QmlSearchBridge::infoHashFromMagnet(const QString &magnet)
{
    static const QRegularExpression re(QStringLiteral("btih:([0-9A-Fa-f]{40})"),
                                       QRegularExpression::CaseInsensitiveOption);
    const auto m = re.match(magnet);
    return m.hasMatch() ? m.captured(1).toLower() : QString();
}

qint64 QmlSearchBridge::parseSizeToBytes(const QString &s)
{
    const QString t = s.trimmed();
    if (t.isEmpty()) return 0;
    bool ok = false;
    // raw integer bytes
    if (!t.contains(QLatin1Char(' ')) && !t.contains(QLatin1Char('.')) && t.at(0).isDigit()) {
        const qint64 n = t.toLongLong(&ok);
        if (ok && n > 0) return n;
    }
    static const QRegularExpression re(
        QStringLiteral(R"(^\s*([\d]+(?:[.,]\d+)?)\s*(B|KB|MB|GB|TB)\s*$)"),
        QRegularExpression::CaseInsensitiveOption);
    const auto m = re.match(t);
    if (!m.hasMatch()) return 0;
    QString num = m.captured(1);
    num.replace(QLatin1Char(','), QLatin1Char('.'));
    const double v = num.toDouble(&ok);
    if (!ok || v < 0) return 0;
    const QString u = m.captured(2).toUpper();
    double mul = 1;
    if (u == QLatin1String("KB")) mul = 1024.0;
    else if (u == QLatin1String("MB")) mul = 1024.0 * 1024.0;
    else if (u == QLatin1String("GB")) mul = 1024.0 * 1024.0 * 1024.0;
    else if (u == QLatin1String("TB")) mul = 1024.0 * 1024.0 * 1024.0 * 1024.0;
    return static_cast<qint64>(v * mul + 0.5);
}

void QmlSearchBridge::applySeedHits(const QHash<QString, QPair<int, int>> &byHash)
{
    if (byHash.isEmpty()) return;
    bool changed = false;
    for (int i = 0; i < m_results.size(); ++i) {
        QVariantMap m = m_results[i].toMap();
        if (!m.value(QStringLiteral("fromCatalog")).toBool()) continue;
        QString h = m.value(QStringLiteral("coverHash")).toString().toLower();
        if (h.size() != 40 && i < m_resultMagnets.size())
            h = infoHashFromMagnet(m_resultMagnets[i]);
        if (!byHash.contains(h)) continue;
        const auto hit = byHash.value(h);
        if (hit.first <= m.value(QStringLiteral("seedsN")).toInt()) continue;
        m[QStringLiteral("seedsN")] = hit.first;
        m[QStringLiteral("seeds")] = QString::number(hit.first);
        m[QStringLiteral("leech")] = QString::number(hit.second);
        m[QStringLiteral("hasSeeds")] = hit.first > 0;
        m_results[i] = m;
        changed = true;
    }
    if (changed) emit resultsChanged();
}

void QmlSearchBridge::mergeCatalogSwarms()
{
    QSet<QString> catalogHashes;
    QHash<QString, QPair<int, int>> swarm;   // hash → (seeds, leech)
    QHash<QString, qint64> sizes;

    for (int i = 0; i < m_results.size(); ++i) {
        const QVariantMap m = m_results[i].toMap();
        QString h = m.value(QStringLiteral("coverHash")).toString().toLower();
        if (h.size() != 40 && i < m_resultMagnets.size())
            h = infoHashFromMagnet(m_resultMagnets[i]);
        if (h.size() != 40) continue;

        if (m.value(QStringLiteral("fromCatalog")).toBool()) {
            catalogHashes.insert(h);
            continue;
        }
        const int seeds = m.value(QStringLiteral("seedsN")).toInt();
        const int leech = m.value(QStringLiteral("leech")).toString().toInt();
        if (!swarm.contains(h) || seeds > swarm.value(h).first)
            swarm.insert(h, {seeds, leech});
        const qint64 sz = m.value(QStringLiteral("sizeBytes")).toLongLong();
        if (sz > 0) sizes.insert(h, sz);
    }
    if (catalogHashes.isEmpty()) return;

    applySeedHits(swarm);

    // Prefer the catalog row: drop indexer duplicates of the same magnet.
    QList<int> drop;
    for (int i = 0; i < m_results.size(); ++i) {
        const QVariantMap m = m_results[i].toMap();
        if (m.value(QStringLiteral("fromCatalog")).toBool()) {
            QString h = m.value(QStringLiteral("coverHash")).toString().toLower();
            if (h.size() != 40 && i < m_resultMagnets.size())
                h = infoHashFromMagnet(m_resultMagnets[i]);
            if (sizes.contains(h) && m.value(QStringLiteral("sizeBytes")).toLongLong() <= 0) {
                QVariantMap mm = m;
                mm[QStringLiteral("sizeBytes")] = sizes.value(h);
                mm[QStringLiteral("sizeStr")] = formatSize(sizes.value(h));
                m_results[i] = mm;
            }
            continue;
        }
        QString h = m.value(QStringLiteral("coverHash")).toString().toLower();
        if (h.size() != 40 && i < m_resultMagnets.size())
            h = infoHashFromMagnet(m_resultMagnets[i]);
        if (catalogHashes.contains(h))
            drop.append(i);
    }
    if (drop.isEmpty()) {
        emit resultsChanged();
        return;
    }
    std::sort(drop.begin(), drop.end(), std::greater<int>());
    for (int i : drop) {
        m_results.removeAt(i);
        if (i < m_resultMagnets.size()) m_resultMagnets.removeAt(i);
        if (i < m_resultHttp.size()) m_resultHttp.removeAt(i);
        if (i < m_resultTitles.size()) m_resultTitles.removeAt(i);
    }
    emit resultsChanged();
}

void QmlSearchBridge::requestCatalogSeedEnrichment()
{
    QStringList queries;
    const QString titled = !m_workTitle.trimmed().isEmpty() ? m_workTitle.trimmed()
                         : m_activeQuery.trimmed();
    if (!titled.isEmpty()) {
        queries << titled;
    } else {
        // Catalog browse page: budget a few title lookups (BitSearch rate limits).
        int budget = 8;
        for (const QVariant &v : m_results) {
            if (budget <= 0) break;
            const QVariantMap m = v.toMap();
            if (!m.value(QStringLiteral("fromCatalog")).toBool()) continue;
            if (m.value(QStringLiteral("seedsN")).toInt() > 0) continue;
            if (m.value(QStringLiteral("coverHash")).toString().size() != 40) continue;
            const QString name = m.value(QStringLiteral("name")).toString().trimmed();
            if (name.isEmpty() || queries.contains(name)) continue;
            queries << name;
            --budget;
        }
    }
    if (queries.isEmpty()) return;

    bool need = false;
    for (const QVariant &v : m_results) {
        const QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("fromCatalog")).toBool()
            && m.value(QStringLiteral("seedsN")).toInt() <= 0
            && m.value(QStringLiteral("coverHash")).toString().size() == 40) {
            need = true;
            break;
        }
    }
    if (!need) return;

    const int gen = ++m_seedEnrichGen;
    auto *nam = new QNetworkAccessManager(this);
    int *pending = new int(queries.size());
    for (const QString &q : queries) {
        const QUrl url(QStringLiteral("https://bitsearch.eu/api/v1/search?q=%1&limit=50")
                           .arg(QString::fromUtf8(QUrl::toPercentEncoding(q))));
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent/2.0"));
        req.setTransferTimeout(12000);
        QNetworkReply *reply = nam->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply, nam, pending, gen]() {
            reply->deleteLater();
            if (gen == m_seedEnrichGen && reply->error() == QNetworkReply::NoError) {
                const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
                const QJsonArray arr = root.value(QStringLiteral("results")).toArray();
                QHash<QString, QPair<int, int>> byHash;
                for (const QJsonValue &v : arr) {
                    const QJsonObject o = v.toObject();
                    QString h = o.value(QStringLiteral("infohash")).toString().toLower();
                    if (h.size() != 40)
                        h = o.value(QStringLiteral("info_hash")).toString().toLower();
                    if (h.size() != 40) continue;
                    const int seeds = o.value(QStringLiteral("seeders")).toInt();
                    const int leech = o.value(QStringLiteral("leechers")).toInt();
                    if (!byHash.contains(h) || seeds > byHash.value(h).first)
                        byHash.insert(h, {seeds, leech});
                }
                applySeedHits(byHash);
            }
            if (--(*pending) == 0) {
                delete pending;
                nam->deleteLater();
            }
        });
    }
}

void QmlSearchBridge::runGameSearch(const QString &query)
{
    m_results.clear();
    m_resultMagnets.clear();
    m_resultTitles.clear();
    m_resultHttp.clear();
    m_gameCache = GameSourceManager::instance().search(query);
    appendGameRows(m_gameCache);
    setSearching(false);
    setStatus(tr_("search_results_n").arg(m_results.size()));
    requestCatalogSeedEnrichment();
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

void QmlSearchBridge::ensureGamesIndexed()
{
    auto &gsm = GameSourceManager::instance();
    if (gsm.gameCount() > 0 || gsm.sources().isEmpty())
        return;
    setStatus(tr_("search_loading_game_catalogs"));
    gsm.refresh(false);
}

void QmlSearchBridge::browseGames(const QString &group, int page, int pageSize)
{
    auto &gsm = GameSourceManager::instance();
    if (gsm.gameCount() == 0 && !gsm.sources().isEmpty()) {
        ensureGamesIndexed();
        return;   // refreshed signal → QML retries browse
    }

    const int size = pageSize > 0 ? pageSize : 48;
    const int p = page < 0 ? 0 : page;
    const int total = gsm.countByGroup(group);
    const int offset = p * size;

    clearWorkContext();
    m_fromTitles = false;
    m_titleSources = false;
    m_isGameSearch = true;
    m_aggregate = false;
    m_activeQuery.clear();
    m_lastQuery.clear();
    setMode(QStringLiteral("games"));

    m_results.clear();
    m_resultMagnets.clear();
    m_resultTitles.clear();
    m_resultHttp.clear();
    m_gameCache = gsm.browse(group, offset, size);
    appendGameRows(m_gameCache);
    setSearching(false);

    if (total <= 0) {
        setStatus(tr_("find_catalog_empty"));
        return;
    }
    const int from = offset + 1;
    const int to = qMin(offset + m_results.size(), total);
    setStatus(tr_("find_catalog_showing").arg(from).arg(to).arg(total));
    requestCatalogSeedEnrichment();
}

int QmlSearchBridge::gameBrowseTotal(const QString &group) const
{
    return GameSourceManager::instance().countByGroup(group);
}

QVariantList QmlSearchBridge::gameRepackTabs() const
{
    return GameSourceManager::instance().groupCounts();
}

bool QmlSearchBridge::fitsOnSaveVolume(qint64 needed) const
{
    if (needed <= 0) return true;   // unknown size — don't block
    const QStorageInfo si(m_savePath);
    return !si.isValid() || needed <= si.bytesAvailable();
}

void QmlSearchBridge::activateResult(int index, bool force)
{
    auto &mgr = AddonManager::instance();
    if (m_mode == "titles") {
        if (index < 0 || index >= m_results.size()) return;
        const QVariantMap w = m_results[index].toMap();
        m_fromTitles = true;
        setWorkContext(w);
        searchSourcesForWork(w.value(QStringLiteral("name")).toString(),
                             w.value(QStringLiteral("year")).toString(),
                             w.value(QStringLiteral("type")).toString(),
                             w.value(QStringLiteral("originalTitle")).toString());
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
        // Series streams need an "id:season:episode" — a bare series id returns
        // nothing from most addons. Route through the episode picker when the
        // addon exposes meta; otherwise keep the old direct lookup.
        if (it.type == QLatin1String("series") && mgr.hasMetaAddon()) {
            m_epType = it.type;
            m_epId = it.id;
            m_fromEpisodes = false;
            setMode("episodes");
            m_results.clear();
            m_episodeCache.clear();
            emit resultsChanged();
            setSearching(true);
            setStatus(tr_("search_loading_episodes"));
            mgr.fetchMeta(it.type, it.id);
            return;
        }
        setMode("streams");
        m_results.clear();
        emit resultsChanged();
        if (!mgr.hasStreamAddon()) { setStatus(tr_("search_no_stream_addon")); return; }
        setSearching(true);
        setStatus(tr_("search_loading_streams_from").arg(it.name));
        mgr.getStreams(it.type, it.id);
    } else if (m_mode == "episodes") {
        if (index < 0 || index >= m_episodeCache.size()) return;
        const QVariantMap ep = m_episodeCache[index].toMap();
        const QString videoId = ep.value(QStringLiteral("videoId")).toString();
        if (videoId.isEmpty()) return;
        m_fromEpisodes = true;
        setMode("streams");
        m_results.clear();
        emit resultsChanged();
        if (!mgr.hasStreamAddon()) { setStatus(tr_("search_no_stream_addon")); return; }
        setSearching(true);
        setStatus(tr_("search_loading_streams_from").arg(ep.value(QStringLiteral("name")).toString()));
        mgr.getStreams(m_epType, videoId);
    } else if (m_mode == "streams") {
        if (index < 0 || index >= m_streamCache.size()) return;
        const auto &s = m_streamCache[index];
        if (s.magnet.startsWith("magnet:")) {
            m_session->addMagnet(s.magnet, m_savePath, m_streamHintTitle, m_streamHintType);
            setStatus(tr_("search_added_name").arg(s.title));
            emit addedTorrent(btihFromMagnet(s.magnet));
        }
    } else {   // torrent / games / all → each flat row carries a magnet OR an http url
        if (index < 0 || index >= m_resultMagnets.size()) return;
        const QString magnet = m_resultMagnets[index];
        const QString httpUrl = index < m_resultHttp.size() ? m_resultHttp[index] : QString();
        if (magnet.isEmpty() && httpUrl.isEmpty()) return;
        const QVariantMap rm = index < m_results.size() ? m_results[index].toMap() : QVariantMap();
        const QString name = rm.value(QStringLiteral("name")).toString();
        const qint64 needed = rm.value(QStringLiteral("sizeBytes")).toLongLong();
        if (!force && !fitsOnSaveVolume(needed)) {
            const QStorageInfo si(m_savePath);
            emit addWontFit(index, name, needed, si.isValid() ? si.bytesAvailable() : 0);
            return;   // QML asks the user, then re-calls with force = true
        }
        const QString hint = index < m_resultTitles.size() ? m_resultTitles[index] : QString();
        const int type = hint.isEmpty() ? -1 : static_cast<int>(ContentType::Game);

        // A file-host-only source (no magnet) downloads directly over HTTP and
        // shows up in the Downloads list via the engine decorator.
        if (magnet.isEmpty()) {
            if (!m_httpDownloads) { setStatus(tr_("add_url_failed")); return; }
            const QString id = m_httpDownloads->add(bat::directDownloadUrl(QUrl(httpUrl)), m_savePath);
            // Resolve an IGDB cover keyed by the row's pseudo-hash, same as a
            // magnet game gets one via addMagnet's cover hint.
            if (!hint.isEmpty() && m_resolver)
                m_resolver->resolveManual(bat::httpRowHash(id), hint, ContentType::Game);
            setStatus(name.isEmpty() ? tr_("search_added") : tr_("search_added_name").arg(name));
            return;
        }
        m_session->addMagnet(magnet, m_savePath, hint, type);   // hint = clean game title, "" for torrents
        setStatus(name.isEmpty() ? tr_("search_added") : tr_("search_added_name").arg(name));
        QString hash = rm.value(QStringLiteral("coverHash")).toString();   // torrent rows carry the hash
        if (hash.isEmpty()) hash = btihFromMagnet(magnet);
        emit addedTorrent(hash);
    }
}

void QmlSearchBridge::back()
{
    if (m_fromTitles && m_mode != "streams" && m_mode != "episodes") {   // sources view → back to the titles grid
        m_fromTitles = false;
        m_aggregate = false;
        clearWorkContext();
        m_results = m_titleCache;
        m_resultMagnets.clear();
        m_resultTitles.clear();
    m_resultHttp.clear();
        setMode("titles");
        setSearching(false);
        setStatus(tr_("search_titles_n").arg(m_results.size()));
        emit resultsChanged();
        return;
    }
    if (m_mode == "streams" && m_fromEpisodes) {   // streams → episode picker
        m_fromEpisodes = false;
        showEpisodeRows();
        return;
    }
    if (m_mode != "streams" && m_mode != "episodes") return;
    m_fromEpisodes = false;
    setMode("catalog");
    rebuildCatalogRows();
    setStatus(tr_("search_results_n").arg(m_catalogCache.size()));
}

void QmlSearchBridge::rebuildCatalogRows()
{
    m_results.clear();
    for (const auto &it : std::as_const(m_catalogCache)) {
        QVariantMap m;
        m["name"] = it.name;
        m["sub"] = it.type;
        m["sizeStr"] = it.year > 0 ? QString::number(it.year) : QString();
        m["seeds"] = ""; m["leech"] = ""; m["releaseGroup"] = "";
        m["poster"] = it.poster; m["coverHash"] = "";
        m["seedsN"] = 0; m["sizeBytes"] = 0;
        fillMediaAttrs(m, it.name);
        m_results << m;
    }
    emit resultsChanged();
}

void QmlSearchBridge::showEpisodeRows()
{
    setMode("episodes");
    m_results.clear();
    m_resultMagnets.clear();
    m_resultTitles.clear();
    m_resultHttp.clear();
    for (const QVariant &v : std::as_const(m_episodeCache)) {
        const QVariantMap ep = v.toMap();
        QVariantMap m;
        m["name"] = ep.value(QStringLiteral("name"));
        m["sub"] = ""; m["provider"] = "";
        m["sizeStr"] = ep.value(QStringLiteral("released")).toString();
        m["seeds"] = ""; m["leech"] = ""; m["releaseGroup"] = "";
        m["poster"] = m_streamHintPoster; m["coverHash"] = "";
        m["seedsN"] = 0; m["sizeBytes"] = 0;
        m["season"] = ep.value(QStringLiteral("season"));
        m["episode"] = ep.value(QStringLiteral("episode"));
        m_results << m;
    }
    setStatus(tr_("search_episodes_n").arg(m_results.size()));
    emit resultsChanged();
}

