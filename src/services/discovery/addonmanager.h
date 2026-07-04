// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef ADDONMANAGER_H
#define ADDONMANAGER_H

#include <QObject>
#include <QStringList>
#include <QJsonArray>
#include <QVariantList>

class QNetworkAccessManager;

// Stremio-compatible addon manifest
struct AddonManifest {
    QString id;
    QString name;
    QString description;
    QString url; // base URL
    QStringList types; // "movie", "series", etc.
    QStringList resources; // "catalog", "stream"
    bool enabled = true;
};

// Catalog item from a Stremio catalog search
struct CatalogItem {
    QString id;   // IMDB ID (e.g. "tt1234567")
    QString type; // "movie" or "series"
    QString name;
    QString poster;
    int year = 0;
};

// Stream result from a Stremio stream endpoint
struct StreamResult {
    QString addonName;
    QString title;
    QString magnet;
    QString quality;  // "1080p", "4K", etc.
    qint64 size = 0;
};

// Torrent search result from a compatible API
struct TorrentSearchResult {
    QString name;
    QString infoHash;
    QString magnet;
    qint64 size = 0;
    int seeders = 0;
    int leechers = 0;
    QString category;
    QString provider;   // display name of the source that returned this row
};

// Search provider — a named torrent search engine with URL template + JSON mapping.
struct SearchProvider {
    QString id;
    QString name;
    QString urlTemplate; // {query} and {category} placeholders
    // JSON paths for parsing response (dot-separated, array root assumed)
    QString arrayPath = ""; // empty = root array, e.g. "torrents" or "results"
    QString namePath = "name";
    QString hashPath = "info_hash";
    QString sizePath = "size";
    QString seedersPath = "seeders";
    QString leechersPath = "leechers";
    bool enabled = true;
    bool builtIn = false;
    // ISO-ish region/language tag for grouping in the Sources manager:
    // "global" | "ptbr" | "cis" | "es" | "anime" | "self". Empty = global.
    QString region = "global";
    QString note;   // one-line caveat shown under the preset (e.g. "needs self-hosted TorAPI")
};

// A localized search source the user can add with one tap, grouped by region.
struct ProviderPreset {
    SearchProvider provider;   // the template that gets installed
    QString note;              // caveat / description
    bool needsConfig = false;  // URL has an API_KEY / host placeholder to edit first
};

class AddonManager : public QObject
{
    Q_OBJECT
public:
    static AddonManager &instance();

    // Addon management (Stremio-compatible)
    void addAddon(const QString &url);
    void removeAddon(int index);
    void setAddonEnabled(int index, bool enabled);
    QList<AddonManifest> addons() const;
    void loadAddons();
    void saveAddons();

    // Check addon capabilities
    bool hasCatalogAddon() const;
    bool hasStreamAddon() const;
    bool hasMetaAddon() const;

    // Stremio protocol: search catalogs
    void searchCatalog(const QString &query);

    // Stremio protocol: get streams for a specific item
    void getStreams(const QString &type, const QString &id);

    // Stremio protocol: item meta — the episode list (meta.videos) of a series.
    // Emits metaVideos(id, [{videoId,season,episode,name,released}]).
    void fetchMeta(const QString &type, const QString &id);

    // Pure: base URL for a stream request. For an unconfigured Torrentio addon,
    // injects "/language=<lang>" so releases in the user's language (dubbed
    // included) rank first — the same knob Torrentio users set by hand.
    static QString streamBaseUrl(const QString &addonUrl, const QString &torrentioLang);

    // Auto tracker list (ngosang/trackerslist)
    void fetchTrackerList();
    QStringList trackerList() const;
    bool autoTrackersEnabled() const;
    void setAutoTrackersEnabled(bool enabled);

    // Search providers (multi-engine torrent search)
    QList<SearchProvider> searchProviders() const;
    void addSearchProvider(const SearchProvider &p);
    void removeSearchProvider(int index);
    void setSearchProviderEnabled(int index, bool enabled);
    void setSearchProviderUrl(int index, const QString &urlTemplate);
    void searchWithProvider(int providerIndex, const QString &query, int category = 0);

    // Curated localized sources the user can one-tap add, grouped by region.
    // Presets already installed (matched by id) are filtered out by the bridge.
    static QList<ProviderPreset> providerCatalog();

    // Legacy single-URL search (kept for backward compat)
    bool torrentSearchEnabled() const;
    void setTorrentSearchEnabled(bool enabled);
    QString torrentSearchUrl() const;
    void setTorrentSearchUrl(const QString &url);
    void searchTorrents(const QString &query, int category);
    // Lightweight, isolated one-shot lookup: how many sources, the healthiest one's
    // size, and the peak seed count for a title. Does NOT touch the search UI pipeline.
    void summarizeTorrents(const QString &query, int category = 0);

signals:
    void addonAdded(const AddonManifest &manifest);
    void addonError(const QString &error);
    void catalogResults(const QList<CatalogItem> &items);
    void catalogFinished();
    void streamResults(const QList<StreamResult> &streams);
    void streamFinished();
    void metaVideos(const QString &id, const QVariantList &videos);
    void trackerListUpdated();
    void torrentSearchResults(const QList<TorrentSearchResult> &results);
    void torrentSearchFinished();
    void torrentSearchError(const QString &error);
    void torrentSummaryReady(const QString &query, int count, qint64 bestSize, int maxSeeds);

private:
    AddonManager();
    void installDefaults();
    void fetchManifest(const QString &url);

    QNetworkAccessManager *m_net;
    QList<AddonManifest> m_addons;
    QStringList m_trackerList;
    bool m_autoTrackers = true;
    bool m_torrentSearchEnabled = false;
    QString m_torrentSearchUrl;
    QList<SearchProvider> m_searchProviders;
    void installDefaultProviders();
    void loadSearchProviders();
    void saveSearchProviders();
    QList<TorrentSearchResult> parseProviderResponse(const SearchProvider &p, const QByteArray &data);
    int m_pendingCatalog = 0;
    int m_pendingStreams = 0;
    // Generation counters: every searchCatalog / getStreams call bumps the
    // matching counter; in-flight replies from an earlier call carry the
    // old gen and are dropped when they land. Without this, typing
    // "matrix" then "inception" mixes the late "matrix" results into the
    // "inception" list.
    quint32 m_catalogGen = 0;
    quint32 m_streamGen = 0;
    quint32 m_metaGen = 0;
    QList<CatalogItem> m_catalogResults;
    QList<StreamResult> m_streamResults;
};

#endif
