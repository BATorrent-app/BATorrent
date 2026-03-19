// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "addonmanager.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QUrl>

AddonManager &AddonManager::instance()
{
    static AddonManager mgr;
    return mgr;
}

AddonManager::AddonManager()
    : m_net(new QNetworkAccessManager(this))
{
    loadAddons();
}

void AddonManager::loadAddons()
{
    QSettings settings("BATorrent", "BATorrent");
    m_autoTrackers = settings.value("autoTrackers", true).toBool();

    int count = settings.beginReadArray("addons");
    m_addons.clear();
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        AddonManifest m;
        m.id = settings.value("id").toString();
        m.name = settings.value("name").toString();
        m.description = settings.value("description").toString();
        m.url = settings.value("url").toString();
        m.types = settings.value("types").toStringList();
        m.resources = settings.value("resources").toStringList();
        m.enabled = settings.value("enabled", true).toBool();
        m_addons.append(m);
    }
    settings.endArray();

    // Load cached tracker list
    m_trackerList = settings.value("trackerList").toStringList();
}

void AddonManager::saveAddons()
{
    QSettings settings("BATorrent", "BATorrent");

    settings.beginWriteArray("addons", m_addons.size());
    for (int i = 0; i < m_addons.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("id", m_addons[i].id);
        settings.setValue("name", m_addons[i].name);
        settings.setValue("description", m_addons[i].description);
        settings.setValue("url", m_addons[i].url);
        settings.setValue("types", m_addons[i].types);
        settings.setValue("resources", m_addons[i].resources);
        settings.setValue("enabled", m_addons[i].enabled);
    }
    settings.endArray();
}

void AddonManager::addAddon(const QString &url)
{
    QString baseUrl = url;
    if (baseUrl.endsWith('/'))
        baseUrl.chop(1);

    // Check duplicates
    for (const auto &a : m_addons) {
        if (a.url == baseUrl) {
            emit addonError("Addon already installed.");
            return;
        }
    }

    fetchManifest(baseUrl);
}

void AddonManager::fetchManifest(const QString &url)
{
    QNetworkRequest req(QUrl(url + "/manifest.json"));
    req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent/1.9");
    auto *reply = m_net->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply, url]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit addonError(reply->errorString());
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            emit addonError("Invalid manifest format.");
            return;
        }

        // Parse Stremio manifest
        QJsonObject obj = doc.object();
        AddonManifest m;
        m.id = obj.value("id").toString();
        m.name = obj.value("name").toString("Unknown Addon");
        m.description = obj.value("description").toString();
        m.url = url;

        // Parse types
        QJsonArray types = obj.value("types").toArray();
        for (const auto &t : types)
            m.types.append(t.toString());

        // Parse resources
        QJsonArray resources = obj.value("resources").toArray();
        for (const auto &r : resources) {
            if (r.isString())
                m.resources.append(r.toString());
            else if (r.isObject())
                m.resources.append(r.toObject().value("name").toString());
        }

        m.enabled = true;
        m_addons.append(m);
        saveAddons();
        emit addonAdded(m);
    });
}

void AddonManager::removeAddon(int index)
{
    if (index < 0 || index >= m_addons.size()) return;
    m_addons.removeAt(index);
    saveAddons();
}

void AddonManager::setAddonEnabled(int index, bool enabled)
{
    if (index < 0 || index >= m_addons.size()) return;
    m_addons[index].enabled = enabled;
    saveAddons();
}

QList<AddonManifest> AddonManager::addons() const
{
    return m_addons;
}

// Stremio catalog search: GET {url}/catalog/{type}/{catalogId}/search={query}.json
void AddonManager::searchCatalog(const QString &query)
{
    m_catalogResults.clear();
    m_pendingCatalog = 0;

    for (const auto &addon : m_addons) {
        if (!addon.enabled || !addon.resources.contains("catalog"))
            continue;

        // Search each type the addon supports
        for (const auto &type : addon.types) {
            m_pendingCatalog++;
            // Stremio search URL format
            QString searchUrl = QString("%1/catalog/%2/top/search=%3.json")
                .arg(addon.url, type, QUrl::toPercentEncoding(query));

            QNetworkRequest req{QUrl(searchUrl)};
            req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent/1.9");
            auto *reply = m_net->get(req);

            connect(reply, &QNetworkReply::finished, this, [this, reply]() {
                reply->deleteLater();
                m_pendingCatalog--;

                if (reply->error() == QNetworkReply::NoError) {
                    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                    QJsonObject obj = doc.object();
                    QJsonArray metas = obj.value("metas").toArray();

                    for (const auto &val : metas) {
                        QJsonObject m = val.toObject();
                        CatalogItem item;
                        item.id = m.value("id").toString();
                        item.type = m.value("type").toString();
                        item.name = m.value("name").toString();
                        item.poster = m.value("poster").toString();
                        // Year from releaseInfo or year field
                        QString release = m.value("releaseInfo").toString();
                        if (!release.isEmpty())
                            item.year = release.left(4).toInt();

                        // Avoid duplicates by IMDB ID
                        bool dup = false;
                        for (const auto &existing : m_catalogResults) {
                            if (existing.id == item.id) { dup = true; break; }
                        }
                        if (!dup && !item.id.isEmpty())
                            m_catalogResults.append(item);
                    }
                }

                emit catalogResults(m_catalogResults);
                if (m_pendingCatalog <= 0)
                    emit catalogFinished();
            });
        }
    }

    if (m_pendingCatalog == 0)
        emit catalogFinished();
}

// Stremio stream: GET {url}/stream/{type}/{id}.json
void AddonManager::getStreams(const QString &type, const QString &id)
{
    m_streamResults.clear();
    m_pendingStreams = 0;

    for (const auto &addon : m_addons) {
        if (!addon.enabled || !addon.resources.contains("stream"))
            continue;
        if (!addon.types.contains(type))
            continue;

        m_pendingStreams++;
        QString streamUrl = QString("%1/stream/%2/%3.json")
            .arg(addon.url, type, id);

        QNetworkRequest req{QUrl(streamUrl)};
        req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent/1.9");
        auto *reply = m_net->get(req);

        connect(reply, &QNetworkReply::finished, this, [this, reply, addonName = addon.name]() {
            reply->deleteLater();
            m_pendingStreams--;

            if (reply->error() == QNetworkReply::NoError) {
                QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                QJsonObject obj = doc.object();
                QJsonArray streams = obj.value("streams").toArray();

                for (const auto &val : streams) {
                    QJsonObject s = val.toObject();
                    StreamResult r;
                    r.addonName = addonName;

                    // Stremio streams can have infoHash+fileIdx or direct magnet
                    QString infoHash = s.value("infoHash").toString();
                    if (!infoHash.isEmpty()) {
                        r.magnet = QString("magnet:?xt=urn:btih:%1").arg(infoHash);
                        // Add trackers from behaviorHints or sources
                        QJsonArray sources = s.value("sources").toArray();
                        for (const auto &src : sources) {
                            QString tracker = src.toString();
                            if (tracker.startsWith("tracker:"))
                                r.magnet += "&tr=" + QUrl::toPercentEncoding(tracker.mid(8));
                        }
                    } else {
                        // Direct URL (magnet or HTTP)
                        r.magnet = s.value("url").toString();
                    }

                    // Parse title/name for quality info
                    r.title = s.value("title").toString();
                    if (r.title.isEmpty())
                        r.title = s.value("name").toString();

                    // Extract size from behaviorHints
                    QJsonObject hints = s.value("behaviorHints").toObject();
                    r.size = hints.value("videoSize").toVariant().toLongLong();

                    if (!r.magnet.isEmpty() && r.magnet.startsWith("magnet:"))
                        m_streamResults.append(r);
                }
            }

            emit streamResults(m_streamResults);
            if (m_pendingStreams <= 0)
                emit streamFinished();
        });
    }

    if (m_pendingStreams == 0)
        emit streamFinished();
}

// --- Auto tracker list ---

void AddonManager::fetchTrackerList()
{
    QUrl url("https://raw.githubusercontent.com/ngosang/trackerslist/master/trackers_best.txt");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent/1.9");
    auto *reply = m_net->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;

        QString data = QString::fromUtf8(reply->readAll());
        QStringList trackers;
        for (const auto &line : data.split('\n')) {
            QString trimmed = line.trimmed();
            if (!trimmed.isEmpty())
                trackers.append(trimmed);
        }

        if (!trackers.isEmpty()) {
            m_trackerList = trackers;
            QSettings settings("BATorrent", "BATorrent");
            settings.setValue("trackerList", m_trackerList);
            emit trackerListUpdated();
        }
    });
}

QStringList AddonManager::trackerList() const
{
    return m_trackerList;
}

bool AddonManager::autoTrackersEnabled() const
{
    return m_autoTrackers;
}

void AddonManager::setAutoTrackersEnabled(bool enabled)
{
    m_autoTrackers = enabled;
    QSettings settings("BATorrent", "BATorrent");
    settings.setValue("autoTrackers", enabled);
}
