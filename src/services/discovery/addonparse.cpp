// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/discovery/addonparse.h"

#include "services/platform/contentlanguage.h"
#include "services/platform/utils.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSettings>
#include <QUrl>

namespace AddonParse {

bool isValidInfoHash(const QString &h)
{
    static const QRegularExpression re(
        QStringLiteral("^([0-9A-Fa-f]{40}|[0-9A-Fa-f]{64}|[A-Za-z2-7]{32})$"));
    return re.match(h).hasMatch();
}

QString normalizeAddonBaseUrl(QString url)
{
    url = url.trimmed();
    while (url.endsWith(QLatin1Char('/')))
        url.chop(1);
    static const QString manifest = QStringLiteral("/manifest.json");
    if (url.endsWith(manifest, Qt::CaseInsensitive))
        url.chop(manifest.size());
    while (url.endsWith(QLatin1Char('/')))
        url.chop(1);
    return url;
}

QString streamBaseUrl(const QString &addonUrl, const QString &torrentioLang)
{
    if (torrentioLang.isEmpty()) return addonUrl;
    if (!QUrl(addonUrl).host().contains(QLatin1String("torrentio"))) return addonUrl;
    if (addonUrl.contains(QLatin1Char('='))) return addonUrl;
    return addonUrl + QLatin1String("/language=") + torrentioLang;
}

QString torrentioLanguageTag(Translator::Language lang)
{
    switch (lang) {
    case Translator::Portuguese: return QStringLiteral("portuguese");
    case Translator::Spanish:    return QStringLiteral("spanish");
    case Translator::Russian:    return QStringLiteral("russian");
    case Translator::Japanese:   return QStringLiteral("japanese");
    case Translator::Chinese:    return QStringLiteral("chinese");
    case Translator::German:     return QStringLiteral("german");
    case Translator::Ukrainian:  return QStringLiteral("ukrainian");
    case Translator::Turkish:    return QStringLiteral("turkish");
    case Translator::English:    break;
    }
    return {};
}

QString torrentioLanguageForApp()
{
    if (!QSettings().value(QStringLiteral("preferNativeLang"), true).toBool())
        return {};
    return torrentioLanguageTag(ContentLanguage::current());
}

QString magnetTrackerParams()
{
    static const QStringList trackers = {
        QStringLiteral("udp://tracker.opentrackr.org:1337/announce"),
        QStringLiteral("udp://open.stealth.si:80/announce"),
        QStringLiteral("udp://tracker.openbittorrent.com:6969/announce"),
        QStringLiteral("udp://exodus.desync.com:6969/announce"),
    };
    QString params;
    for (const auto &t : trackers)
        params += QLatin1String("&tr=") + QUrl::toPercentEncoding(t);
    return params;
}

qint64 parseSizeValue(const QJsonValue &v)
{
    if (v.isDouble()) return static_cast<qint64>(v.toDouble());
    QString s = v.toString().trimmed();
    s.replace(QChar(0x00A0), QLatin1Char(' '));
    if (s.isEmpty()) return 0;
    bool ok = false;
    const qint64 plain = s.toLongLong(&ok);
    if (ok) return plain;
    static const QRegularExpression re(
        QStringLiteral("([\\d.,]+)\\s*([KMGT]?)i?B"), QRegularExpression::CaseInsensitiveOption);
    const auto m = re.match(s);
    if (!m.hasMatch()) return 0;
    const double num = m.captured(1).replace(QLatin1Char(','), QLatin1Char('.')).toDouble();
    const QString u = m.captured(2).toUpper();
    const double mult = u == QLatin1String("K") ? 1024.0
                      : u == QLatin1String("M") ? 1024.0 * 1024
                      : u == QLatin1String("G") ? 1024.0 * 1024 * 1024
                      : u == QLatin1String("T") ? 1024.0 * 1024 * 1024 * 1024 : 1.0;
    return static_cast<qint64>(num * mult);
}

QList<TorrentSearchResult> parseProviderResponse(const SearchProvider &p, const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);

    QJsonArray arr;
    if (p.arrayPath.isEmpty()) {
        if (doc.isArray()) arr = doc.array();
    } else {
        QJsonObject root = doc.object();
        QJsonValue v = root.value(p.arrayPath);
        if (v.isArray()) arr = v.toArray();
    }

    const QString trackerParams = magnetTrackerParams();

    QList<TorrentSearchResult> results;
    for (const auto &val : arr) {
        QJsonObject obj = val.toObject();
        QString name = decodeHtmlEntities(obj.value(p.namePath).toString());
        QString infoHash = obj.value(p.hashPath).toString();
        if (infoHash.isEmpty() || infoHash == QLatin1String("0")) continue;

        TorrentSearchResult r;
        r.name = name;
        r.infoHash = infoHash;
        r.size = parseSizeValue(obj.value(p.sizePath));
        r.seeders = obj.value(p.seedersPath).toVariant().toInt();
        r.leechers = obj.value(p.leechersPath).toVariant().toInt();
        r.category = obj.value(QStringLiteral("category")).toString();
        r.provider = p.name;
        r.magnet = QStringLiteral("magnet:?xt=urn:btih:%1&dn=%2%3")
            .arg(infoHash, QUrl::toPercentEncoding(name), trackerParams);
        results.append(r);
    }
    return results;
}

bool parseManifestJson(const QByteArray &data, const QString &baseUrl, AddonManifest *out)
{
    if (!out) return false;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return false;

    QJsonObject obj = doc.object();
    AddonManifest m;
    m.id = obj.value(QStringLiteral("id")).toString();
    m.name = obj.value(QStringLiteral("name")).toString(QStringLiteral("Unknown Addon"));
    m.description = obj.value(QStringLiteral("description")).toString();
    m.url = baseUrl;

    for (const auto &t : obj.value(QStringLiteral("types")).toArray())
        m.types.append(t.toString());

    for (const auto &r : obj.value(QStringLiteral("resources")).toArray()) {
        if (r.isString())
            m.resources.append(r.toString());
        else if (r.isObject())
            m.resources.append(r.toObject().value(QStringLiteral("name")).toString());
    }

    m.enabled = true;
    *out = m;
    return true;
}

QList<CatalogItem> parseCatalogMetas(const QByteArray &data)
{
    QList<CatalogItem> items;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    for (const auto &val : doc.object().value(QStringLiteral("metas")).toArray()) {
        QJsonObject m = val.toObject();
        CatalogItem item;
        item.id = m.value(QStringLiteral("id")).toString();
        item.type = m.value(QStringLiteral("type")).toString();
        item.name = decodeHtmlEntities(m.value(QStringLiteral("name")).toString());
        item.poster = m.value(QStringLiteral("poster")).toString();
        const QString release = m.value(QStringLiteral("releaseInfo")).toString();
        if (!release.isEmpty())
            item.year = release.left(4).toInt();
        if (!item.id.isEmpty())
            items.append(item);
    }
    return items;
}

QList<StreamResult> parseStreamResults(const QByteArray &data, const QString &addonName)
{
    QList<StreamResult> out;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    for (const auto &val : doc.object().value(QStringLiteral("streams")).toArray()) {
        QJsonObject s = val.toObject();
        StreamResult r;
        r.addonName = addonName;

        const QString infoHash = s.value(QStringLiteral("infoHash")).toString();
        if (isValidInfoHash(infoHash)) {
            r.magnet = QStringLiteral("magnet:?xt=urn:btih:%1").arg(infoHash);
            for (const auto &src : s.value(QStringLiteral("sources")).toArray()) {
                const QString tracker = src.toString();
                if (tracker.startsWith(QLatin1String("tracker:")))
                    r.magnet += QLatin1String("&tr=") + QUrl::toPercentEncoding(tracker.mid(8));
            }
        } else {
            r.magnet = s.value(QStringLiteral("url")).toString();
        }

        r.title = decodeHtmlEntities(s.value(QStringLiteral("title")).toString());
        if (r.title.isEmpty())
            r.title = decodeHtmlEntities(s.value(QStringLiteral("name")).toString());

        r.size = s.value(QStringLiteral("behaviorHints")).toObject()
                   .value(QStringLiteral("videoSize")).toVariant().toLongLong();

        if (!r.magnet.isEmpty() && r.magnet.startsWith(QLatin1String("magnet:")))
            out.append(r);
    }
    return out;
}

QList<TorrentSearchResult> parseApibayArray(const QByteArray &data, const QString &providerName)
{
    QList<TorrentSearchResult> results;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) return results;

    const QString trackerParams = magnetTrackerParams();
    for (const auto &val : doc.array()) {
        QJsonObject obj = val.toObject();
        QString name = decodeHtmlEntities(obj.value(QStringLiteral("name")).toString());
        QString infoHash = obj.value(QStringLiteral("info_hash")).toString();
        if (!isValidInfoHash(infoHash))
            continue;

        TorrentSearchResult r;
        r.name = name;
        r.infoHash = infoHash;
        r.size = obj.value(QStringLiteral("size")).toVariant().toLongLong();
        r.seeders = obj.value(QStringLiteral("seeders")).toVariant().toInt();
        r.leechers = obj.value(QStringLiteral("leechers")).toVariant().toInt();
        r.category = obj.value(QStringLiteral("category")).toString();
        r.provider = providerName;
        r.magnet = QStringLiteral("magnet:?xt=urn:btih:%1&dn=%2%3")
            .arg(infoHash, QUrl::toPercentEncoding(name), trackerParams);
        results.append(r);
    }
    return results;
}

} // namespace AddonParse
