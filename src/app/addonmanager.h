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

    // Stremio protocol: search catalogs
    void searchCatalog(const QString &query);

    // Stremio protocol: get streams for a specific item
    void getStreams(const QString &type, const QString &id);

    // Auto tracker list (ngosang/trackerslist)
    void fetchTrackerList();
    QStringList trackerList() const;
    bool autoTrackersEnabled() const;
    void setAutoTrackersEnabled(bool enabled);

signals:
    void addonAdded(const AddonManifest &manifest);
    void addonError(const QString &error);
    void catalogResults(const QList<CatalogItem> &items);
    void catalogFinished();
    void streamResults(const QList<StreamResult> &streams);
    void streamFinished();
    void trackerListUpdated();

private:
    AddonManager();
    void fetchManifest(const QString &url);

    QNetworkAccessManager *m_net;
    QList<AddonManifest> m_addons;
    QStringList m_trackerList;
    bool m_autoTrackers = true;
    int m_pendingCatalog = 0;
    int m_pendingStreams = 0;
    QList<CatalogItem> m_catalogResults;
    QList<StreamResult> m_streamResults;
};

#endif
