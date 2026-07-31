// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/session/qmlsessionbridge.h"
#include "torrent/iengine.h"
#include "services/downloads/httpdownloadmanager.h"
#include <QThread>
#include <QStorageInfo>
#include "services/metadata/metadataresolver.h"
#include "services/discovery/discoveryservice.h"
#include "services/security/defender.h"
#include "services/metadata/nameparser.h"
#include "services/integrations/rssmanager.h"
#include "services/discovery/addonmanager.h"
#include "services/platform/utils.h"
#include "services/platform/translator.h"
#include "services/integrations/geoip.h"
#include "webui/webserver.h"
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDateTime>

#include <QNetworkInterface>

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#if defined(Q_OS_WIN)
#include <windows.h>
#else
#include <csignal>
#include <cerrno>
#endif
#include <QApplication>
#include <QWindow>
#include <QEvent>
#ifdef Q_OS_WIN
#  include <windows.h>
#  include <dwmapi.h>
#  ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#    define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#  endif
#endif
#include <QCoreApplication>
#include <QStyleHints>
#include <QPainter>
#include <QSvgRenderer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <cstring>
#include <algorithm>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

#include <libtorrent/torrent_info.hpp>
#include <libtorrent/file_storage.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/version.hpp>
#include <openssl/opensslv.h>
#include <boost/version.hpp>
#include <memory>
#include <sstream>

QmlSessionBridge::QmlSessionBridge(IEngine *session, MetadataResolver *resolver, QObject *parent)
    : QObject(parent), m_session(session), m_resolver(resolver)
{
    m_sampleTimer.setInterval(1000);
    connect(&m_sampleTimer, &QTimer::timeout, this, &QmlSessionBridge::sampleSpeeds);
    m_sampleTimer.start();
    recomputeAggregates();   // so the pills show real counts before the first tick

    m_geoIp = new GeoIpResolver(this);
    // A big swarm resolves hundreds of peer IPs one-by-one; emitting on each one
    // rebuilt the whole peer list every time, and each rebuild re-queued lookups
    // — a feedback storm that lagged forever. Coalesce into ≤1 rebuild/sec.
    m_peerListThrottle.setSingleShot(true);
    m_peerListThrottle.setInterval(1000);
    connect(&m_peerListThrottle, &QTimer::timeout, this, [this]() {
        emit selectionChanged(); emit selectionListsChanged();
    });
    connect(m_geoIp, &GeoIpResolver::resolved, this, [this](const QString &, const QString &) {
        if (!m_peerListThrottle.isActive()) m_peerListThrottle.start();
    });

    if (m_resolver) {
        connect(m_resolver, &MetadataResolver::metadataReady, this,
                [this](const QString &infoHash) {
            if (!m_resolver->hasCached(infoHash)) return;
            auto meta = m_resolver->cached(infoHash);
            if (meta.valid && !meta.posterPath.isEmpty())
                emit previewPosterReady(infoHash, meta.posterPath);
        });
    }

    connect(m_session, &IEngine::altSpeedsActiveChanged,
            this, &QmlSessionBridge::altSpeedsActiveChanged);
    connect(m_session, &IEngine::portStatusChanged,
            this, &QmlSessionBridge::portStatusChanged);
    connect(m_session, &IEngine::torrentsUpdated,
            this, &QmlSessionBridge::onWatchTick);
    connect(m_session, &IEngine::extractionCompleted,
            this, &QmlSessionBridge::onExtractionCompleted);
    connect(m_session, &IEngine::torrentFinished,
            this, &QmlSessionBridge::onGameTorrentFinished);
}

bool QmlSessionBridge::altSpeedsActive() const { return m_session->altSpeedsActive(); }
int QmlSessionBridge::portStatus() const { return m_session->portStatus(); }
void QmlSessionBridge::setAltSpeedsActive(bool active) { m_session->setAltSpeedsActive(active); }




// Playback/streaming (stream URLs, subtitles, play-by-hash, next-episode,
// watch-when-ready) lives in qmlsessionbridge_playback.cpp; the movie library +
// watchlist projections in qmlsessionbridge_library.cpp.
