// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge — add torrent/magnet/http + preview.

#include "bridges/session/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"
#include "services/metadata/metadataresolver.h"
#include "services/downloads/httpdownloadmanager.h"
#include "services/downloads/filehostresolver.h"
#include "services/platform/translator.h"
#include "services/platform/utils.h"

#include <QFileInfo>
#include <QUrl>
#include <QVariantMap>
#include <QVariantList>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTemporaryFile>
#include <QDir>
#include <QStandardPaths>

void QmlSessionBridge::addTorrentFile(const QString &filePath)
{
    if (filePath.isEmpty()) return;
    QString local = filePath;
    if (local.startsWith(QStringLiteral("file://")))
        local = QUrl(local).toLocalFile();
    m_session->addTorrent(local, defaultSavePath());
}

void QmlSessionBridge::requestAddTorrentFile(const QString &filePath)
{
    if (filePath.isEmpty()) return;
    emit openTorrentRequested(filePath);
}

void QmlSessionBridge::addMagnetUri(const QString &uri, const QString &savePath)
{
    if (uri.isEmpty()) return;
    // Any direct add (drag-drop, browser handoff, smart paste) marks the link
    // as seen — regaining focus right after must not re-offer the same magnet
    // in the Add dialog (reported: duplicate dialog after drag & drop).
    const QString normalized = normalizeClipboardMagnet(uri);
    if (!normalized.isEmpty()) m_lastClipboardMagnet = normalized;
    if (!savePath.isEmpty()) rememberSavePath(savePath);
    m_session->addMagnet(uri, savePath.isEmpty() ? defaultSavePath() : savePath);
}

void QmlSessionBridge::addHttpUrl(const QString &url, const QString &savePath)
{
    const QString u = url.trimmed();
    if (u.isEmpty()) return;
    if (u.startsWith(QStringLiteral("magnet:"), Qt::CaseInsensitive)) { addMagnetUri(u, savePath); return; }

    const QUrl qurl(u);
    if (!m_httpDownloads || !qurl.isValid()
        || !(qurl.scheme() == QLatin1String("http") || qurl.scheme() == QLatin1String("https"))) {
        emit toast(tr_("add_url_failed"), u);
        return;
    }
    if (!savePath.isEmpty()) rememberSavePath(savePath);
    // The new row landing in the Downloads list is the confirmation (like a
    // torrent add) — no success toast needed.
    m_httpDownloads->add(qurl, savePath.isEmpty() ? defaultSavePath() : savePath);
}

void QmlSessionBridge::addTorrentUrl(const QString &url)
{
    const QString u = url.trimmed();
    if (u.isEmpty()) return;
    if (u.startsWith(QStringLiteral("magnet:"), Qt::CaseInsensitive)) { addMagnetUri(u); return; }

    const QUrl qurl(u);
    if (!qurl.isValid() || !(qurl.scheme() == QLatin1String("http") || qurl.scheme() == QLatin1String("https"))) {
        emit toast(tr_("add_url_failed"), u);
        return;
    }

    auto *nam = new QNetworkAccessManager(this);
    QNetworkRequest req(qurl);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto *reply = nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, nam]() {
        reply->deleteLater();
        nam->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit toast(tr_("add_url_failed"), reply->errorString());
            return;
        }
        const QByteArray data = reply->readAll();
        // A bencoded .torrent always starts with 'd' (a dict). Anything else
        // (an HTML error page, a redirect to a login wall) is rejected here.
        if (data.isEmpty() || data.front() != 'd') {
            emit toast(tr_("add_url_failed"), reply->url().toString());
            return;
        }
        QString name = QFileInfo(reply->url().path()).fileName();
        if (!name.endsWith(QStringLiteral(".torrent"), Qt::CaseInsensitive))
            name = QStringLiteral("download.torrent");
        const QString path = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
            + QStringLiteral("/bat_%1_%2").arg(QDateTime::currentMSecsSinceEpoch()).arg(name);
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly)) {
            emit toast(tr_("add_url_failed"), path);
            return;
        }
        f.write(data);
        f.close();
        // Route through the same add dialog as a dropped file (user picks
        // save path / files) instead of silently auto-downloading.
        emit openTorrentRequested(path);
    });
}

QVariantMap QmlSessionBridge::previewTorrent(const QString &filePath) const
{
    QString local = filePath;
    if (local.startsWith(QStringLiteral("file://")))
        local = QUrl(local).toLocalFile();

    QVariantMap out;
    std::shared_ptr<lt::torrent_info> ti;
    try {
        ti = std::make_shared<lt::torrent_info>(local.toStdString());
    } catch (const std::exception &e) {
        out["ok"] = false;
        out["error"] = QString::fromUtf8(e.what());
        return out;
    }
    out["ok"] = true;
    out["name"] = QString::fromStdString(ti->name());
    out["totalSize"] = formatSize(ti->total_size());
    out["totalSizeBytes"] = static_cast<qint64>(ti->total_size());
    out["fileCount"] = ti->num_files();

    // info hash (same convention as SessionManager: info_hashes().get_best())
    QString infoHash = QString::fromStdString(
        (std::ostringstream() << ti->info_hashes().get_best()).str());
    out["infoHash"] = infoHash;

    // poster from metadata cache, if this torrent was seen before
    if (m_resolver && m_resolver->hasCached(infoHash)) {
        auto meta = m_resolver->cached(infoHash);
        if (meta.valid) out["posterPath"] = meta.posterPath;
    }

    QVariantList files;
    const lt::file_storage &fs = ti->files();
    for (int i = 0; i < ti->num_files(); ++i) {
        lt::file_index_t fi(i);
        QVariantMap f;
        f["path"] = QString::fromStdString(fs.file_path(fi));
        f["size"] = formatSize(fs.file_size(fi));
        f["dir"]  = false;
        f["depth"] = 0;
        files << f;
    }
    out["files"] = files;
    return out;
}

void QmlSessionBridge::resolvePreview(const QString &infoHash, const QString &name)
{
    if (m_resolver && !infoHash.isEmpty() && !m_resolver->hasCached(infoHash))
        m_resolver->resolve(infoHash, name);
}

void QmlSessionBridge::addTorrentWithPrefs(const QString &filePath, const QString &savePath,
                                           const QVariantList &priorities)
{
    if (filePath.isEmpty()) return;
    QString local = filePath;
    if (local.startsWith(QStringLiteral("file://")))
        local = QUrl(local).toLocalFile();
    QString dest = savePath;
    if (dest.startsWith(QStringLiteral("file://")))
        dest = QUrl(dest).toLocalFile();
    if (dest.isEmpty()) dest = defaultSavePath();
    rememberSavePath(dest);

    if (priorities.isEmpty()) {
        m_session->addTorrent(local, dest);
    } else {
        std::vector<int> prios;
        prios.reserve(priorities.size());
        for (const auto &v : priorities) prios.push_back(v.toInt());
        m_session->addTorrentWithPriorities(local, dest, prios);
    }
}

