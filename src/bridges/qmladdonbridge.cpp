// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/qmladdonbridge.h"

#include "services/discovery/addonmanager.h"

#include <QSet>

QmlAddonBridge::QmlAddonBridge(QObject *parent) : QObject(parent)
{
    auto &mgr = AddonManager::instance();
    connect(&mgr, &AddonManager::addonAdded, this, [this](const AddonManifest &){ emit changed(); });
    connect(&mgr, &AddonManager::addonError, this, [this](const QString &e){ emit error(e); });
    connect(&mgr, &AddonManager::trackerListUpdated, this, &QmlAddonBridge::changed);
}

QVariantList QmlAddonBridge::installed() const
{
    QVariantList out;
    const auto list = AddonManager::instance().addons();
    for (const auto &a : list) {
        QVariantMap m;
        m["name"] = a.name;
        m["description"] = a.description;
        m["url"] = a.url;
        m["types"] = a.types.join(", ");
        m["enabled"] = a.enabled;
        out << m;
    }
    return out;
}

QVariantList QmlAddonBridge::suggested() const
{
    QVariantList out;
    for (const auto &s : AddonManager::curatedCatalog()) {
        QVariantMap m;
        m[QStringLiteral("name")] = s.name;
        m[QStringLiteral("descKey")] = s.descKey;
        m[QStringLiteral("description")] = s.description;
        m[QStringLiteral("url")] = s.url;
        m[QStringLiteral("configureUrl")] = s.configureUrl;
        m[QStringLiteral("lang")] = s.lang;
        m[QStringLiteral("needsConfig")] = s.needsConfig;
        m[QStringLiteral("needsDebrid")] = s.needsDebrid;
        m[QStringLiteral("seedDefault")] = s.seedDefault;
        m[QStringLiteral("alwaysOn")] = s.alwaysOn;
        const bool installed = (!s.url.isEmpty() && isInstalled(s.url))
                               || (!s.id.isEmpty() && [&]() {
                                      for (const auto &a : AddonManager::instance().addons())
                                          if (a.id == s.id) return true;
                                      return false;
                                  }());
        m[QStringLiteral("installed")] = installed;
        out << m;
    }
    return out;
}

bool QmlAddonBridge::autoTrackers() const { return AddonManager::instance().autoTrackersEnabled(); }
void QmlAddonBridge::setAutoTrackers(bool on) { AddonManager::instance().setAutoTrackersEnabled(on); emit changed(); }
int QmlAddonBridge::trackerCount() const { return AddonManager::instance().trackerList().size(); }
bool QmlAddonBridge::torrentSearchEnabled() const { return AddonManager::instance().torrentSearchEnabled(); }
void QmlAddonBridge::setTorrentSearchEnabled(bool on) { AddonManager::instance().setTorrentSearchEnabled(on); emit changed(); }
QString QmlAddonBridge::torrentSearchUrl() const { return AddonManager::instance().torrentSearchUrl(); }
void QmlAddonBridge::setTorrentSearchUrl(const QString &url) { AddonManager::instance().setTorrentSearchUrl(url); emit changed(); }

void QmlAddonBridge::addAddon(const QString &url) { if (!url.isEmpty()) AddonManager::instance().addAddon(url); }
void QmlAddonBridge::removeAddon(int index) { AddonManager::instance().removeAddon(index); emit changed(); }
void QmlAddonBridge::setEnabled(int index, bool on) { AddonManager::instance().setAddonEnabled(index, on); emit changed(); }
void QmlAddonBridge::refreshTrackers() { AddonManager::instance().fetchTrackerList(); }

bool QmlAddonBridge::isInstalled(const QString &url) const
{
    QString want = url.trimmed();
    while (want.endsWith(QLatin1Char('/')))
        want.chop(1);
    static const QString manifest = QStringLiteral("/manifest.json");
    if (want.endsWith(manifest, Qt::CaseInsensitive))
        want.chop(manifest.size());
    while (want.endsWith(QLatin1Char('/')))
        want.chop(1);

    const auto list = AddonManager::instance().addons();
    for (const auto &a : list)
        if (a.url == want) return true;
    return false;
}

QVariantList QmlAddonBridge::searchProviders() const
{
    QVariantList out;
    const auto list = AddonManager::instance().searchProviders();
    for (int i = 0; i < list.size(); ++i) {
        const auto &p = list[i];
        QVariantMap m;
        m["index"] = i;
        m["name"] = p.name;
        m["region"] = p.region.isEmpty() ? QStringLiteral("global") : p.region;
        m["enabled"] = p.enabled;
        m["builtIn"] = p.builtIn;
        m["note"] = p.note;
        m["url"] = p.urlTemplate;
        m["editable"] = p.urlTemplate.contains(QLatin1String("API_KEY"))
                        || p.urlTemplate.contains(QLatin1String("127.0.0.1"))
                        || p.region == QLatin1String("self");
        out << m;
    }
    return out;
}

QVariantList QmlAddonBridge::sourceCatalog() const
{
    QSet<QString> have;
    for (const auto &p : AddonManager::instance().searchProviders()) have.insert(p.id);

    QVariantList out;
    for (const auto &preset : AddonManager::providerCatalog()) {
        if (have.contains(preset.provider.id)) continue;
        QVariantMap m;
        m["id"] = preset.provider.id;
        m["name"] = preset.provider.name;
        m["region"] = preset.provider.region;
        m["note"] = preset.note;
        m["needsConfig"] = preset.needsConfig;
        out << m;
    }
    return out;
}

void QmlAddonBridge::setSearchProviderEnabled(int index, bool on)
{
    AddonManager::instance().setSearchProviderEnabled(index, on);
    emit changed();
}

void QmlAddonBridge::removeSearchProvider(int index)
{
    AddonManager::instance().removeSearchProvider(index);
    emit changed();
}

void QmlAddonBridge::addCatalogSource(const QString &id)
{
    for (const auto &preset : AddonManager::providerCatalog()) {
        if (preset.provider.id != id) continue;
        SearchProvider p = preset.provider;
        p.note = preset.note;
        p.enabled = !preset.needsConfig;
        AddonManager::instance().addSearchProvider(p);
        emit changed();
        return;
    }
}

void QmlAddonBridge::updateSearchProviderUrl(int index, const QString &url)
{
    AddonManager::instance().setSearchProviderUrl(index, url);
    emit changed();
}
