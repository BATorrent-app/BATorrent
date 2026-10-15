// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge: what the Add download window asks for. What a pasted
// line is, magnets that fetch their metadata inside the window, a direct
// link's size and whether it resumes, a torrent's files in groups, and the
// folder each kind of download goes to.

#include "bridges/session/qmlsessionbridge.h"
#include "bridges/librarygroups.h"
#include "torrent/addkind.h"
#include "torrent/filetree.h"
#include "torrent/iengine.h"
#include "services/metadata/filegroups.h"
#include "services/metadata/metadataresolver.h"
#include "services/metadata/nameparser.h"
#include "services/platform/utils.h"

#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/torrent_info.hpp>
#include <QDir>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QUrl>
#include <sstream>

namespace {

QString kindOf(const QString &name, const QStringList &paths)
{
    ContentType t = paths.isEmpty() ? ContentType::Unknown : NameParser::classifyByFiles(paths);
    if (t == ContentType::Unknown) t = NameParser::parse(name).contentType;
    switch (t) {
    case ContentType::Game:   return QStringLiteral("game");
    case ContentType::Movie:
    case ContentType::Series: return QStringLiteral("video");
    case ContentType::Unknown: break;
    }
    return QStringLiteral("other");
}

QVariantList intList(const QList<int> &l)
{
    QVariantList out;
    out.reserve(l.size());
    for (int i : l) out << i;
    return out;
}

const char *folderKey(const QString &kind)
{
    if (kind == QLatin1String("game"))  return "folderGames";
    if (kind == QLatin1String("video")) return "folderVideo";
    return "folderOther";
}

} // namespace

QVariantMap QmlSessionBridge::buildPreview(const QString &name, const QString &infoHash,
                                          const QStringList &paths, const QList<qint64> &sizes) const
{
    QVariantMap out;
    out["ok"] = true;
    out["name"] = name;
    out["infoHash"] = infoHash;
    qint64 total = 0;
    for (qint64 s : sizes) total += s;
    out["totalSize"] = formatSize(total);
    out["totalSizeBytes"] = total;
    out["fileCount"] = paths.size();

    if (m_resolver && m_resolver->hasCached(infoHash)) {
        auto meta = m_resolver->cached(infoHash);
        if (meta.valid && QFileInfo::exists(meta.posterPath)) out["posterPath"] = meta.posterPath;
    }

    const ParsedName pn = NameParser::parse(name);
    out["cleanTitle"] = pn.cleanTitle.isEmpty() ? name : pn.cleanTitle;
    out["year"] = pn.year;
    out["repacker"] = pn.repackerTag;
    out["kind"] = kindOf(name, paths);
    out["joins"] = seriesMatch(name);
    out["inLibrary"] = inLibrary(infoHash);

    QVariantList files;
    const auto rows = buildFileTree(paths);
    files.reserve(rows.size());
    for (const FileTreeRow &r : rows) {
        QVariantMap f;
        f["path"]  = r.name;
        f["dir"]   = r.isDir;
        f["depth"] = r.depth;
        f["fileIndex"] = r.fileIndex;
        const qint64 bytes = r.isDir || r.fileIndex < 0 || r.fileIndex >= sizes.size() ? 0 : sizes[r.fileIndex];
        f["size"] = r.isDir ? QString() : formatSize(bytes);
        f["sizeBytes"] = bytes;
        files << f;
    }
    out["files"] = files;

    QList<FileGroups::File> fl;
    fl.reserve(paths.size());
    for (int i = 0; i < paths.size(); ++i) fl << FileGroups::File{ i, paths[i], i < sizes.size() ? sizes[i] : 0 };
    QVariantList groups;
    for (const FileGroups::Group &g : FileGroups::group(fl, name)) {
        QVariantMap gm;
        gm["kind"] = g.kind;
        gm["label"] = g.label;
        gm["season"] = g.season;
        gm["files"] = intList(g.files);
        gm["sizeBytes"] = g.size;
        gm["size"] = formatSize(g.size);
        QVariantList chips;
        for (const FileGroups::Chip &c : g.chips) {
            QVariantMap cm;
            cm["label"] = c.label;
            cm["lang"] = c.lang;
            cm["files"] = intList(c.files);
            cm["sizeBytes"] = c.size;
            cm["size"] = formatSize(c.size);
            cm["inBase"] = c.inBase;
            chips << cm;
        }
        gm["chips"] = chips;
        groups << gm;
    }
    out["groups"] = groups;
    return out;
}

QVariantMap QmlSessionBridge::describeInput(const QString &input) const
{
    QVariantMap out;
    const AddTarget t = classifyAdd(input.trimmed());
    switch (t.kind) {
    case AddKind::Magnet: {
        out["kind"] = QStringLiteral("magnet");
        out["value"] = t.value;
        try {
            const lt::add_torrent_params atp = lt::parse_magnet_uri(t.value.toStdString());
            const QString hash = QString::fromStdString(
                (std::ostringstream() << atp.info_hashes.get_best()).str());
            out["hash"] = hash;
            out["name"] = QString::fromStdString(atp.name);
            out["inLibrary"] = inLibrary(hash);
            if (!atp.name.empty()) out["joins"] = seriesMatch(QString::fromStdString(atp.name));
        } catch (const std::exception &) {
            return {};
        }
        break;
    }
    case AddKind::TorrentFile:
        out["kind"] = QStringLiteral("torrentFile");
        out["value"] = t.value;
        break;
    case AddKind::TorrentUrl:
    case AddKind::WebFile: {
        out["kind"] = t.kind == AddKind::TorrentUrl ? QStringLiteral("torrentUrl") : QStringLiteral("webFile");
        out["value"] = t.value;
        const QUrl u(t.value);
        out["host"] = u.host();
        out["name"] = QFileInfo(u.path()).fileName();
        break;
    }
    case AddKind::Unknown:
        return {};
    }
    return out;
}

bool QmlSessionBridge::inLibrary(const QString &infoHash) const
{
    return !infoHash.isEmpty() && m_session->torrentIndexByInfoHash(infoHash) >= 0;
}

QString QmlSessionBridge::seriesMatch(const QString &name) const
{
    const ParsedName pn = NameParser::parse(name);
    if (pn.episode < 0 || pn.cleanTitle.isEmpty()) return {};
    const QString key = LibraryGroups::nameKey(pn.cleanTitle);
    if (key.isEmpty()) return {};
    for (int i = 0; i < m_session->torrentCount(); ++i) {
        const ParsedName other = NameParser::parse(m_session->torrentAt(i).name);
        if (other.cleanTitle.isEmpty() || LibraryGroups::nameKey(other.cleanTitle) != key) continue;
        // the show's title as the library already shows it
        return pn.season >= 0 ? QStringLiteral("%1\t%2").arg(other.cleanTitle).arg(pn.season)
                              : other.cleanTitle;
    }
    return {};
}

void QmlSessionBridge::stageMagnet(const QString &uri)
{
    m_session->stageMagnet(uri);
}

void QmlSessionBridge::discardStaged(const QString &infoHash)
{
    m_session->discardStaged(infoHash);
}

namespace {
std::vector<int> toPrios(const QVariantList &l)
{
    std::vector<int> out;
    out.reserve(l.size());
    for (const QVariant &v : l) out.push_back(v.toInt());
    return out;
}
QString localPath(QString p)
{
    if (p.startsWith(QStringLiteral("file://"))) p = QUrl(p).toLocalFile();
    return p;
}
}

// The folder made and remembered once, for any of the three ways in
AddOptions QmlSessionBridge::addOptions(const QString &savePath, const QVariantList &priorities, bool paused)
{
    AddOptions o;
    o.savePath = localPath(savePath).isEmpty() ? defaultSavePath() : localPath(savePath);
    QDir().mkpath(o.savePath);
    rememberSavePath(o.savePath);
    o.filePriorities = toPrios(priorities);
    o.paused = paused;
    return o;
}

void QmlSessionBridge::commitStaged(const QString &infoHash, const QString &savePath,
                                    const QVariantList &priorities, bool paused)
{
    m_session->commitStaged(infoHash, addOptions(savePath, priorities, paused));
}

void QmlSessionBridge::addTorrentFileWith(const QString &filePath, const QString &savePath,
                                          const QVariantList &priorities, bool paused)
{
    if (filePath.isEmpty()) return;
    m_session->addTorrentFileWith(localPath(filePath), addOptions(savePath, priorities, paused));
}

void QmlSessionBridge::addMagnetWith(const QString &uri, const QString &savePath, bool paused)
{
    if (uri.isEmpty()) return;
    const QString normalized = normalizeClipboardMagnet(uri);
    if (!normalized.isEmpty()) m_lastClipboardMagnet = normalized;
    m_session->addMagnetWith(uri, addOptions(savePath, {}, paused));
}

void QmlSessionBridge::onStagedMetadata(const QString &hash, const QString &name, qint64,
                                        const QStringList &paths, const QList<qint64> &sizes)
{
    emit stagedPreview(hash, buildPreview(name, hash, paths, sizes));
}

// A HEAD request: the size, the file name the server gives, and whether it
// answers ranges (a download that can resume after a drop).
void QmlSessionBridge::probeUrl(const QString &url)
{
    const QUrl qurl(url.trimmed());
    if (!qurl.isValid()) return;
    if (!m_probeNet) m_probeNet = new QNetworkAccessManager(this);
    QNetworkRequest req(qurl);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setTransferTimeout(15000);
    QNetworkReply *reply = m_probeNet->head(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, url]() {
        reply->deleteLater();
        QVariantMap out;
        out["ok"] = reply->error() == QNetworkReply::NoError;
        const qint64 size = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
        out["sizeBytes"] = size;
        out["size"] = size > 0 ? formatSize(size) : QString();
        out["resumable"] = reply->rawHeader("Accept-Ranges").toLower().contains("bytes");
        QString name;
        static const QRegularExpression cd(QStringLiteral("filename\\*?=(?:UTF-8'')?\"?([^\";]+)"),
                                           QRegularExpression::CaseInsensitiveOption);
        const auto m = cd.match(QString::fromUtf8(reply->rawHeader("Content-Disposition")));
        if (m.hasMatch()) name = QUrl::fromPercentEncoding(m.captured(1).toUtf8());
        if (name.isEmpty()) name = QFileInfo(reply->url().path()).fileName();
        out["name"] = name;
        out["host"] = reply->url().host();
        out["kind"] = kindOf(name, {});
        emit urlProbed(url, out);
    });
}

// A .torrent behind a link, fetched to a temp file so the window can read it
// like a dropped one.
void QmlSessionBridge::fetchTorrentUrl(const QString &url)
{
    const QUrl qurl(url.trimmed());
    if (!qurl.isValid()) { emit torrentUrlFetched(url, QString()); return; }
    if (!m_probeNet) m_probeNet = new QNetworkAccessManager(this);
    QNetworkRequest req(qurl);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setTransferTimeout(30000);
    QNetworkReply *reply = m_probeNet->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, url]() {
        reply->deleteLater();
        const QByteArray data = reply->error() == QNetworkReply::NoError ? reply->readAll() : QByteArray();
        if (data.isEmpty() || data.front() != 'd') { emit torrentUrlFetched(url, QString()); return; }
        QString name = QFileInfo(reply->url().path()).fileName();
        if (!name.endsWith(QStringLiteral(".torrent"), Qt::CaseInsensitive)) name = QStringLiteral("download.torrent");
        const QString path = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
            + QStringLiteral("/bat_%1_%2").arg(QDateTime::currentMSecsSinceEpoch()).arg(name);
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly)) { emit torrentUrlFetched(url, QString()); return; }
        f.write(data);
        f.close();
        emit torrentUrlFetched(url, path);
    });
}

// Where each kind of download goes. Unset, games go to the disk with the most
// room and the rest to the usual place.
QString QmlSessionBridge::folderFor(const QString &kind) const
{
    const QString set = QSettings().value(QLatin1String(folderKey(kind))).toString();
    if (!set.isEmpty()) return QDir::toNativeSeparators(set);
    if (kind != QLatin1String("game")) return QDir::toNativeSeparators(defaultSavePath());

    QStorageInfo best;
    for (const QStorageInfo &v : QStorageInfo::mountedVolumes()) {
        if (!v.isValid() || !v.isReady() || v.isReadOnly()) continue;
        const QString fs = QString::fromLatin1(v.fileSystemType()).toLower();
        if (fs.contains(QLatin1String("tmpfs")) || fs.contains(QLatin1String("squash"))) continue;
        if (!best.isValid() || v.bytesAvailable() > best.bytesAvailable()) best = v;
    }
    // inside the usual folder when that is on the roomiest disk, the user's
    // Downloads when it is the system disk, else a Games folder at its root
    QString base;
    if (!best.isValid() || QStorageInfo(defaultSavePath()).rootPath() == best.rootPath())
        base = defaultSavePath();
    else if (QStorageInfo(QDir::homePath()).rootPath() == best.rootPath()) {
        // the system disk: the user's Downloads, unless that was moved elsewhere
        const QString dl = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        base = QStorageInfo(dl).rootPath() == best.rootPath() ? dl : QDir::homePath();
    } else
        base = best.rootPath();
    return QDir::toNativeSeparators(QDir(base).filePath(QStringLiteral("Games")));
}

void QmlSessionBridge::setTypeFolder(const QString &kind, const QString &path)
{
    const QString p = QDir::cleanPath(localPath(path));
    if (p.isEmpty()) return;
    QSettings().setValue(QLatin1String(folderKey(kind)), p);
    emit typeFoldersChanged();
}

QVariantList QmlSessionBridge::typeFolders() const
{
    QVariantList out;
    for (const QString kind : { QStringLiteral("game"), QStringLiteral("video"), QStringLiteral("other") }) {
        const QString p = folderFor(kind);
        QVariantMap m;
        m["kind"] = kind;
        m["path"] = p;
        m["name"] = QFileInfo(p).fileName().isEmpty() ? p : QFileInfo(p).fileName();
        const double free = freeBytesAt(p);
        m["freeBytes"] = free;
        m["free"] = free >= 0 ? formatSize(qint64(free)) : QString();
        out << m;
    }
    return out;
}
