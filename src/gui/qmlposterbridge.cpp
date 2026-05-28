// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "qmlposterbridge.h"
#include "../torrent/sessionmanager.h"
#include "../app/metadataresolver.h"
#include "../app/utils.h"
#include "thememanager.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QGuiApplication>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

QmlPosterModel::QmlPosterModel(SessionManager *session, MetadataResolver *resolver,
                               QObject *parent)
    : QAbstractListModel(parent), m_session(session), m_resolver(resolver)
{
    m_lastCount = m_session->torrentCount();
}

int QmlPosterModel::rowCount(const QModelIndex &) const
{
    return m_session->torrentCount();
}

QVariant QmlPosterModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_session->torrentCount())
        return {};

    TorrentInfo info = m_session->torrentAt(index.row());
    QString hash = m_session->torrentHashAt(index.row());

    switch (role) {
    case NameRole:       return info.name;
    case StateKeyRole: {
        if (info.completed) return QStringLiteral("completed");
        if (info.paused) return QStringLiteral("paused");
        if (info.progress >= 1.0f) return QStringLiteral("seeding");
        return QStringLiteral("downloading");
    }
    case InfoHashRole:   return hash;
    case ProgressRole:   return static_cast<qreal>(info.progress);
    case PosterPathRole: {
        if (m_resolver && m_resolver->hasCached(hash)) {
            auto meta = m_resolver->cached(hash);
            if (meta.valid) return meta.posterPath;
        }
        return QString();
    }
    case MetaTitleRole: {
        if (m_resolver && m_resolver->hasCached(hash)) {
            auto meta = m_resolver->cached(hash);
            if (meta.valid) return meta.title;
        }
        return QString();
    }
    case StateStringRole: return info.stateString;
    case DownSpeedRole:   return formatSpeed(info.downloadRate);
    case UpSpeedRole:     return formatSpeed(info.uploadRate);
    case SizeRole:        return formatSize(info.totalSize);
    }
    return {};
}

QHash<int, QByteArray> QmlPosterModel::roleNames() const
{
    return {
        {NameRole,        "torrentName"},
        {StateKeyRole,    "stateKey"},
        {InfoHashRole,    "infoHash"},
        {ProgressRole,    "progress"},
        {PosterPathRole,  "posterPath"},
        {MetaTitleRole,   "metaTitle"},
        {StateStringRole, "stateString"},
        {DownSpeedRole,   "downSpeed"},
        {UpSpeedRole,     "upSpeed"},
        {SizeRole,        "size"}
    };
}

void QmlPosterModel::refresh()
{
    int newCount = m_session->torrentCount();
    if (newCount > m_lastCount) {
        beginInsertRows(QModelIndex(), m_lastCount, newCount - 1);
        m_lastCount = newCount;
        endInsertRows();
    } else if (newCount < m_lastCount) {
        beginRemoveRows(QModelIndex(), newCount, m_lastCount - 1);
        m_lastCount = newCount;
        endRemoveRows();
    }
    if (newCount > 0)
        emit dataChanged(index(0), index(newCount - 1));
}

// Filter proxy

QmlTorrentFilterProxy::QmlTorrentFilterProxy(QObject *parent)
    : QSortFilterProxyModel(parent) {}

void QmlTorrentFilterProxy::setFilterState(const QString &state)
{
    if (state == m_filterState) return;
    m_filterState = state;
    invalidateFilter();
}

void QmlTorrentFilterProxy::setSearchText(const QString &text)
{
    if (text == m_searchText) return;
    m_searchText = text;
    invalidateFilter();
}

int QmlTorrentFilterProxy::mapToSource(int proxyRow) const
{
    if (proxyRow < 0 || proxyRow >= rowCount()) return -1;
    return QSortFilterProxyModel::mapToSource(index(proxyRow, 0)).row();
}

bool QmlTorrentFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const QAbstractItemModel *src = sourceModel();
    if (!src) return true;
    QModelIndex idx = src->index(sourceRow, 0, sourceParent);

    if (!m_searchText.isEmpty()) {
        QString name = src->data(idx, QmlPosterModel::NameRole).toString();
        QString meta = src->data(idx, QmlPosterModel::MetaTitleRole).toString();
        if (!name.contains(m_searchText, Qt::CaseInsensitive)
            && !meta.contains(m_searchText, Qt::CaseInsensitive))
            return false;
    }

    if (m_filterState == QStringLiteral("all"))
        return true;

    QString key = src->data(idx, QmlPosterModel::StateKeyRole).toString();
    if (m_filterState == QStringLiteral("active"))
        return key == QStringLiteral("downloading") || key == QStringLiteral("seeding");
    return key == m_filterState;
}

// Session bridge

QmlSessionBridge::QmlSessionBridge(SessionManager *session, MetadataResolver *resolver, QObject *parent)
    : QObject(parent), m_session(session), m_resolver(resolver)
{
    m_sampleTimer.setInterval(1000);
    connect(&m_sampleTimer, &QTimer::timeout, this, &QmlSessionBridge::sampleSpeeds);
    m_sampleTimer.start();
}

void QmlSessionBridge::sampleSpeeds()
{
    int dl = 0, ul = 0;
    for (int i = 0; i < m_session->torrentCount(); ++i) {
        auto info = m_session->torrentAt(i);
        dl += info.downloadRate;
        ul += info.uploadRate;
    }
    m_downloadHistory.append(dl);
    m_uploadHistory.append(ul);
    while (m_downloadHistory.size() > HistoryMaxPoints) m_downloadHistory.removeFirst();
    while (m_uploadHistory.size() > HistoryMaxPoints) m_uploadHistory.removeFirst();
    emit historyChanged();
}

QVariantList QmlSessionBridge::downloadHistory() const
{
    QVariantList out;
    out.reserve(m_downloadHistory.size());
    for (int v : m_downloadHistory) out << v;
    return out;
}

QVariantList QmlSessionBridge::uploadHistory() const
{
    QVariantList out;
    out.reserve(m_uploadHistory.size());
    for (int v : m_uploadHistory) out << v;
    return out;
}

int QmlSessionBridge::historyMaxBytes() const
{
    int m = 1024;
    for (int v : m_downloadHistory) if (v > m) m = v;
    for (int v : m_uploadHistory) if (v > m) m = v;
    return m;
}

int QmlSessionBridge::torrentCount() const { return m_session->torrentCount(); }

int QmlSessionBridge::activeCount() const
{
    int n = 0;
    for (int i = 0; i < m_session->torrentCount(); ++i) {
        auto info = m_session->torrentAt(i);
        if (!info.paused && info.progress < 1.0f) ++n;
    }
    return n;
}

int QmlSessionBridge::downloadingCount() const
{
    int n = 0;
    for (int i = 0; i < m_session->torrentCount(); ++i) {
        auto info = m_session->torrentAt(i);
        if (!info.paused && info.progress < 1.0f) ++n;
    }
    return n;
}

int QmlSessionBridge::seedingCount() const
{
    int n = 0;
    for (int i = 0; i < m_session->torrentCount(); ++i) {
        auto info = m_session->torrentAt(i);
        if (!info.paused && info.progress >= 1.0f && !info.completed) ++n;
    }
    return n;
}

int QmlSessionBridge::pausedCount() const
{
    int n = 0;
    for (int i = 0; i < m_session->torrentCount(); ++i)
        if (m_session->torrentAt(i).paused) ++n;
    return n;
}

int QmlSessionBridge::completedCount() const
{
    int n = 0;
    for (int i = 0; i < m_session->torrentCount(); ++i)
        if (m_session->torrentAt(i).completed) ++n;
    return n;
}

QString QmlSessionBridge::totalDownSpeed() const
{
    int total = 0;
    for (int i = 0; i < m_session->torrentCount(); ++i)
        total += m_session->torrentAt(i).downloadRate;
    return formatSpeed(total);
}

QString QmlSessionBridge::totalUpSpeed() const
{
    int total = 0;
    for (int i = 0; i < m_session->torrentCount(); ++i)
        total += m_session->torrentAt(i).uploadRate;
    return formatSpeed(total);
}

QString QmlSessionBridge::totalDownloaded() const { return formatSize(m_session->globalDownloaded()); }
QString QmlSessionBridge::totalUploaded() const { return formatSize(m_session->globalUploaded()); }
QString QmlSessionBridge::globalRatio() const { return QString::number(m_session->globalRatio(), 'f', 2); }

void QmlSessionBridge::setSelectedIndex(int index)
{
    if (index == m_selectedIndex) return;
    m_selectedIndex = index;
    emit selectionChanged();
}

void QmlSessionBridge::pauseSelected()
{
    if (m_selectedIndex >= 0) m_session->pauseTorrent(m_selectedIndex);
}

void QmlSessionBridge::resumeSelected()
{
    if (m_selectedIndex >= 0) m_session->resumeTorrent(m_selectedIndex);
}

void QmlSessionBridge::removeSelected()
{
    if (m_selectedIndex >= 0) {
        m_session->removeTorrent(m_selectedIndex, false);
        m_selectedIndex = -1;
        emit selectionChanged();
    }
}

void QmlSessionBridge::removeSelectedWithFiles()
{
    if (m_selectedIndex >= 0) {
        m_session->removeTorrent(m_selectedIndex, true);
        m_selectedIndex = -1;
        emit selectionChanged();
    }
}

void QmlSessionBridge::pauseAll() { m_session->pauseAll(); }
void QmlSessionBridge::resumeAll() { m_session->resumeAll(); }

void QmlSessionBridge::copyMagnetLink()
{
    if (!hasSelection()) return;
    QString uri = m_session->torrentMagnetUri(m_selectedIndex);
    if (!uri.isEmpty()) QGuiApplication::clipboard()->setText(uri);
}

void QmlSessionBridge::copyInfoHash()
{
    if (!hasSelection()) return;
    QString hash = m_session->torrentHashAt(m_selectedIndex);
    if (!hash.isEmpty()) QGuiApplication::clipboard()->setText(hash);
}

void QmlSessionBridge::openSaveFolder()
{
    if (!hasSelection()) return;
    QString path = m_session->torrentAt(m_selectedIndex).savePath;
    if (!path.isEmpty()) QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

bool QmlSessionBridge::selectedForceStart() const
{
    return hasSelection() && m_session->isForceStart(m_selectedIndex);
}

bool QmlSessionBridge::selectedSuperSeeding() const
{
    return hasSelection() && m_session->isSuperSeeding(m_selectedIndex);
}

bool QmlSessionBridge::selectedCompleted() const
{
    return hasSelection() && m_session->torrentAt(m_selectedIndex).completed;
}

bool QmlSessionBridge::selectedAtFullProgress() const
{
    return hasSelection() && m_session->torrentAt(m_selectedIndex).progress >= 1.0f;
}

void QmlSessionBridge::setSelectedForceStart(bool on)
{
    if (hasSelection()) m_session->setForceStart(m_selectedIndex, on);
    emit selectionChanged();
}

void QmlSessionBridge::setSelectedSuperSeeding(bool on)
{
    if (hasSelection()) m_session->setSuperSeeding(m_selectedIndex, on);
    emit selectionChanged();
}

void QmlSessionBridge::markSelectedCompleted()
{
    if (hasSelection()) m_session->markCompleted(m_selectedIndex);
    emit selectionChanged();
}

void QmlSessionBridge::unmarkSelectedCompleted()
{
    if (hasSelection()) m_session->unmarkCompleted(m_selectedIndex);
    emit selectionChanged();
}

void QmlSessionBridge::forceRecheckSelected()
{
    if (hasSelection()) m_session->forceRecheck(m_selectedIndex);
}

void QmlSessionBridge::forceReannounceSelected()
{
    if (hasSelection()) m_session->forceReannounce(m_selectedIndex);
}

void QmlSessionBridge::queueUpSelected()
{
    if (!hasSelection() || m_selectedIndex <= 0) return;
    m_session->setTorrentQueuePosition(m_selectedIndex, m_selectedIndex - 1);
}

void QmlSessionBridge::queueDownSelected()
{
    if (!hasSelection() || m_selectedIndex >= m_session->torrentCount() - 1) return;
    m_session->setTorrentQueuePosition(m_selectedIndex, m_selectedIndex + 1);
}

QVariantList QmlSessionBridge::selectedPeerList() const
{
    QVariantList out;
    if (!hasSelection()) return out;
    auto peers = m_session->peersAt(m_selectedIndex);
    out.reserve(peers.size());
    for (const auto &p : peers) {
        QVariantMap m;
        m["ip"]       = p.ip;
        m["port"]     = p.port;
        m["client"]   = p.client;
        m["downSpeed"]= formatSpeed(p.downloadRate);
        m["upSpeed"]  = formatSpeed(p.uploadRate);
        m["progress"] = p.progress;
        out << m;
    }
    return out;
}

QVariantList QmlSessionBridge::selectedFiles() const
{
    QVariantList out;
    if (!hasSelection()) return out;
    auto files = m_session->filesAt(m_selectedIndex);
    out.reserve(files.size());
    for (const auto &f : files) {
        QVariantMap m;
        m["path"]     = f.path;
        m["size"]     = formatSize(f.size);
        m["progress"] = f.progress;
        m["priority"] = f.priority;
        out << m;
    }
    return out;
}

QVariantList QmlSessionBridge::selectedTrackers() const
{
    QVariantList out;
    if (!hasSelection()) return out;
    auto trackers = m_session->trackersAt(m_selectedIndex);
    out.reserve(trackers.size());
    for (const auto &t : trackers) {
        QVariantMap m;
        m["url"]    = t.url;
        m["tier"]   = t.tier;
        m["status"] = t.status;
        out << m;
    }
    return out;
}

QVariantList QmlSessionBridge::selectedPieces() const
{
    QVariantList out;
    if (!hasSelection()) return out;
    auto pieces = m_session->piecesAt(m_selectedIndex);
    out.reserve(pieces.size());
    for (bool b : pieces) out << b;
    return out;
}

QString QmlSessionBridge::defaultSavePath() const
{
    QSettings s;
    QString p = s.value(QStringLiteral("lastSavePath")).toString();
    if (!p.isEmpty() && QDir(p).exists()) return p;
    return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
}

void QmlSessionBridge::addTorrentFile(const QString &filePath)
{
    if (filePath.isEmpty()) return;
    QString local = filePath;
    if (local.startsWith(QStringLiteral("file://")))
        local = QUrl(local).toLocalFile();
    m_session->addTorrent(local, defaultSavePath());
}

void QmlSessionBridge::addMagnetUri(const QString &uri)
{
    if (uri.isEmpty()) return;
    m_session->addMagnet(uri, defaultSavePath());
}

bool QmlSessionBridge::hasSelection() const
{
    return m_selectedIndex >= 0 && m_selectedIndex < m_session->torrentCount();
}

QString QmlSessionBridge::selectedName() const
{
    if (!hasSelection()) return {};
    return m_session->torrentAt(m_selectedIndex).name;
}

QString QmlSessionBridge::selectedSavePath() const
{
    if (!hasSelection()) return {};
    return m_session->torrentAt(m_selectedIndex).savePath;
}

QString QmlSessionBridge::selectedSize() const
{
    if (!hasSelection()) return {};
    return formatSize(m_session->torrentAt(m_selectedIndex).totalSize);
}

QString QmlSessionBridge::selectedHash() const
{
    if (!hasSelection()) return {};
    QString h = m_session->torrentHashAt(m_selectedIndex);
    if (h.size() > 14) return h.left(8) + QStringLiteral("…") + h.right(4);
    return h;
}

QString QmlSessionBridge::selectedDownloaded() const
{
    if (!hasSelection()) return {};
    auto info = m_session->torrentAt(m_selectedIndex);
    return QString("%1 (%2%)").arg(formatSize(info.totalDone))
                              .arg(info.progress * 100.0, 0, 'f', 1);
}

QString QmlSessionBridge::selectedUploaded() const
{
    if (!hasSelection()) return {};
    auto info = m_session->torrentAt(m_selectedIndex);
    return formatSize(static_cast<qint64>(info.totalDone * info.ratio));
}

QString QmlSessionBridge::selectedSpeed() const
{
    if (!hasSelection()) return {};
    auto info = m_session->torrentAt(m_selectedIndex);
    return QString("↓ %1   ↑ %2").arg(formatSpeed(info.downloadRate), formatSpeed(info.uploadRate));
}

QString QmlSessionBridge::selectedEta() const
{
    if (!hasSelection()) return QStringLiteral("—");
    auto info = m_session->torrentAt(m_selectedIndex);
    if (info.downloadRate <= 0 || info.totalSize <= info.totalDone) return QStringLiteral("—");
    qint64 secs = (info.totalSize - info.totalDone) / info.downloadRate;
    if (secs < 60) return QString("%1s").arg(secs);
    if (secs < 3600) return QString("%1m %2s").arg(secs / 60).arg(secs % 60);
    return QString("%1h %2m").arg(secs / 3600).arg((secs % 3600) / 60);
}

int QmlSessionBridge::selectedSeeds() const
{
    return hasSelection() ? m_session->torrentAt(m_selectedIndex).numSeeds : 0;
}

int QmlSessionBridge::selectedPeers() const
{
    return hasSelection() ? m_session->torrentAt(m_selectedIndex).numPeers : 0;
}

QString QmlSessionBridge::selectedRatio() const
{
    if (!hasSelection()) return {};
    return QString::number(m_session->torrentAt(m_selectedIndex).ratio, 'f', 2);
}

QString QmlSessionBridge::selectedState() const
{
    if (!hasSelection()) return {};
    return m_session->torrentAt(m_selectedIndex).stateString;
}

QString QmlSessionBridge::selectedPoster() const
{
    if (!hasSelection() || !m_resolver) return {};
    QString hash = m_session->torrentHashAt(m_selectedIndex);
    if (m_resolver->hasCached(hash)) {
        auto meta = m_resolver->cached(hash);
        if (meta.valid) return meta.posterPath;
    }
    return {};
}

QString QmlSessionBridge::selectedDescription() const
{
    if (!hasSelection() || !m_resolver) return {};
    QString hash = m_session->torrentHashAt(m_selectedIndex);
    if (m_resolver->hasCached(hash)) {
        auto meta = m_resolver->cached(hash);
        if (meta.valid) return meta.description;
    }
    return {};
}

QString QmlSessionBridge::selectedMetaTitle() const
{
    if (!hasSelection() || !m_resolver) return {};
    QString hash = m_session->torrentHashAt(m_selectedIndex);
    if (m_resolver->hasCached(hash)) {
        auto meta = m_resolver->cached(hash);
        if (meta.valid) return meta.title;
    }
    return {};
}

QString QmlSessionBridge::selectedMetaInfo() const
{
    if (!hasSelection() || !m_resolver) return {};
    QString hash = m_session->torrentHashAt(m_selectedIndex);
    if (!m_resolver->hasCached(hash)) return {};
    auto meta = m_resolver->cached(hash);
    if (!meta.valid) return {};
    QStringList parts;
    if (meta.year > 0) parts << QString::number(meta.year);
    if (!meta.genres.isEmpty()) parts << meta.genres.mid(0, 3).join(QStringLiteral(", "));
    if (meta.rating > 0) parts << QStringLiteral("%1/10").arg(meta.rating, 0, 'f', 1);
    return parts.join(QStringLiteral(" · "));
}

void QmlSessionBridge::emitStats()
{
    emit statsChanged();
    emit selectionChanged();
}

// Theme bridge

QmlThemeBridge::QmlThemeBridge(QObject *parent) : QObject(parent) {}

QColor QmlThemeBridge::bg() const { return QColor(ThemeManager::instance().bgColor()); }
QColor QmlThemeBridge::surface() const { return QColor(ThemeManager::instance().surfaceColor()); }
QColor QmlThemeBridge::panel() const { return QColor(ThemeManager::instance().panelColor()); }
QColor QmlThemeBridge::surfaceAlt() const { return QColor(ThemeManager::instance().surfaceAltColor()); }
QColor QmlThemeBridge::accent() const { return QColor(ThemeManager::instance().accentColor()); }
QColor QmlThemeBridge::accentDark() const { return QColor(ThemeManager::instance().accentDarkColor()); }
QColor QmlThemeBridge::accentLight() const { return QColor(ThemeManager::instance().accentLightColor()); }
QColor QmlThemeBridge::text() const { return QColor(ThemeManager::instance().textColor()); }
QColor QmlThemeBridge::muted() const { return QColor(ThemeManager::instance().mutedColor()); }
QColor QmlThemeBridge::dim() const { return QColor(ThemeManager::instance().dimColor()); }
QColor QmlThemeBridge::border() const { return QColor(ThemeManager::instance().borderColor()); }
QColor QmlThemeBridge::stateDownloading() const { return QColor(ThemeManager::instance().stateDownloadingColor()); }
QColor QmlThemeBridge::stateSeeding() const { return QColor(ThemeManager::instance().stateSeedingColor()); }
QColor QmlThemeBridge::stateCompleted() const { return QColor(ThemeManager::instance().stateCompletedColor()); }
QColor QmlThemeBridge::statePaused() const { return QColor(ThemeManager::instance().statePausedColor()); }
bool QmlThemeBridge::isSakura() const { return ThemeManager::instance().theme() == ThemeManager::Sakura; }

void QmlThemeBridge::emitChanged() { emit changed(); }
