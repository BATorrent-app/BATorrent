// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "enginehost.h"
#include "ipcprotocol.h"
#include "ipcserialize.h"
#include "../torrent/sessionmanager.h"
#include <QLocalServer>
#include <QLocalSocket>
#include <QDataStream>
#include <QDebug>

EngineHost::EngineHost(SessionManager *session, const QString &serverName, QObject *parent)
    : QObject(parent), m_session(session), m_serverName(serverName), m_server(new QLocalServer(this))
{
    connect(m_server, &QLocalServer::newConnection, this, &EngineHost::onNewConnection);

    // Forward the engine's reactive surface to the UI as events.
    connect(m_session, &SessionManager::torrentsUpdated, this, &EngineHost::pushSnapshot);
    connect(m_session, &SessionManager::torrentFinished, this, [this](const QString &name, const QString &hash) {
        QByteArray a; QDataStream o(&a, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion);
        o << name << hash; sendEvent(QStringLiteral("torrentFinished"), a);
    });
    connect(m_session, &SessionManager::extractionCompleted, this, [this](const QString &hash, bool ok) {
        QByteArray a; QDataStream o(&a, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion);
        o << hash << ok; sendEvent(QStringLiteral("extractionCompleted"), a);
    });
    connect(m_session, &SessionManager::altSpeedsActiveChanged, this, [this](bool active) {
        QByteArray a; QDataStream o(&a, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion);
        o << active; sendEvent(QStringLiteral("altSpeedsActiveChanged"), a);
    });
    connect(m_session, &SessionManager::portStatusChanged, this, [this](int status) {
        QByteArray a; QDataStream o(&a, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion);
        o << qint32(status); sendEvent(QStringLiteral("portStatusChanged"), a);
    });
}

bool EngineHost::listen()
{
    QLocalServer::removeServer(m_serverName);   // clear a stale socket from a previous crash
    if (!m_server->listen(m_serverName)) {
        qWarning() << "[engine] failed to listen on" << m_serverName << m_server->errorString();
        return false;
    }
    qInfo() << "[engine] listening on" << m_serverName;
    return true;
}

void EngineHost::onNewConnection()
{
    QLocalSocket *sock = m_server->nextPendingConnection();
    if (!sock) return;
    // One UI at a time; a reconnect (after a UI restart) replaces the old socket.
    if (m_client) m_client->deleteLater();
    m_client = sock;
    m_buf.clear();
    connect(sock, &QLocalSocket::readyRead, this, &EngineHost::onReadyRead);
    connect(sock, &QLocalSocket::disconnected, sock, &QLocalSocket::deleteLater);
    ipc::writeFrame(sock, ipc::Kind::Hello, {});
    pushSnapshot();   // give the UI initial state without waiting for the next tick
}

void EngineHost::onReadyRead()
{
    if (!m_client) return;
    m_buf.append(m_client->readAll());
    ipc::drainFrames(m_buf, [this](ipc::Kind kind, const QByteArray &payload) {
        if (kind == ipc::Kind::Ping) {
            ipc::writeFrame(m_client, ipc::Kind::Pong, {});
            return;
        }
        if (kind == ipc::Kind::Request) {
            QDataStream in(payload);
            in.setVersion(ipc::kStreamVersion);
            quint32 id = 0; QString method; QByteArray args;
            in >> id >> method >> args;
            const QByteArray result = dispatch(method, args);
            QByteArray reply;
            QDataStream out(&reply, QIODevice::WriteOnly);
            out.setVersion(ipc::kStreamVersion);
            out << id << result;
            ipc::writeFrame(m_client, ipc::Kind::Reply, reply);
        }
    });
}

void EngineHost::sendEvent(const QString &name, const QByteArray &args)
{
    if (!m_client) return;
    QByteArray p;
    QDataStream o(&p, QIODevice::WriteOnly);
    o.setVersion(ipc::kStreamVersion);
    o << name << args;
    ipc::writeFrame(m_client, ipc::Kind::Event, p);
}

void EngineHost::pushSnapshot()
{
    if (!m_client) return;
    EngineSnapshot snap;
    const int n = m_session->torrentCount();
    snap.rows.reserve(n);
    for (int i = 0; i < n; ++i) {
        snap.hashes << m_session->torrentHashAt(i);
        snap.rows << m_session->torrentAt(i);
    }
    snap.globalUploaded = m_session->globalUploaded();
    snap.globalDownloaded = m_session->globalDownloaded();
    snap.sessionUploaded = m_session->sessionUploaded();
    snap.sessionDownloaded = m_session->sessionDownloaded();
    snap.globalRatio = m_session->globalRatio();
    snap.totalTorrentsAdded = m_session->totalTorrentsAdded();
    snap.portStatus = m_session->portStatus();
    snap.listenPort = m_session->listenPort();
    snap.dhtEnabled = m_session->dhtEnabled();
    snap.altSpeedsActive = m_session->altSpeedsActive();
    snap.detailed = m_session->detailedStats();

    QByteArray a;
    QDataStream o(&a, QIODevice::WriteOnly);
    o.setVersion(ipc::kStreamVersion);
    o << snap;
    sendEvent(QStringLiteral("snapshot"), a);
}

// Decode args → call m_session → encode result. Hot-path reads (torrentAt,
// counts, globals) are served UI-side from the snapshot and never reach here;
// this handles mutations and the per-selection detail queries.
QByteArray EngineHost::dispatch(const QString &method, const QByteArray &args)
{
    QByteArray result;
    QDataStream out(&result, QIODevice::WriteOnly);
    out.setVersion(ipc::kStreamVersion);

    QDataStream in(args);
    in.setVersion(ipc::kStreamVersion);

    auto idx = [&]() { qint32 i = 0; in >> i; return i; };

    // ---- mutations (fire-and-forget; empty reply) ----
    if (method == QLatin1String("pauseAll")) {
        m_session->pauseAll();
    } else if (method == QLatin1String("resumeAll")) {
        m_session->resumeAll();
    } else if (method == QLatin1String("pauseTorrent")) {
        m_session->pauseTorrent(idx());
    } else if (method == QLatin1String("resumeTorrent")) {
        m_session->resumeTorrent(idx());
    } else if (method == QLatin1String("removeTorrent")) {
        qint32 i; bool del, perm; in >> i >> del >> perm;
        m_session->removeTorrent(i, del, perm);
    } else if (method == QLatin1String("setFilePriority")) {
        qint32 ti, fi, pr; in >> ti >> fi >> pr;
        m_session->setFilePriority(ti, fi, pr);
    } else if (method == QLatin1String("setTorrentQueuePosition")) {
        qint32 i, p; in >> i >> p; m_session->setTorrentQueuePosition(i, p);
    } else if (method == QLatin1String("setSequentialDownload")) {
        qint32 i; bool seq; in >> i >> seq; m_session->setSequentialDownload(i, seq);
    } else if (method == QLatin1String("prioritizeFilePieceBoundaries")) {
        qint32 ti, fi; in >> ti >> fi; m_session->prioritizeFilePieceBoundaries(ti, fi);
    } else if (method == QLatin1String("addTorrent")) {
        QString fp, sp; in >> fp >> sp; m_session->addTorrent(fp, sp);
    } else if (method == QLatin1String("addTorrentWithPriorities")) {
        QString fp, sp; std::vector<int> pr; qint32 n;
        in >> fp >> sp >> n; pr.reserve(n);
        for (qint32 k = 0; k < n; ++k) { qint32 v; in >> v; pr.push_back(v); }
        m_session->addTorrentWithPriorities(fp, sp, pr);
    } else if (method == QLatin1String("addMagnet")) {
        QString uri, sp, hint; qint32 ct; in >> uri >> sp >> hint >> ct;
        m_session->addMagnet(uri, sp, hint, ct);
    } else if (method == QLatin1String("renameFile")) {
        qint32 ti, fi; QString p; in >> ti >> fi >> p; m_session->renameFile(ti, fi, p);
    } else if (method == QLatin1String("extractTorrent")) {
        qint32 i; QString pw; in >> i >> pw; m_session->extractTorrent(i, pw);
    } else if (method == QLatin1String("unmarkCompleted")) {
        m_session->unmarkCompleted(idx());
    } else if (method == QLatin1String("markCompleted")) {
        m_session->markCompleted(idx());
    } else if (method == QLatin1String("stopSeedingTorrent")) {
        m_session->stopSeedingTorrent(idx());
    } else if (method == QLatin1String("setTorrentUploadLimit")) {
        qint32 i, k; in >> i >> k; m_session->setTorrentUploadLimit(i, k);
    } else if (method == QLatin1String("setTorrentDownloadLimit")) {
        qint32 i, k; in >> i >> k; m_session->setTorrentDownloadLimit(i, k);
    } else if (method == QLatin1String("setTorrentTags")) {
        qint32 i; QStringList t; in >> i >> t; m_session->setTorrentTags(i, t);
    } else if (method == QLatin1String("setTorrentStopAfterDownload")) {
        qint32 i, v; in >> i >> v; m_session->setTorrentStopAfterDownload(i, v);
    } else if (method == QLatin1String("setTorrentMaxSeedSeconds")) {
        qint32 i; qint64 s; in >> i >> s; m_session->setTorrentMaxSeedSeconds(i, s);
    } else if (method == QLatin1String("setTorrentCategory")) {
        qint32 i; QString c; in >> i >> c; m_session->setTorrentCategory(i, c);
    } else if (method == QLatin1String("setSuperSeeding")) {
        qint32 i; bool on; in >> i >> on; m_session->setSuperSeeding(i, on);
    } else if (method == QLatin1String("setForceStart")) {
        qint32 i; bool on; in >> i >> on; m_session->setForceStart(i, on);
    } else if (method == QLatin1String("setAltSpeedsActive")) {
        bool a; in >> a; m_session->setAltSpeedsActive(a);
    } else if (method == QLatin1String("saveResumeData")) {
        m_session->saveResumeData();
    } else if (method == QLatin1String("moveStorage")) {
        qint32 ti; QString p; in >> ti >> p; m_session->moveStorage(ti, p);
    } else if (method == QLatin1String("forceRecheck")) {
        m_session->forceRecheck(idx());
    } else if (method == QLatin1String("forceReannounce")) {
        m_session->forceReannounce(idx());
    } else if (method == QLatin1String("addTracker")) {
        qint32 i; QString u; in >> i >> u; m_session->addTracker(i, u);
    } else if (method == QLatin1String("replaceTrackers")) {
        qint32 ti; QStringList u; in >> ti >> u; m_session->replaceTrackers(ti, u);
    } else if (method == QLatin1String("clearRemovedHistory")) {
        m_session->clearRemovedHistory();

    // ---- queries (non-empty reply) ----
    } else if (method == QLatin1String("torrentCount")) {
        out << qint32(m_session->torrentCount());
    } else if (method == QLatin1String("filesAt")) {
        writeVec(out, m_session->filesAt(idx()));
    } else if (method == QLatin1String("trackersAt")) {
        writeVec(out, m_session->trackersAt(idx()));
    } else if (method == QLatin1String("peersAt")) {
        qint32 i, mx; in >> i >> mx; writeVec(out, m_session->peersAt(i, mx));
    } else if (method == QLatin1String("piecesAt")) {
        writeBoolVec(out, m_session->piecesAt(idx()));
    } else if (method == QLatin1String("torrentRootPath")) {
        out << m_session->torrentRootPath(idx());
    } else if (method == QLatin1String("torrentHasVideo")) {
        out << m_session->torrentHasVideo(idx());
    } else if (method == QLatin1String("torrentHasArchives")) {
        out << m_session->torrentHasArchives(idx());
    } else if (method == QLatin1String("torrentDownloadLimit")) {
        out << qint32(m_session->torrentDownloadLimit(idx()));
    } else if (method == QLatin1String("torrentUploadLimit")) {
        out << qint32(m_session->torrentUploadLimit(idx()));
    } else if (method == QLatin1String("torrentTags")) {
        out << m_session->torrentTags(idx());
    } else if (method == QLatin1String("torrentStopAfterDownload")) {
        out << qint32(m_session->torrentStopAfterDownload(idx()));
    } else if (method == QLatin1String("torrentMaxSeedSeconds")) {
        out << qint64(m_session->torrentMaxSeedSeconds(idx()));
    } else if (method == QLatin1String("torrentMagnetUri")) {
        out << m_session->torrentMagnetUri(idx());
    } else if (method == QLatin1String("torrentFileNames")) {
        out << m_session->torrentFileNames(idx());
    } else if (method == QLatin1String("streamFilePath")) {
        qint32 ti, fi; in >> ti >> fi; out << m_session->streamFilePath(ti, fi);
    } else if (method == QLatin1String("streamFileStats")) {
        qint32 ti, fi; in >> ti >> fi; out << m_session->streamFileStats(ti, fi);
    } else if (method == QLatin1String("statsWrapped")) {
        out << m_session->statsWrapped(idx());
    } else if (method == QLatin1String("isSuperSeeding")) {
        out << m_session->isSuperSeeding(idx());
    } else if (method == QLatin1String("isSequentialDownload")) {
        out << m_session->isSequentialDownload(idx());
    } else if (method == QLatin1String("isForceStart")) {
        out << m_session->isForceStart(idx());
    } else if (method == QLatin1String("restoreRemoved")) {
        QString h; in >> h; out << m_session->restoreRemoved(h);
    } else if (method == QLatin1String("recentlyRemoved")) {
        out << m_session->recentlyRemoved();
    } else if (method == QLatin1String("importFromQBittorrent")) {
        QString p; in >> p; out << qint32(m_session->importFromQBittorrent(p));
    } else if (method == QLatin1String("exportTorrent")) {
        qint32 i; QString d; in >> i >> d; out << m_session->exportTorrent(i, d);
    } else if (method == QLatin1String("categories")) {
        out << m_session->categories();
    } else if (method == QLatin1String("allCategorySavePaths")) {
        out << m_session->allCategorySavePaths();

    // ---- proxy / advanced ----
    } else if (method == QLatin1String("setProxySettings")) {
        qint32 type, port; QString host, user, pass;
        in >> type >> host >> port >> user >> pass;
        m_session->setProxySettings(type, host, port, user, pass);
    } else if (method == QLatin1String("proxyType")) {
        out << qint32(m_session->proxyType());
    } else if (method == QLatin1String("proxyHost")) {
        out << m_session->proxyHost();
    } else if (method == QLatin1String("proxyPort")) {
        out << qint32(m_session->proxyPort());
    } else if (method == QLatin1String("proxyUser")) {
        out << m_session->proxyUser();
    } else if (method == QLatin1String("proxyPass")) {
        out << m_session->proxyPass();
    } else if (method == QLatin1String("advancedSettings")) {
        out << m_session->advancedSettings();
    } else if (method == QLatin1String("setAdvancedSettings")) {
        AdvancedSettings a; in >> a; m_session->setAdvancedSettings(a);

    // ---- streaming hooks ----
    } else if (method == QLatin1String("streamFileSize")) {
        qint32 ti, fi; in >> ti >> fi; out << qint64(m_session->streamFileSize(ti, fi));
    } else if (method == QLatin1String("streamContiguousAvailableBytes")) {
        qint32 ti, fi; qint64 from, cap; in >> ti >> fi >> from >> cap;
        out << qint64(m_session->streamContiguousAvailableBytes(ti, fi, from, cap));
    } else if (method == QLatin1String("streamSetDeadlineWindow")) {
        qint32 ti, fi, win; qint64 start; in >> ti >> fi >> start >> win;
        m_session->streamSetDeadlineWindow(ti, fi, start, win);
    } else {
        qWarning() << "[engine] unhandled method" << method;
    }
    return result;
}
