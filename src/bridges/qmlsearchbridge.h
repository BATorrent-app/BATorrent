// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef QMLSEARCHBRIDGE_H
#define QMLSEARCHBRIDGE_H

#include "bridges/bridgecommon.h"

class QmlSearchBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList sources READ sources NOTIFY sourcesChanged)
    Q_PROPERTY(QVariantList categories READ categories CONSTANT)
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
    Q_PROPERTY(QString activeQuery READ activeQuery NOTIFY resultsChanged) // for client-side relevance ranking
    Q_PROPERTY(QString mode READ mode NOTIFY modeChanged)        // titles|catalog|streams|torrent|games
    Q_PROPERTY(bool inStreams READ inStreams NOTIFY modeChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY modeChanged) // sources view reachable from a title/catalog
    Q_PROPERTY(bool singleTitleView READ singleTitleView NOTIFY modeChanged) // list is one picked title → drop per-row covers
    Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
public:
    explicit QmlSearchBridge(IEngine *session, QObject *parent = nullptr);

    QVariantList sources() const;
    QVariantList categories() const;
    QVariantList results() const;
    QString activeQuery() const;
    QString mode() const;
    bool inStreams() const;
    bool canGoBack() const;
    bool singleTitleView() const;
    bool searching() const;
    QString statusText() const;

    Q_INVOKABLE void refreshSources();
    Q_INVOKABLE void search(const QString &sourceKey, const QString &query, int categoryCode = 0);
    Q_INVOKABLE void activateResult(int index, bool force = false);   // catalog→streams; else add magnet (force skips the disk-fit guard)
    Q_INVOKABLE void back();                       // streams → catalog

    // Relevance ranking (backed by the tested SearchRanker service) so the view
    // sorts by relevance without reimplementing the scoring in QML/JS.
    Q_INVOKABLE QStringList queryWords() const;                       // significant words of the active query
    Q_INVOKABLE int relevance(const QString &name, const QStringList &words) const;
    Q_INVOKABLE int bestResultIndex() const { return pickBestResult(); }   // best release in the current list, or -1

    // Hydra-format game catalogs the user adds (neutral infra — nothing bundled).
    Q_INVOKABLE QVariantList gameSources() const;          // [{name, url}]
    Q_INVOKABLE void addGameSource(const QString &name, const QString &url);
    Q_INVOKABLE void removeGameSource(const QString &url);
    Q_INVOKABLE void refreshGames();

    // Lazily resolve a TMDB cover for a torrent row (by its info hash) as it
    // scrolls into view — mirrors the Downloads grid's on-demand resolution.
    void setResolver(MetadataResolver *r);
    Q_INVOKABLE void resolveCover(int index);

    // Title-first search (default "Tudo"): resolve the query to real works via
    // DiscoveryService, then drill into a picked title's torrents.
    void setDiscovery(DiscoveryService *d);
    Q_INVOKABLE void searchRaw();   // escape hatch: flat aggregate over every source
    Q_INVOKABLE void copyMagnet(int index);   // copy a flat result's magnet to the clipboard
    Q_INVOKABLE QString magnetAt(int index) const;   // a flat result's magnet (for Real-Debrid)
    // One-click "Get & Watch": search the title, auto-pick the best release, add
    // it, then hand the hash off (prepareAndWatch) so the player opens when it
    // buffers. Movie/series only — games are handled separately.
    Q_INVOKABLE void getAndWatch(const QString &title, const QString &year, const QString &type);
    Q_INVOKABLE void cancelGetAndWatch();   // user cancelled before a release was added
    // Discover hero: one cached source lookup per featured title → emits sourceSummary.
    Q_INVOKABLE void summarizeSources(const QString &title);

signals:
    void sourcesChanged();
    void resultsChanged();
    void modeChanged();
    void searchingChanged();
    void statusChanged();
    void gameSourcesChanged();
    void coverReady(const QString &infoHash, const QString &posterPath);
    void addedTorrent(const QString &infoHash);   // a magnet was added from Search
    void addWontFit(int index, const QString &name, qint64 needed, qint64 freeBytes);   // result too big for the save volume; QML confirms before force-add
    void watchSearching(const QString &title);    // Get&Watch: started looking for a release
    void watchNoRelease(const QString &title);    // Get&Watch: nothing usable found
    void prepareAndWatch(const QString &infoHash, const QString &title);   // added → buffer & open
    void sourceSummary(const QString &title, int count, qint64 bestSize, int maxSeeds);

private:
    bool fitsOnSaveVolume(qint64 needed) const;   // false ⇒ won't fit on the save disk
    void setSearching(bool on);
    void setStatus(const QString &s);
    void setMode(const QString &m);
    void runGameSearch(const QString &query);
    QSet<QString> currentResultKeys() const;
    void appendGameRows(const QList<GameDownload> &games);
    void appendTorrentRows(const QList<TorrentSearchResult> &results);
    void finishAggregateSource();
    static QString detectRepacker(const QString &name);
    // Parse quality/source/codec/hdr/lang tokens out of a release name for filtering.
    static void fillMediaAttrs(QVariantMap &m, const QString &name);
    static QString appLangCode();   // the app UI language as a release-tag code (PT/EN/…)
    // Flat aggregate search over every enabled source (the old "Tudo" behavior).
    void rawAggregateSearch(const QString &q, int categoryCode);
    // Type-scoped drill-down for a picked title: games hit game catalogs + the
    // games category; movies/series hit video torrents only (no game catalogs).
    void searchSourcesForWork(const QString &title, const QString &year, const QString &type);
    int pickBestResult() const;       // index of the best release in m_results, or -1
    void gwResolve();                 // Get&Watch: pick + add once the search settles

    bool m_gwActive = false;          // a Get&Watch search is in flight
    bool m_gwCancelled = false;       // user cancelled mid-search → don't add
    QString m_gwTitle, m_gwType;

    MetadataResolver *m_resolver = nullptr;
    DiscoveryService *m_discovery = nullptr;
    QString m_streamHintPoster;         // activated catalog item's poster, for stream rows
    QVariantList m_titleCache;          // works grid, restored when leaving the sources view
    bool m_fromTitles = false;          // sources view was opened by picking a title
    bool m_titleSources = false;        // current flat list is one picked title's torrents
    QString m_titleQuery;               // the free-text query behind the current titles
    QString m_activeQuery;              // effective query of the current flat list (for relevance)

    IEngine *m_session;
    QString m_mode;
    QString m_savePath;
    QString m_lastQuery;
    bool m_searching = false;
    bool m_isGameSearch = false;
    bool m_aggregate = false;          // "Tudo": merge every source into one list
    int m_pendingSources = 0;          // async sources still running in aggregate
    QString m_status;
    QVariantList m_results;
    QStringList m_resultMagnets;        // magnet per flat result row (add target)
    QStringList m_resultTitles;         // clean game title per row (cover hint), "" for torrents
    QList<CatalogItem> m_catalogCache;
    QList<StreamResult> m_streamCache;
    QList<TorrentSearchResult> m_torrentCache;
    QHash<QString, QVariantList> m_srcSummaryCache;   // lc(title) → [count, bestSize, maxSeeds]
    QSet<QString> m_srcSummaryInFlight;               // titles being summarized (dedup requests)
    QList<GameDownload> m_gameCache;
    QString m_pendingGameQuery;
    QString m_streamHintTitle;          // parent catalog title for a Stremio stream add
    int m_streamHintType = -1;          // its ContentType (Movie/Series) as int
    QVariantList m_catalogResultsSnapshot;
};

#endif // QMLSEARCHBRIDGE_H
