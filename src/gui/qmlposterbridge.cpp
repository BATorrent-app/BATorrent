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
#include <QRegularExpression>
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
    case CategoryRole:    return info.category;
    case NumPeersRole:    return info.numPeers;
    case DownRateRole:    return info.downloadRate;
    case UpRateRole:      return info.uploadRate;
    case SizeBytesRole:   return static_cast<qint64>(info.totalSize);
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
        {CategoryRole,    "category"},
        {NumPeersRole,    "numPeers"},
        {DownRateRole,    "downRate"},
        {UpRateRole,      "upRate"},
        {SizeRole,        "size"}
    };
}

void QmlPosterModel::moveRow(int from, int to)
{
    if (from < 0 || to < 0 || from == to) return;
    int count = m_session->torrentCount();
    if (from >= count || to >= count) return;
    int destRow = to > from ? to + 1 : to;
    beginMoveRows(QModelIndex(), from, from, QModelIndex(), destRow);
    endMoveRows();
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

void QmlTorrentFilterProxy::setSortColumn(const QString &column, bool ascending)
{
    m_sortColumn = column;
    sort(0, ascending ? Qt::AscendingOrder : Qt::DescendingOrder);
}

void QmlTorrentFilterProxy::clearSort()
{
    m_sortColumn.clear();
    sort(-1);
}

bool QmlTorrentFilterProxy::lessThan(const QModelIndex &l, const QModelIndex &r) const
{
    const QAbstractItemModel *src = sourceModel();
    if (!src) return false;

    auto raw = [&](int role) {
        return std::make_pair(src->data(l, role), src->data(r, role));
    };

    if (m_sortColumn == QStringLiteral("name") || m_sortColumn.isEmpty()) {
        auto [a, b] = raw(QmlPosterModel::NameRole);
        return a.toString().toLower() < b.toString().toLower();
    }
    if (m_sortColumn == QStringLiteral("size")) {
        auto [a, b] = raw(QmlPosterModel::SizeBytesRole);
        return a.toLongLong() < b.toLongLong();
    }
    if (m_sortColumn == QStringLiteral("progress")) {
        auto [a, b] = raw(QmlPosterModel::ProgressRole);
        return a.toReal() < b.toReal();
    }
    if (m_sortColumn == QStringLiteral("down")) {
        auto [a, b] = raw(QmlPosterModel::DownRateRole);
        return a.toInt() < b.toInt();
    }
    if (m_sortColumn == QStringLiteral("up")) {
        auto [a, b] = raw(QmlPosterModel::UpRateRole);
        return a.toInt() < b.toInt();
    }
    if (m_sortColumn == QStringLiteral("state")) {
        auto [a, b] = raw(QmlPosterModel::StateStringRole);
        return a.toString() < b.toString();
    }
    if (m_sortColumn == QStringLiteral("category")) {
        auto [a, b] = raw(QmlPosterModel::CategoryRole);
        return a.toString() < b.toString();
    }
    if (m_sortColumn == QStringLiteral("peers")) {
        auto [a, b] = raw(QmlPosterModel::NumPeersRole);
        return a.toInt() < b.toInt();
    }
    return false;
}

int QmlTorrentFilterProxy::mapToSource(int proxyRow) const
{
    if (proxyRow < 0 || proxyRow >= rowCount()) return -1;
    return QSortFilterProxyModel::mapToSource(index(proxyRow, 0)).row();
}

int QmlTorrentFilterProxy::mapFromSource(int sourceRow) const
{
    if (!sourceModel() || sourceRow < 0 || sourceRow >= sourceModel()->rowCount())
        return -1;
    QModelIndex src = sourceModel()->index(sourceRow, 0);
    return QSortFilterProxyModel::mapFromSource(src).row();
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
    m_selectedRows.clear();
    if (index >= 0) m_selectedRows << index;
    m_selectedIndex = index;
    emit selectionChanged();
}

void QmlSessionBridge::setSelectedRows(const QList<int> &rows)
{
    m_selectedRows = rows;
    m_selectedIndex = rows.isEmpty() ? -1 : rows.last();
    emit selectionChanged();
}

QList<int> QmlSessionBridge::selectedRows() const { return m_selectedRows; }

void QmlSessionBridge::pauseSelected()
{
    if (m_selectedRows.isEmpty()) {
        if (m_selectedIndex >= 0) m_session->pauseTorrent(m_selectedIndex);
        return;
    }
    for (int r : m_selectedRows) m_session->pauseTorrent(r);
}

void QmlSessionBridge::resumeSelected()
{
    if (m_selectedRows.isEmpty()) {
        if (m_selectedIndex >= 0) m_session->resumeTorrent(m_selectedIndex);
        return;
    }
    for (int r : m_selectedRows) m_session->resumeTorrent(r);
}

void QmlSessionBridge::removeSelected()
{
    QList<int> rows = m_selectedRows.isEmpty()
        ? (m_selectedIndex >= 0 ? QList<int>{m_selectedIndex} : QList<int>{})
        : m_selectedRows;
    if (rows.isEmpty()) return;
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int r : rows) m_session->removeTorrent(r, false);
    m_selectedRows.clear();
    m_selectedIndex = -1;
    emit selectionChanged();
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

bool QmlSessionBridge::selectedPaused() const
{
    return hasSelection() && m_session->torrentAt(m_selectedIndex).paused;
}

void QmlSessionBridge::toggleSelectedPause()
{
    if (!hasSelection()) return;
    if (m_session->torrentAt(m_selectedIndex).paused)
        m_session->resumeTorrent(m_selectedIndex);
    else
        m_session->pauseTorrent(m_selectedIndex);
}

void QmlSessionBridge::smartPaste()
{
    QString clip = QGuiApplication::clipboard()->text().trimmed();
    if (clip.isEmpty()) return;
    if (clip.startsWith(QStringLiteral("magnet:"), Qt::CaseInsensitive)) {
        addMagnetUri(clip);
        return;
    }
    static const QRegularExpression hashRe(QStringLiteral("^[0-9a-fA-F]{40}$"));
    if (hashRe.match(clip).hasMatch()) {
        addMagnetUri(QStringLiteral("magnet:?xt=urn:btih:") + clip);
        return;
    }
    if (clip.endsWith(QStringLiteral(".torrent"), Qt::CaseInsensitive)
        && (clip.startsWith(QStringLiteral("http"), Qt::CaseInsensitive)
            || clip.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive))) {
        addTorrentFile(clip);
    }
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
    QList<int> rows = m_selectedRows.isEmpty()
        ? (m_selectedIndex >= 0 ? QList<int>{m_selectedIndex} : QList<int>{})
        : m_selectedRows;
    if (rows.isEmpty()) return;
    std::sort(rows.begin(), rows.end());
    QList<int> newRows;
    for (int r : rows) {
        if (r > 0 && !newRows.contains(r - 1)) {
            m_session->setTorrentQueuePosition(r, r - 1);
            emit queueMoved(r, r - 1);
            newRows << (r - 1);
        } else {
            newRows << r;
        }
    }
    m_selectedRows = newRows;
    m_selectedIndex = newRows.isEmpty() ? -1 : newRows.last();
    emit queueRefreshNeeded();
    emit selectionChanged();
}

void QmlSessionBridge::queueDownSelected()
{
    QList<int> rows = m_selectedRows.isEmpty()
        ? (m_selectedIndex >= 0 ? QList<int>{m_selectedIndex} : QList<int>{})
        : m_selectedRows;
    if (rows.isEmpty()) return;
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    int lastIdx = m_session->torrentCount() - 1;
    QList<int> newRows;
    for (int r : rows) {
        if (r < lastIdx && !newRows.contains(r + 1)) {
            m_session->setTorrentQueuePosition(r, r + 1);
            emit queueMoved(r, r + 1);
            newRows << (r + 1);
        } else {
            newRows << r;
        }
    }
    m_selectedRows = newRows;
    m_selectedIndex = newRows.isEmpty() ? -1 : newRows.first();
    emit queueRefreshNeeded();
    emit selectionChanged();
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

QString QmlSessionBridge::selectedDownSpeed() const
{
    if (!hasSelection()) return QStringLiteral("—");
    return formatSpeed(m_session->torrentAt(m_selectedIndex).downloadRate);
}

QString QmlSessionBridge::selectedUpSpeed() const
{
    if (!hasSelection()) return QStringLiteral("—");
    return formatSpeed(m_session->torrentAt(m_selectedIndex).uploadRate);
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

QmlThemeBridge::QmlThemeBridge(QObject *parent) : QObject(parent)
{
    QSettings s;
    m_themeName = s.value(QStringLiteral("qmlThemeName"), QStringLiteral("dark")).toString();
    m_anime = s.value(QStringLiteral("qmlAnime"), false).toBool();
}

QString QmlThemeBridge::themeName() const { return m_themeName; }
void QmlThemeBridge::setThemeName(const QString &n)
{
    if (n == m_themeName) return;
    m_themeName = n;
    QSettings().setValue(QStringLiteral("qmlThemeName"), n);
    emit changed();
}

bool QmlThemeBridge::anime() const { return m_anime; }
void QmlThemeBridge::setAnime(bool on)
{
    if (on == m_anime) return;
    m_anime = on;
    QSettings().setValue(QStringLiteral("qmlAnime"), on);
    emit changed();
}
