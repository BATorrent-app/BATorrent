// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef ADDONMANAGER_H
#define ADDONMANAGER_H

#include <QObject>
#include <QStringList>
#include <QJsonArray>

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

    // Stremio protocol: search catalogs
    void searchCatalog(const QString &query);

    // Stremio protocol: get streams for a specific item
    void getStreams(const QString &type, const QString &id);

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
    void searchWithProvider(int providerIndex, const QString &query, int category = 0);

    // Legacy single-URL search (kept for backward compat)
    bool torrentSearchEnabled() const;
    void setTorrentSearchEnabled(bool enabled);
    QString torrentSearchUrl() const;
    void setTorrentSearchUrl(const QString &url);
    void searchTorrents(const QString &query, int category);

signals:
    void addonAdded(const AddonManifest &manifest);
    void addonError(const QString &error);
    void catalogResults(const QList<CatalogItem> &items);
    void catalogFinished();
    void streamResults(const QList<StreamResult> &streams);
    void streamFinished();
    void trackerListUpdated();
    void torrentSearchResults(const QList<TorrentSearchResult> &results);
    void torrentSearchFinished();
    void torrentSearchError(const QString &error);

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
    QList<CatalogItem> m_catalogResults;
    QList<StreamResult> m_streamResults;
};

#endif
