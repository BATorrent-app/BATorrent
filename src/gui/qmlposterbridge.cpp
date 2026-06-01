// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "qmlposterbridge.h"
#include "../torrent/sessionmanager.h"
#include "../app/metadataresolver.h"
#include "../app/rssmanager.h"
#include "../app/addonmanager.h"
#include "../app/logger.h"
#include "../app/qrcodegen.h"
#include "../app/utils.h"
#include "../app/translator.h"
#include "../app/geoip.h"
#include "../app/discordrpc.h"
#include "../app/updater.h"
#include "../app/notifier.h"
#include "../app/secretstore.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDateTime>

#include <QNetworkInterface>
#include "thememanager.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QCoreApplication>
#include <QStyleHints>
#include <QPainter>
#include <QSvgRenderer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
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
        // appends always land at the end, so an incremental insert is correct
        beginInsertRows(QModelIndex(), m_lastCount, newCount - 1);
        m_lastCount = newCount;
        endInsertRows();
    } else if (newCount < m_lastCount) {
        // a removal can be at ANY index (not just the tail). We only see the
        // count drop, not which row left, so reset the model to stay in sync —
        // an incremental tail-remove here desynced the view (removed torrent
        // stayed visible).
        beginResetModel();
        m_lastCount = newCount;
        endResetModel();
        return;
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

void QmlTorrentFilterProxy::setCategoryFilter(const QString &category)
{
    if (category == m_categoryFilter) return;
    m_categoryFilter = category;
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

    if (!m_categoryFilter.isEmpty()) {
        if (src->data(idx, QmlPosterModel::CategoryRole).toString() != m_categoryFilter)
            return false;
    }

    if (m_filterState == QStringLiteral("all"))
        return true;

    if (m_filterState == QStringLiteral("active")) {
        // active = actually transferring (down or up), matching activeCount
        return src->data(idx, QmlPosterModel::DownRateRole).toLongLong() > 0
            || src->data(idx, QmlPosterModel::UpRateRole).toLongLong() > 0;
    }
    QString key = src->data(idx, QmlPosterModel::StateKeyRole).toString();
    return key == m_filterState;
}

// Session bridge

QmlSessionBridge::QmlSessionBridge(SessionManager *session, MetadataResolver *resolver, QObject *parent)
    : QObject(parent), m_session(session), m_resolver(resolver)
{
    m_sampleTimer.setInterval(1000);
    connect(&m_sampleTimer, &QTimer::timeout, this, &QmlSessionBridge::sampleSpeeds);
    m_sampleTimer.start();

    m_geoIp = new GeoIpResolver(this);
    connect(m_geoIp, &GeoIpResolver::resolved, this, [this](const QString &, const QString &) {
        emit selectionChanged();
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
        if (info.downloadRate > 0 || info.uploadRate > 0) ++n;   // transferring
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

// resolve the active rows (multi-select, falling back to the focus index)
static QList<int> resolveRows(const QList<int> &rows, int idx)
{
    if (!rows.isEmpty()) return rows;
    return idx >= 0 ? QList<int>{idx} : QList<int>{};
}

void QmlSessionBridge::queueTopSelected()
{
    for (int r : resolveRows(m_selectedRows, m_selectedIndex))
        m_session->setTorrentQueuePosition(r, 0);
    emit queueRefreshNeeded();
    emit selectionChanged();
}

void QmlSessionBridge::queueBottomSelected()
{
    const int last = m_session->torrentCount() - 1;
    for (int r : resolveRows(m_selectedRows, m_selectedIndex))
        m_session->setTorrentQueuePosition(r, last);
    emit queueRefreshNeeded();
    emit selectionChanged();
}

void QmlSessionBridge::stopSeedingSelected()
{
    for (int r : resolveRows(m_selectedRows, m_selectedIndex))
        m_session->stopSeedingTorrent(r);
    emit selectionChanged();
}

void QmlSessionBridge::moveSelectedStorage(const QString &path)
{
    if (path.isEmpty()) return;
    for (int r : resolveRows(m_selectedRows, m_selectedIndex))
        m_session->moveStorage(r, path);
    emit selectionChanged();
}

void QmlSessionBridge::setSelectedDownloadLimit(int kbps)
{
    for (int r : resolveRows(m_selectedRows, m_selectedIndex))
        m_session->setTorrentDownloadLimit(r, kbps);
    emit selectionChanged();
}

void QmlSessionBridge::setSelectedUploadLimit(int kbps)
{
    for (int r : resolveRows(m_selectedRows, m_selectedIndex))
        m_session->setTorrentUploadLimit(r, kbps);
    emit selectionChanged();
}

void QmlSessionBridge::setSelectedSequential(bool on)
{
    for (int r : resolveRows(m_selectedRows, m_selectedIndex))
        m_session->setSequentialDownload(r, on);
    emit selectionChanged();
}

void QmlSessionBridge::setSelectedCategory(const QString &category)
{
    for (int r : resolveRows(m_selectedRows, m_selectedIndex))
        m_session->setTorrentCategory(r, category);
    emit selectionChanged();
}

void QmlSessionBridge::setSelectedTags(const QStringList &tags)
{
    for (int r : resolveRows(m_selectedRows, m_selectedIndex))
        m_session->setTorrentTags(r, tags);
    emit selectionChanged();
}

void QmlSessionBridge::addTrackerToSelected(const QString &url)
{
    if (url.isEmpty()) return;
    for (int r : resolveRows(m_selectedRows, m_selectedIndex))
        m_session->addTracker(r, url);
    emit selectionChanged();
}

void QmlSessionBridge::removeTrackerFromSelected(const QString &url)
{
    if (url.isEmpty() || !hasSelection()) return;
    QStringList keep;
    for (const auto &t : m_session->trackersAt(m_selectedIndex))
        if (t.url != url) keep << t.url;
    m_session->replaceTrackers(m_selectedIndex, keep);
    emit selectionChanged();
}

void QmlSessionBridge::renameSelectedFile(int fileIndex, const QString &newName)
{
    if (!hasSelection() || newName.isEmpty()) return;
    m_session->renameFile(m_selectedIndex, fileIndex, newName);
    emit selectionChanged();
}

void QmlSessionBridge::setSelectedFilePriority(int fileIndex, int priority)
{
    if (!hasSelection()) return;
    m_session->setFilePriority(m_selectedIndex, fileIndex, priority);
    emit selectionChanged();
}

void QmlSessionBridge::copySelectedName()
{
    if (!hasSelection()) return;
    const QString n = m_session->torrentAt(m_selectedIndex).name;
    if (!n.isEmpty()) QGuiApplication::clipboard()->setText(n);
}

void QmlSessionBridge::openSelectedFile()
{
    if (!hasSelection()) return;
    const QString path = m_session->torrentRootPath(m_selectedIndex);
    if (!path.isEmpty()) QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void QmlSessionBridge::importQbittorrent(const QString &savePath)
{
    m_session->importFromQBittorrent(savePath);
    emit queueRefreshNeeded();
}

void QmlSessionBridge::copyToClipboard(const QString &text)
{
    if (!text.isEmpty()) QGuiApplication::clipboard()->setText(text);
}

QVariantMap QmlSessionBridge::statistics() const
{
    QVariantMap m;
    const qint64 down = m_session->globalDownloaded();
    const qint64 up = m_session->globalUploaded();
    const qint64 sDown = m_session->sessionDownloaded();
    const qint64 sUp = m_session->sessionUploaded();
    m["totalDownloaded"] = formatSize(down);
    m["totalUploaded"] = formatSize(up);
    m["totalRatio"] = QString::number(m_session->globalRatio(), 'f', 3);
    m["torrentsAdded"] = m_session->totalTorrentsAdded();
    m["sessionDownloaded"] = formatSize(sDown);
    m["sessionUploaded"] = formatSize(sUp);
    m["sessionRatio"] = QString::number(sDown > 0 ? double(sUp) / double(sDown) : 0.0, 'f', 3);
    QSettings s;
    const qint64 startTime = s.value(QStringLiteral("sessionStartTime"), 0).toLongLong();
    const qint64 uptime = startTime > 0 ? QDateTime::currentSecsSinceEpoch() - startTime : 0;
    const int d = int(uptime / 86400), h = int((uptime % 86400) / 3600), mn = int((uptime % 3600) / 60);
    m["uptime"] = d > 0 ? QString("%1d %2h %3m").arg(d).arg(h).arg(mn)
                        : QString("%1h %2m").arg(h).arg(mn);
    return m;
}

QVariantMap QmlSessionBridge::diagnostics() const
{
    QVariantMap m;
    const int port = m_session->listenPort();
    const auto stats = m_session->detailedStats();
    const bool dht = m_session->dhtEnabled();

    m["listenOk"] = port > 0;
    m["listenText"] = port > 0 ? QStringLiteral("Escutando na porta %1").arg(port)
                               : QStringLiteral("Não está escutando");
    m["dhtOk"] = dht;
    m["dhtText"] = dht ? QStringLiteral("Ativo (%1 nós)").arg(stats.dhtNodes)
                       : QStringLiteral("Desativado");
    m["natOk"] = stats.hasIncomingConnections;
    m["natText"] = stats.hasIncomingConnections ? QStringLiteral("Conexões de entrada OK")
                                                : QStringLiteral("Sem conexões de entrada");
    m["portOk"] = stats.peersCount > 0;
    m["portText"] = stats.peersCount > 0 ? QStringLiteral("%1 peers conectados").arg(stats.peersCount)
                                         : QStringLiteral("Não confirmado");
    return m;
}

QVariantList QmlSessionBridge::recentlyRemoved() const
{
    QVariantList out;
    const auto entries = m_session->recentlyRemoved();
    for (const auto &e : entries) {
        QVariantMap m;
        m["hash"] = e.hash;
        m["name"] = e.name;
        m["size"] = formatSize(e.totalSize);
        m["when"] = QLocale::system().toString(
            QDateTime::fromSecsSinceEpoch(e.removedAt), QLocale::ShortFormat);
        out << m;
    }
    return out;
}

bool QmlSessionBridge::restoreRemoved(const QString &hash)
{
    if (hash.isEmpty()) return false;
    bool ok = m_session->restoreRemoved(hash);
    if (ok) emit queueRefreshNeeded();
    return ok;
}

void QmlSessionBridge::clearRemovedHistory()
{
    m_session->clearRemovedHistory();
}

QString QmlSessionBridge::suggestTorrentOutput(const QString &source) const
{
    QString s = source;
    if (s.startsWith("file://")) s = QUrl(s).toLocalFile();
    if (s.isEmpty()) return {};
    QFileInfo fi(s);
    return fi.absolutePath() + "/" + fi.fileName() + ".torrent";
}

QString QmlSessionBridge::createTorrent(const QVariantMap &opts)
{
    QString source = opts.value("source").toString();
    QString output = opts.value("output").toString();
    if (source.startsWith("file://")) source = QUrl(source).toLocalFile();
    if (output.startsWith("file://")) output = QUrl(output).toLocalFile();
    if (source.isEmpty() || output.isEmpty())
        return QStringLiteral("Informe origem e arquivo de saída.");

    const QString trackerText = opts.value("trackers").toString();
    const int pieceSize = opts.value("pieceSize").toInt();   // bytes, 0 = auto
    const QString comment = opts.value("comment").toString().trimmed();
    const bool priv = opts.value("priv").toBool();
    const bool startSeeding = opts.value("startSeeding").toBool();

    try {
        lt::file_storage fs;
        QFileInfo srcInfo(source);
        const QString parentDir = srcInfo.absolutePath();
        lt::add_files(fs, source.toStdString());
        if (fs.num_files() == 0)
            return QStringLiteral("Nenhum arquivo encontrado na origem.");

        lt::create_torrent ct(fs, pieceSize > 0 ? pieceSize : 0);

        const QStringList trackers = trackerText.split('\n', Qt::SkipEmptyParts);
        for (int i = 0; i < trackers.size(); ++i)
            ct.add_tracker(trackers[i].trimmed().toStdString(), i);

        if (!comment.isEmpty()) ct.set_comment(comment.toStdString().c_str());
        ct.set_creator("BATorrent");
        if (priv) ct.set_priv(true);

        lt::set_piece_hashes(ct, parentDir.toStdString());
        auto entry = ct.generate();

        std::vector<char> buf;
        lt::bencode(std::back_inserter(buf), entry);

        QFile outFile(output);
        if (!outFile.open(QIODevice::WriteOnly))
            return QStringLiteral("Não foi possível gravar o arquivo de saída.");
        outFile.write(buf.data(), static_cast<qsizetype>(buf.size()));
        outFile.close();

        if (startSeeding) {
            m_session->addTorrent(output, parentDir);
            emit queueRefreshNeeded();
        }
        return {};
    } catch (const std::exception &e) {
        return QString::fromStdString(e.what());
    }
}

int QmlSessionBridge::selectedDownloadLimit() const
{
    return hasSelection() ? m_session->torrentDownloadLimit(m_selectedIndex) : 0;
}

int QmlSessionBridge::selectedUploadLimit() const
{
    return hasSelection() ? m_session->torrentUploadLimit(m_selectedIndex) : 0;
}

QString QmlSessionBridge::selectedCategory() const
{
    return hasSelection() ? m_session->torrentAt(m_selectedIndex).category : QString();
}

QStringList QmlSessionBridge::selectedTagList() const
{
    return hasSelection() ? m_session->torrentTags(m_selectedIndex) : QStringList();
}

QStringList QmlSessionBridge::categories() const { return m_session->categories(); }
QStringList QmlSessionBridge::allTags() const { return m_session->allTags(); }

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
        const QString cc = m_geoIp->cachedCountry(p.ip);
        if (cc.isEmpty())
            m_geoIp->resolve(p.ip);
        m["flag"] = cc.isEmpty() ? QString() : countryCodeToFlag(cc);
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

    if (priorities.isEmpty()) {
        m_session->addTorrent(local, dest);
    } else {
        std::vector<int> prios;
        prios.reserve(priorities.size());
        for (const auto &v : priorities) prios.push_back(v.toInt());
        m_session->addTorrentWithPriorities(local, dest, prios);
    }
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
    loadProfiles();
    m_activeProfile = s.value(QStringLiteral("qmlActiveProfile"), 0).toInt();
    if (m_activeProfile < 0 || m_activeProfile >= m_profiles.size())
        m_activeProfile = 0;

    if (auto *hints = QGuiApplication::styleHints()) {
        m_osLight = (hints->colorScheme() == Qt::ColorScheme::Light);
        connect(hints, &QStyleHints::colorSchemeChanged, this, [this](Qt::ColorScheme scheme) {
            const bool light = (scheme == Qt::ColorScheme::Light);
            if (light == m_osLight) return;
            m_osLight = light;
            QGuiApplication::setWindowIcon(trayIcon());   // taskbar/titlebar, live
            emit osSchemeChanged();                        // QML tray re-binds
        });
    }
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

// ---- custom profiles ----

QVariantMap QmlThemeBridge::defaultProfile(const QString &name)
{
    QVariantMap p;
    p["name"]      = name;
    p["bg"]        = QStringLiteral("#0e0e10");
    p["panel"]     = QStringLiteral("#141416");
    p["text"]      = QStringLiteral("#f3f3f4");
    p["primary"]   = QStringLiteral("#e5332b");
    p["secondary"] = QStringLiteral("#d99a2b");
    p["tertiary"]  = QStringLiteral("#3fb950");
    p["image"]     = QString();
    p["opacity"]   = 55;
    return p;
}

void QmlThemeBridge::loadProfiles()
{
    QSettings s;
    const QByteArray json = s.value(QStringLiteral("qmlCustomProfiles")).toByteArray();
    if (!json.isEmpty()) {
        const auto doc = QJsonDocument::fromJson(json);
        if (doc.isArray()) {
            m_profiles.clear();
            for (const auto &v : doc.array()) {
                // backfill any missing keys from the default so a partial/old
                // profile never collapses the palette to empty colors
                QVariantMap p = defaultProfile(QString());
                const QVariantMap stored = v.toObject().toVariantMap();
                for (auto it = stored.cbegin(); it != stored.cend(); ++it)
                    p[it.key()] = it.value();
                m_profiles << p;
            }
        }
    }
    if (m_profiles.isEmpty())
        m_profiles << defaultProfile(QStringLiteral("Custom 1"));
}

void QmlThemeBridge::saveProfiles()
{
    QJsonArray arr;
    for (const auto &v : m_profiles)
        arr.append(QJsonObject::fromVariantMap(v.toMap()));
    QSettings().setValue(QStringLiteral("qmlCustomProfiles"),
                         QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

QVariantList QmlThemeBridge::customProfiles() const { return m_profiles; }

QVariantMap QmlThemeBridge::activeMap() const
{
    if (m_activeProfile >= 0 && m_activeProfile < m_profiles.size())
        return m_profiles.at(m_activeProfile).toMap();
    return defaultProfile(QStringLiteral("Custom 1"));
}

int QmlThemeBridge::activeProfile() const { return m_activeProfile; }
void QmlThemeBridge::setActiveProfile(int i)
{
    if (i == m_activeProfile || i < 0 || i >= m_profiles.size()) return;
    m_activeProfile = i;
    QSettings().setValue(QStringLiteral("qmlActiveProfile"), i);
    emit changed();
}

int QmlThemeBridge::addProfile()
{
    m_profiles << defaultProfile(QStringLiteral("Custom %1").arg(m_profiles.size() + 1));
    saveProfiles();
    emit profilesChanged();
    return m_profiles.size() - 1;
}

void QmlThemeBridge::removeProfile(int i)
{
    if (i < 0 || i >= m_profiles.size() || m_profiles.size() <= 1) return;
    m_profiles.removeAt(i);
    // keep the active selection pointing at the SAME profile: shift down if an
    // earlier one was removed, clamp if the active one (or past-end) was removed.
    if (i < m_activeProfile)
        --m_activeProfile;
    if (m_activeProfile >= m_profiles.size())
        m_activeProfile = m_profiles.size() - 1;
    QSettings().setValue(QStringLiteral("qmlActiveProfile"), m_activeProfile);
    saveProfiles();
    emit profilesChanged();
    emit changed();
}

void QmlThemeBridge::renameProfile(int i, const QString &name)
{
    if (i < 0 || i >= m_profiles.size()) return;
    QVariantMap p = m_profiles.at(i).toMap();
    if (p.value("name").toString() == name) return;
    p["name"] = name;
    m_profiles[i] = p;
    saveProfiles();
    emit profilesChanged();
}

void QmlThemeBridge::setProfileColor(int i, const QString &role, const QString &hex)
{
    if (i < 0 || i >= m_profiles.size()) return;
    QVariantMap p = m_profiles.at(i).toMap();
    if (p.value(role).toString() == hex) return;
    p[role] = hex;
    m_profiles[i] = p;
    saveProfiles();
    emit profilesChanged();
    if (i == m_activeProfile) emit changed();
}

void QmlThemeBridge::setProfileImage(int i, const QString &path)
{
    if (i < 0 || i >= m_profiles.size()) return;
    QVariantMap p = m_profiles.at(i).toMap();
    if (p.value("image").toString() == path) return;
    p["image"] = path;
    m_profiles[i] = p;
    saveProfiles();
    emit profilesChanged();
    if (i == m_activeProfile) emit changed();
}

void QmlThemeBridge::setProfileOpacity(int i, int pct)
{
    if (i < 0 || i >= m_profiles.size()) return;
    QVariantMap p = m_profiles.at(i).toMap();
    if (p.value("opacity").toInt() == pct) return;
    p["opacity"] = pct;
    m_profiles[i] = p;
    saveProfiles();
    emit profilesChanged();
    if (i == m_activeProfile) emit changed();
}

QString QmlThemeBridge::cBg()        const { return activeMap().value("bg").toString(); }
QString QmlThemeBridge::cPanel()     const { return activeMap().value("panel").toString(); }
QString QmlThemeBridge::cText()      const { return activeMap().value("text").toString(); }
QString QmlThemeBridge::cPrimary()   const { return activeMap().value("primary").toString(); }
QString QmlThemeBridge::cSecondary() const { return activeMap().value("secondary").toString(); }
QString QmlThemeBridge::cTertiary()  const { return activeMap().value("tertiary").toString(); }
QString QmlThemeBridge::cBgImage()   const { return activeMap().value("image").toString(); }
int     QmlThemeBridge::cBgOpacity() const { return activeMap().value("opacity").toInt(); }

bool QmlThemeBridge::osLight() const { return m_osLight; }

QString QmlThemeBridge::appVersion() const { return QCoreApplication::applicationVersion(); }

QString QmlThemeBridge::releaseNotes() const
{
    QFile f(QStringLiteral(":/CHANGELOG.md"));   // the real source of truth
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(f.readAll());
}

QVariantList QmlThemeBridge::libraries() const
{
    QVariantList out;
    auto add = [&](const QString &nm, const QString &v) {
        QVariantMap m; m["nm"] = nm; m["v"] = v; out << m;
    };
    add("Qt", QString::fromLatin1(qVersion()));
    add("libtorrent-rasterbar", QStringLiteral(LIBTORRENT_VERSION));
#ifdef OPENSSL_VERSION_STR
    add("OpenSSL", QStringLiteral(OPENSSL_VERSION_STR));
#endif
    add("Boost", QString::fromLatin1(BOOST_LIB_VERSION).replace('_', '.'));
    return out;
}

QPixmap QmlThemeBridge::renderLogo(bool darkBody, int size, qreal dpr)
{
    if (dpr <= 0) dpr = 1.0;
    QFile f(QStringLiteral(":/images/logo.svg"));
    if (!f.open(QIODevice::ReadOnly))
        return QPixmap();
    QByteArray svg = f.readAll();
    // Body is off-white (#E9E9E8); swap to dark text when the background is
    // light so it doesn't vanish. The red wings (#E72134) stay in both cases.
    if (darkBody)
        svg.replace("#E9E9E8", "#16171a");
    QSvgRenderer renderer(svg);
    const int px = int(size * dpr);
    QPixmap pm(px, px);
    pm.fill(Qt::transparent);
    pm.setDevicePixelRatio(dpr);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    renderer.render(&p, QRectF(0, 0, size, size));   // logical extent (retina fix)
    return pm;
}

QIcon QmlThemeBridge::trayIcon() const
{
    QIcon icon;
    for (int sz : {16, 24, 32, 64, 128, 256})
        icon.addPixmap(renderLogo(m_osLight, sz, 1.0));
    return icon;
}

// RSS bridge

QmlRssBridge::QmlRssBridge(QObject *parent) : QObject(parent)
{
    auto &rss = RssManager::instance();
    connect(&rss, &RssManager::feedAdded, this, [this]() { emit feedsChanged(); });
    connect(&rss, &RssManager::feedUpdated, this, [this]() { emit feedsChanged(); });
    connect(&rss, &RssManager::feedError, this, &QmlRssBridge::errorOccurred);
    connect(&rss, &RssManager::itemAutoDownloaded, this, &QmlRssBridge::autoDownloaded);
    rss.loadFeeds();
}

QVariantList QmlRssBridge::feeds() const
{
    QVariantList out;
    const auto feeds = RssManager::instance().feeds();
    for (int i = 0; i < feeds.size(); ++i) {
        const RssFeed &f = feeds[i];
        QVariantMap m;
        m["index"] = i;
        m["name"] = f.name.isEmpty() ? f.url : f.name;
        m["url"] = f.url;
        m["enabled"] = f.enabled;
        m["autoDownload"] = f.autoDownload;
        m["filterPattern"] = f.filterPattern;
        m["savePath"] = f.savePath;
        m["checkInterval"] = f.checkIntervalMin;
        m["lastChecked"] = f.lastChecked.isValid()
            ? f.lastChecked.toString("yyyy-MM-dd hh:mm") : QString();
        m["count"] = RssManager::instance().itemsForFeed(i).size();
        out << m;
    }
    return out;
}

QVariantList QmlRssBridge::itemsForFeed(int feedIndex) const
{
    QVariantList out;
    const auto items = RssManager::instance().itemsForFeed(feedIndex);
    for (const RssItem &it : items) {
        QVariantMap m;
        m["title"] = it.title;
        m["link"] = it.link;
        m["size"] = it.size > 0 ? formatSize(it.size) : QString();
        m["date"] = it.pubDate.isValid() ? it.pubDate.toString("yyyy-MM-dd hh:mm") : QString();
        m["downloaded"] = it.downloaded;
        out << m;
    }
    return out;
}

void QmlRssBridge::addFeed(const QString &url)
{
    if (url.trimmed().isEmpty()) return;
    RssManager::instance().addFeed(url.trimmed());
    RssManager::instance().saveFeeds();
    emit feedsChanged();
}

void QmlRssBridge::removeFeed(int index)
{
    RssManager::instance().removeFeed(index);
    RssManager::instance().saveFeeds();
    emit feedsChanged();
}

void QmlRssBridge::setFeedEnabled(int index, bool enabled)
{
    RssManager::instance().setFeedEnabled(index, enabled);
    RssManager::instance().saveFeeds();
    emit feedsChanged();
}

void QmlRssBridge::setAutoDownload(int index, bool on)
{
    auto feeds = RssManager::instance().feeds();
    if (index < 0 || index >= feeds.size()) return;
    RssFeed copy = feeds[index];
    copy.autoDownload = on;
    RssManager::instance().updateFeed(index, copy);
    RssManager::instance().saveFeeds();
    emit feedsChanged();
}

void QmlRssBridge::checkAllFeeds()
{
    RssManager::instance().checkAllFeeds();
}

void QmlRssBridge::checkFeed(int index)
{
    RssManager::instance().checkFeed(index);
}

void QmlRssBridge::updateFeedSettings(int index, const QString &filterPattern,
                                      const QString &savePath, int checkInterval,
                                      bool enabled, bool autoDownload)
{
    auto feeds = RssManager::instance().feeds();
    if (index < 0 || index >= feeds.size()) return;
    RssFeed f = feeds[index];
    f.filterPattern = filterPattern;
    f.savePath = savePath.startsWith("file://") ? QUrl(savePath).toLocalFile() : savePath;
    f.checkIntervalMin = qBound(5, checkInterval, 1440);
    f.enabled = enabled;
    f.autoDownload = autoDownload;
    RssManager::instance().updateFeed(index, f);
    RssManager::instance().saveFeeds();
    emit feedsChanged();
}

void QmlRssBridge::downloadItem(int feedIndex, int itemIndex)
{
    RssManager::instance().downloadItem(feedIndex, itemIndex);
}

// ===================== QmlSearchBridge =====================

QmlSearchBridge::QmlSearchBridge(SessionManager *session, QObject *parent)
    : QObject(parent), m_session(session), m_mode("torrent")
{
    auto &mgr = AddonManager::instance();
    connect(&mgr, &AddonManager::catalogResults, this, [this](const QList<CatalogItem> &items) {
        m_catalogCache = items;
        if (m_mode != "catalog") return;
        m_results.clear();
        for (const auto &it : items) {
            QVariantMap m;
            m["name"] = it.name;
            m["sub"] = it.type;
            m["sizeStr"] = it.year > 0 ? QString::number(it.year) : QString();
            m["seeds"] = ""; m["leech"] = ""; m["repacker"] = "";
            m_results << m;
        }
        emit resultsChanged();
    });
    connect(&mgr, &AddonManager::catalogFinished, this, [this]() {
        setSearching(false);
        setStatus(QString("%1 resultados").arg(m_catalogCache.size()));
    });
    connect(&mgr, &AddonManager::streamResults, this, [this](const QList<StreamResult> &streams) {
        m_streamCache = streams;
        if (m_mode != "streams") return;
        m_results.clear();
        for (const auto &s : streams) {
            QVariantMap m;
            m["name"] = s.title;
            m["sub"] = s.addonName;
            m["sizeStr"] = s.size > 0 ? formatSize(s.size) : QString();
            m["seeds"] = ""; m["leech"] = ""; m["repacker"] = "";
            m_results << m;
        }
        emit resultsChanged();
    });
    connect(&mgr, &AddonManager::streamFinished, this, [this]() {
        setSearching(false);
        setStatus(QString("%1 streams").arg(m_streamCache.size()));
    });
    connect(&mgr, &AddonManager::torrentSearchResults, this,
            [this](const QList<TorrentSearchResult> &results) {
        m_torrentCache = results;
        if (m_mode != "torrent" && m_mode != "games") return;
        m_results.clear();
        for (const auto &r : results) {
            QVariantMap m;
            m["name"] = r.name;
            m["sub"] = "";
            m["sizeStr"] = r.size > 0 ? formatSize(r.size) : QString();
            m["seeds"] = QString::number(r.seeders);
            m["leech"] = QString::number(r.leechers);
            m["hasSeeds"] = r.seeders > 0;
            m["repacker"] = (m_mode == "games") ? detectRepacker(r.name) : QString();
            m_results << m;
        }
        emit resultsChanged();
    });
    connect(&mgr, &AddonManager::torrentSearchFinished, this, [this]() {
        setSearching(false);
        setStatus(QString("%1 resultados").arg(m_torrentCache.size()));
    });
    connect(&mgr, &AddonManager::torrentSearchError, this, [this](const QString &err) {
        setSearching(false);
        setStatus(err);
    });

    QSettings s;
    m_savePath = s.value(QStringLiteral("lastSavePath")).toString();
    if (m_savePath.isEmpty() || !QDir(m_savePath).exists())
        m_savePath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
}

QString QmlSearchBridge::detectRepacker(const QString &name)
{
    const QString lower = name.toLower();
    if (lower.contains("fitgirl")) return "FitGirl";
    if (lower.contains("dodi")) return "DODI";
    if (lower.contains("online-fix") || lower.contains("onlinefix")) return "Online-Fix";
    if (lower.contains("elamigos")) return "ElAmigos";
    if (lower.contains("xatab")) return "Xatab";
    if (lower.contains("r.g. mechanics") || lower.contains("rg mechanics")) return "R.G. Mechanics";
    if (lower.contains("gog")) return "GOG";
    if (lower.contains("codex")) return "CODEX";
    if (lower.contains("plaza")) return "PLAZA";
    if (lower.contains("skidrow")) return "SKIDROW";
    return "";
}

QVariantList QmlSearchBridge::sources() const
{
    QVariantList out;
    auto add = [&out](const QString &key, const QString &label) {
        QVariantMap m; m["key"] = key; m["label"] = label; out << m;
    };
    add("stremio", "Stremio (catálogo)");
    auto &mgr = AddonManager::instance();
    if (mgr.torrentSearchEnabled()) {
        add("legacy", "Torrents");
        add("games", "Jogos");
    }
    const auto providers = mgr.searchProviders();
    for (int i = 0; i < providers.size(); ++i)
        if (providers[i].enabled)
            add(QString("provider:%1").arg(i), providers[i].name);
    return out;
}

QVariantList QmlSearchBridge::categories() const
{
    QVariantList out;
    auto add = [&out](int code, const QString &label) {
        QVariantMap m; m["code"] = code; m["label"] = label; out << m;
    };
    add(0, "Todas"); add(100, "Áudio"); add(200, "Vídeo");
    add(300, "Apps"); add(400, "Jogos"); add(500, "Outros");
    return out;
}

QVariantList QmlSearchBridge::results() const { return m_results; }
QString QmlSearchBridge::mode() const { return m_mode; }
bool QmlSearchBridge::inStreams() const { return m_mode == "streams"; }
bool QmlSearchBridge::searching() const { return m_searching; }
QString QmlSearchBridge::statusText() const { return m_status; }

void QmlSearchBridge::setSearching(bool on) { if (m_searching == on) return; m_searching = on; emit searchingChanged(); }
void QmlSearchBridge::setStatus(const QString &s) { if (m_status == s) return; m_status = s; emit statusChanged(); }
void QmlSearchBridge::setMode(const QString &m) { if (m_mode == m) return; m_mode = m; emit modeChanged(); }

void QmlSearchBridge::refreshSources() { emit sourcesChanged(); }

void QmlSearchBridge::search(const QString &sourceKey, const QString &query, int categoryCode)
{
    const QString q = query.trimmed();
    if (q.isEmpty()) return;
    m_lastQuery = q;
    m_results.clear();
    emit resultsChanged();

    auto &mgr = AddonManager::instance();
    if (sourceKey == "games") {
        m_isGameSearch = true;
        setMode("games");
        setSearching(true);
        setStatus("Buscando…");
        mgr.searchTorrents(q, 400);
    } else if (sourceKey.startsWith("provider:")) {
        m_isGameSearch = false;
        setMode("torrent");
        setSearching(true);
        setStatus("Buscando…");
        mgr.searchWithProvider(sourceKey.mid(9).toInt(), q);
    } else if (sourceKey == "legacy") {
        m_isGameSearch = false;
        setMode("torrent");
        setSearching(true);
        setStatus("Buscando…");
        mgr.searchTorrents(q, categoryCode);
    } else {
        if (!mgr.hasCatalogAddon()) { setStatus("Nenhum addon de catálogo instalado."); return; }
        setMode("catalog");
        setSearching(true);
        setStatus("Buscando…");
        mgr.searchCatalog(q);
    }
}

void QmlSearchBridge::activateResult(int index)
{
    auto &mgr = AddonManager::instance();
    if (m_mode == "catalog") {
        if (index < 0 || index >= m_catalogCache.size()) return;
        const auto &it = m_catalogCache[index];
        setMode("streams");
        m_results.clear();
        emit resultsChanged();
        if (!mgr.hasStreamAddon()) { setStatus("Nenhum addon de streams instalado."); return; }
        setSearching(true);
        setStatus(QString("Carregando streams de %1…").arg(it.name));
        mgr.getStreams(it.type, it.id);
    } else if (m_mode == "streams") {
        if (index < 0 || index >= m_streamCache.size()) return;
        const auto &s = m_streamCache[index];
        if (s.magnet.startsWith("magnet:")) {
            m_session->addMagnet(s.magnet, m_savePath);
            setStatus(QString("Adicionado: %1").arg(s.title));
        }
    } else {
        if (index < 0 || index >= m_torrentCache.size()) return;
        const auto &r = m_torrentCache[index];
        m_session->addMagnet(r.magnet, m_savePath);
        setStatus(QString("Adicionado: %1").arg(r.name));
    }
}

void QmlSearchBridge::back()
{
    if (m_mode != "streams") return;
    setMode("catalog");
    m_results.clear();
    for (const auto &it : m_catalogCache) {
        QVariantMap m;
        m["name"] = it.name;
        m["sub"] = it.type;
        m["sizeStr"] = it.year > 0 ? QString::number(it.year) : QString();
        m["seeds"] = ""; m["leech"] = ""; m["repacker"] = "";
        m_results << m;
    }
    emit resultsChanged();
    setStatus(QString("%1 resultados").arg(m_catalogCache.size()));
}

// ===================== QmlAddonBridge =====================

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
    struct S { const char *nm; const char *d; const char *url; };
    static const S items[] = {
        { "Cinemeta", "Catálogos oficiais de filmes e séries", "https://v3-cinemeta.strem.io" },
        { "Torrentio", "Streams de torrent para filmes e séries", "https://torrentio.strem.fun" },
    };
    QVariantList out;
    for (const auto &s : items) {
        QVariantMap m;
        m["name"] = QString::fromUtf8(s.nm);
        m["description"] = QString::fromUtf8(s.d);
        m["url"] = QString::fromUtf8(s.url);
        m["installed"] = isInstalled(QString::fromUtf8(s.url));
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
    const auto list = AddonManager::instance().addons();
    for (const auto &a : list)
        if (a.url == url) return true;
    return false;
}

// ===================== QmlPairingBridge =====================

QString QmlPairingBridge::detectLanIp()
{
    QString preferred, fallback;
    for (const auto &iface : QNetworkInterface::allInterfaces()) {
        const auto flags = iface.flags();
        if (!flags.testFlag(QNetworkInterface::IsUp)) continue;
        if (!flags.testFlag(QNetworkInterface::IsRunning)) continue;
        if (flags.testFlag(QNetworkInterface::IsLoopBack)) continue;
        const QString name = iface.name();
        for (const auto &entry : iface.addressEntries()) {
            if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol) continue;
            const QString ip = entry.ip().toString();
            if (ip.startsWith("169.254.")) continue;
            if (name.startsWith("en") || name.contains("wlan") || name.contains("wlp")) {
                if (preferred.isEmpty()) preferred = ip;   // first primary iface wins, deterministic
            } else if (fallback.isEmpty()) {
                fallback = ip;
            }
        }
    }
    return preferred.isEmpty() ? fallback : preferred;
}

QmlPairingBridge::QmlPairingBridge(QObject *parent) : QObject(parent)
{
    const int port = QSettings().value(QStringLiteral("webui/port"), 8080).toInt();
    const QString ip = detectLanIp();
    if (!ip.isEmpty())
        m_url = QStringLiteral("http://%1:%2/").arg(ip).arg(port);
}

void QmlPairingBridge::copyUrl()
{
    if (!m_url.isEmpty()) QGuiApplication::clipboard()->setText(m_url);
}

void QmlPairingBridge::openBrowser()
{
    if (!m_url.isEmpty()) QDesktopServices::openUrl(QUrl(m_url));
}

QStringList QmlPairingBridge::qrRows() const
{
    QStringList rows;
    if (m_url.isEmpty()) return rows;
    const qrgen::Matrix m = qrgen::encode(m_url);
    if (m.size == 0) return rows;
    for (int y = 0; y < m.size; ++y) {
        QString s;
        s.reserve(m.size);
        for (int x = 0; x < m.size; ++x)
            s += m.at(x, y) ? QLatin1Char('1') : QLatin1Char('0');
        rows << s;
    }
    return rows;
}

// ===================== QmlLogBridge =====================

QmlLogBridge::QmlLogBridge(QObject *parent) : QObject(parent)
{
    m_pollTimer.setInterval(1000);
    connect(&m_pollTimer, &QTimer::timeout, this, &QmlLogBridge::poll);
}

int QmlLogBridge::level() const { return int(Logger::instance().level()); }

void QmlLogBridge::setLevel(int l)
{
    Logger::instance().setLevel(Logger::Level(l));
    m_lastSize = 0;
    poll();
    emit levelChanged();
}

QStringList QmlLogBridge::levelNames() const
{
    // Critical omitted on purpose (matches the old combo's 5 entries).
    return { "Trace", "Debug", "Info", "Warning", "Error" };
}

QString QmlLogBridge::logsDir() const { return Logger::instance().logsDir(); }

void QmlLogBridge::start() { poll(); m_pollTimer.start(); }
void QmlLogBridge::stop() { m_pollTimer.stop(); }

void QmlLogBridge::poll()
{
    QFile f(Logger::instance().currentLogPath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    const qint64 size = f.size();
    if (size == m_lastSize) return;
    if (size < m_lastSize) { m_text.clear(); m_lastSize = 0; }  // rotated/cleared
    f.seek(m_lastSize);
    m_text += QString::fromUtf8(f.readAll());
    m_lastSize = size;
    // cap to ~20000 lines (replaces QPlainTextEdit::maximumBlockCount)
    int nl = m_text.count('\n');
    if (nl > 20000) {
        int drop = m_text.indexOf('\n', 0);
        while (nl-- > 20000 && drop >= 0) drop = m_text.indexOf('\n', drop + 1);
        if (drop > 0) m_text = m_text.mid(drop + 1);
    }
    emit textChanged();
}

void QmlLogBridge::clearLog()
{
    Logger::instance().clear();
    m_text.clear();
    m_lastSize = 0;
    emit textChanged();
}

void QmlLogBridge::openLogsFolder()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(Logger::instance().logsDir()));
}

bool QmlLogBridge::exportLogs(const QString &filePath)
{
    QString path = filePath;
    if (path.startsWith("file://")) path = QUrl(path).toLocalFile();
    QFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    out.write(Logger::instance().readAllLogs().toUtf8());
    out.close();
    return true;
}

QString QmlLogBridge::defaultExportName() const
{
    const QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    return desktop + "/batorrent-logs-"
        + QDateTime::currentDateTime().toString("yyyy-MM-dd-HHmmss") + ".txt";
}

// ===================== QmlSettingsBridge =====================

QmlSettingsBridge::QmlSettingsBridge(SessionManager *session, QObject *parent)
    : QObject(parent), m_session(session) {}

QVariant QmlSettingsBridge::get(const QString &key) const
{
    SessionManager *s = m_session;
    // speed
    if (key == "downloadLimit")       return s->downloadLimit();
    if (key == "uploadLimit")         return s->uploadLimit();
    if (key == "maxActiveDownloads")  return s->maxActiveDownloads();
    if (key == "seedRatioLimit")      return s->seedRatioLimit();
    if (key == "stopAfterDownload")   return s->stopAfterDownload();
    if (key == "maxSeedDays")         return int(s->maxSeedSeconds() / 86400);
    if (key == "schedulerEnabled")    return s->schedulerEnabled();
    if (key == "altDownloadLimit")    return s->altDownloadLimit();
    if (key == "altUploadLimit")      return s->altUploadLimit();
    if (key == "scheduleFromHour")    return s->scheduleFromHour();
    if (key == "scheduleToHour")      return s->scheduleToHour();
    if (key == "scheduleDays")        return s->scheduleDays();
    // network
    if (key == "listenPort")          return s->listenPort();
    if (key == "maxConnections")      return s->maxConnections();
    if (key == "dhtEnabled")          return s->dhtEnabled();
    if (key == "utpEnabled")          return s->utpEnabled();
    if (key == "encryptionMode")      return s->encryptionMode();
    if (key == "anonymousMode")       return s->anonymousMode();
    if (key == "forceIpv4")           return s->forceIpv4();
    if (key == "ptMode")              return s->ptMode();
    if (key == "blockLeechers")       return s->blockLeecherClients();
    // vpn
    if (key == "outgoingInterface")   return s->outgoingInterface();
    if (key == "killSwitchEnabled")   return s->killSwitchEnabled();
    if (key == "autoResumeOnReconnect") return s->autoResumeOnReconnect();
    // proxy / ip filter
    if (key == "proxyType")           return s->proxyType();
    if (key == "proxyHost")           return s->proxyHost();
    if (key == "proxyPort")           return s->proxyPort();
    if (key == "proxyUser")           return s->proxyUser();
    if (key == "proxyPass")           return s->proxyPass();
    if (key == "ipFilterPath")        return s->ipFilterPath();
    // files / media
    if (key == "tempPath")            return s->tempPath();
    if (key == "contentLayout")       return s->contentLayout();
    if (key == "torrentExportDir")    return s->torrentExportDir();
    if (key == "extractPasswords")    return s->extractPasswords().join(QStringLiteral("; "));
    if (key == "autoExtract")         return s->autoExtract();
    if (key == "autoExtractDelete")   return s->autoExtractDelete();
    if (key == "runOnComplete")       return s->runOnComplete();
    if (key == "watchedFolder")       return s->watchedFolder();
    if (key == "autoMoveEnabled")     return s->autoMoveEnabled();
    if (key == "autoMovePath")        return s->autoMovePath();
    // telegram (token lives in the keychain; events are a bitmask)
    if (key == "telegramToken")   return SecretStore::instance().get("telegramBotToken");
    {
        int bit = telegramEventBit(key);
        if (bit) {
            QSettings st;
            int mask = st.value("telegramEvents", 0x0F).toInt();   // default: all on
            return bool(mask & bit);
        }
    }
    if (key == "discordEnabled") { QSettings st; return st.value("discordEnabled", true).toBool(); }
    // UI-only prefs + media API keys
    QSettings st;
    return st.value(key);
}

// Maps a per-event toggle key to its TelegramNotifier::Events bit (0 if not one).
int QmlSettingsBridge::telegramEventBit(const QString &key)
{
    if (key == "telegramEvtFinished") return 1 << 0;
    if (key == "telegramEvtKill")     return 1 << 1;
    if (key == "telegramEvtRss")      return 1 << 2;
    if (key == "telegramEvtError")    return 1 << 3;
    return 0;
}

void QmlSettingsBridge::set(const QString &key, const QVariant &v)
{
    // telegram: token → keychain, events → bitmask, chatId → settings; reload after.
    if (key == "telegramToken") {
        SecretStore::instance().set("telegramBotToken", v.toString());
        if (m_telegram) m_telegram->reload();
        emit changed(); return;
    }
    if (int bit = telegramEventBit(key)) {
        QSettings st;
        int mask = st.value("telegramEvents", 0x0F).toInt();
        if (v.toBool()) mask |= bit; else mask &= ~bit;
        st.setValue("telegramEvents", mask);
        if (m_telegram) m_telegram->reload();
        emit changed(); return;
    }
    if (key == "telegramChatId") {
        QSettings st; st.setValue("telegramChatId", v);
        if (m_telegram) m_telegram->reload();
        emit changed(); return;
    }

    SessionManager *s = m_session;
    if (key == "downloadLimit")            s->setDownloadLimit(v.toInt());
    else if (key == "uploadLimit")         s->setUploadLimit(v.toInt());
    else if (key == "maxActiveDownloads")  s->setMaxActiveDownloads(v.toInt());
    else if (key == "seedRatioLimit")      s->setSeedRatioLimit(v.toFloat());
    else if (key == "stopAfterDownload")   s->setStopAfterDownload(v.toBool());
    else if (key == "maxSeedDays")         s->setMaxSeedSeconds(qint64(v.toInt()) * 86400);
    else if (key == "schedulerEnabled")    s->setSchedulerEnabled(v.toBool());
    else if (key == "altDownloadLimit")    s->setAltSpeedLimits(v.toInt(), s->altUploadLimit());
    else if (key == "altUploadLimit")      s->setAltSpeedLimits(s->altDownloadLimit(), v.toInt());
    else if (key == "scheduleFromHour")    s->setScheduleFromHour(v.toInt());
    else if (key == "scheduleToHour")      s->setScheduleToHour(v.toInt());
    else if (key == "scheduleDays")        s->setScheduleDays(v.toInt());
    else if (key == "listenPort")          s->setListenPort(v.toInt());
    else if (key == "maxConnections")      s->setMaxConnections(v.toInt());
    else if (key == "dhtEnabled")          s->setDhtEnabled(v.toBool());
    else if (key == "utpEnabled")          s->setUtpEnabled(v.toBool());
    else if (key == "encryptionMode")      s->setEncryptionMode(v.toInt());
    else if (key == "anonymousMode")       s->setAnonymousMode(v.toBool());
    else if (key == "forceIpv4")           s->setForceIpv4(v.toBool());
    else if (key == "ptMode")              s->setPtMode(v.toBool());
    else if (key == "blockLeechers")       s->setBlockLeecherClients(v.toBool());
    else if (key == "outgoingInterface")   s->setOutgoingInterface(v.toString());
    else if (key == "killSwitchEnabled")   s->setKillSwitchEnabled(v.toBool());
    else if (key == "autoResumeOnReconnect") s->setAutoResumeOnReconnect(v.toBool());
    else if (key == "proxyType")           s->setProxySettings(v.toInt(), s->proxyHost(), s->proxyPort(), s->proxyUser(), s->proxyPass());
    else if (key == "proxyHost")           s->setProxySettings(s->proxyType(), v.toString(), s->proxyPort(), s->proxyUser(), s->proxyPass());
    else if (key == "proxyPort")           s->setProxySettings(s->proxyType(), s->proxyHost(), v.toInt(), s->proxyUser(), s->proxyPass());
    else if (key == "proxyUser")           s->setProxySettings(s->proxyType(), s->proxyHost(), s->proxyPort(), v.toString(), s->proxyPass());
    else if (key == "proxyPass")           s->setProxySettings(s->proxyType(), s->proxyHost(), s->proxyPort(), s->proxyUser(), v.toString());
    else if (key == "ipFilterPath")        { QString p = v.toString(); if (p.isEmpty()) s->clearIpFilter(); else s->loadIpFilter(p); }
    else if (key == "tempPath")            s->setTempPath(v.toString());
    else if (key == "contentLayout")       s->setContentLayout(v.toInt());
    else if (key == "torrentExportDir")    s->setTorrentExportDir(v.toString());
    else if (key == "extractPasswords") {
        QStringList pw;
        const auto parts = v.toString().split(QRegularExpression(QStringLiteral("[;\\n]")), Qt::SkipEmptyParts);
        for (const QString &p : parts) { QString t = p.trimmed(); if (!t.isEmpty()) pw << t; }
        s->setExtractPasswords(pw);
    }
    else if (key == "autoExtract")         s->setAutoExtract(v.toBool());
    else if (key == "autoExtractDelete")   s->setAutoExtractDelete(v.toBool());
    else if (key == "runOnComplete")       s->setRunOnComplete(v.toString());
    else if (key == "watchedFolder")       s->setWatchedFolder(v.toString());
    else if (key == "autoMoveEnabled")     s->setAutoMove(v.toBool(), s->autoMovePath());
    else if (key == "autoMovePath")        s->setAutoMove(s->autoMoveEnabled(), v.toString());
    else { QSettings st; st.setValue(key, v); }
    emit changed();
}

void QmlSettingsBridge::testTelegram()
{
    const QString token = SecretStore::instance().get("telegramBotToken");
    QSettings st;
    const QString chatId = st.value("telegramChatId").toString();
    if (token.isEmpty() || chatId.isEmpty()) {
        emit telegramTestResult(false, tr_("settings_telegram_test_missing"));
        return;
    }
    auto *nam = new QNetworkAccessManager(this);
    QUrl url(QStringLiteral("https://api.telegram.org/bot%1/sendMessage").arg(token));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject body;
    body.insert("chat_id", chatId);
    body.insert("text", QStringLiteral("🦇 BATorrent test — webhook works."));
    auto *reply = nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, nam]() {
        if (reply->error() == QNetworkReply::NoError)
            emit telegramTestResult(true, tr_("settings_telegram_test_ok"));
        else
            emit telegramTestResult(false, QStringLiteral("✗ %1").arg(reply->errorString()));
        reply->deleteLater();
        nam->deleteLater();
    });
}

bool QmlSettingsBridge::excludeFromDefender()
{
#if defined(Q_OS_WIN) && !defined(BAT_STORE_BUILD)
    QSettings s;
    QString path = s.value(QStringLiteral("lastSavePath")).toString();
    if (path.isEmpty() || !QDir(path).exists())
        path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (path.isEmpty()) return false;
    const QString cmd = QStringLiteral("Add-MpPreference -ExclusionPath '%1'").arg(path);
    int ret = QProcess::execute(QStringLiteral("powershell.exe"),
        {QStringLiteral("-Command"),
         QStringLiteral("Start-Process powershell -ArgumentList '-Command','") + cmd
             + QStringLiteral("' -Verb RunAs -Wait")});
    return ret == 0;
#else
    return false;   // Windows-only
#endif
}

bool QmlSettingsBridge::setAsDefaultApp()
{
    const QString exe = QCoreApplication::applicationFilePath();
    bool ok = false;
#ifdef Q_OS_WIN
    const QString nativeExe = QDir::toNativeSeparators(exe);
    QSettings reg("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);
    reg.setValue(".torrent/.", "BATorrent.torrent");
    reg.setValue("BATorrent.torrent/.", "BATorrent Torrent File");
    reg.setValue("BATorrent.torrent/shell/open/command/.", "\"" + nativeExe + "\" \"%1\"");
    reg.setValue("BATorrent.torrent/DefaultIcon/.", nativeExe + ",0");
    reg.setValue("magnet/.", "URL:Magnet Protocol");
    reg.setValue("magnet/URL Protocol", "");
    reg.setValue("magnet/shell/open/command/.", "\"" + nativeExe + "\" \"%1\"");
    reg.setValue("magnet/DefaultIcon/.", nativeExe + ",0");
    reg.sync();
    ok = (reg.status() == QSettings::NoError);
    if (ok)
        QProcess::startDetached("cmd", {"/c", "assoc", ".torrent=BATorrent.torrent"});
#elif defined(Q_OS_LINUX)
    QProcess p;
    p.start("xdg-mime", {"default", "batorrent.desktop", "application/x-bittorrent"});
    p.waitForFinished(3000);
    ok = (p.exitCode() == 0);
    QProcess p2;
    p2.start("xdg-mime", {"default", "batorrent.desktop", "x-scheme-handler/magnet"});
    p2.waitForFinished(3000);
    ok = ok && (p2.exitCode() == 0);
#elif defined(Q_OS_MACOS)
    QProcess p;
    p.start("duti", {"-s", "com.batorrent.app", ".torrent", "all"});
    p.waitForFinished(3000);
    ok = (p.exitCode() == 0);
#endif
    return ok;
}

// --- QmlNotificationBridge ---

void QmlNotificationBridge::onTorrentFinished(const QString &name, const QString &)
{
    emit notify(tr_("dlg_download_complete"), name, 3);   // 3 = success (green)
}

void QmlNotificationBridge::onTorrentError(const QString &message)
{
    emit notify(tr_("dlg_error"), message, 2);
}

void QmlNotificationBridge::onKillSwitchTriggered()
{
    emit notify(tr_("killswitch_title"), tr_("killswitch_triggered"), 1);
}

void QmlNotificationBridge::onRssAutoDownloaded(const QString &feedName, const QString &itemTitle)
{
    emit notify(feedName, itemTitle, 0);
}

// --- DiscordRpcBridge ---

DiscordRpcBridge::DiscordRpcBridge(SessionManager *session, QObject *parent)
    : QObject(parent), m_session(session)
{
    m_rpc = new DiscordRPC(this);
    m_sessionStart = QDateTime::currentSecsSinceEpoch();
    if (QSettings().value("discordRichPresence", true).toBool())
        m_rpc->setClientId(QStringLiteral("1508208411282640956"));

    connect(&m_timer, &QTimer::timeout, this, &DiscordRpcBridge::refresh);
    m_timer.start(15000);
    refresh();
}

void DiscordRpcBridge::refresh()
{
    if (!m_rpc || m_rpc->clientId().isEmpty()) return;
    if (!QSettings().value("discordEnabled", true).toBool()) { m_rpc->clearActivity(); return; }
    // Mirror MainWindow::refreshDiscordPresence: feature the fastest active
    // download; otherwise report seeding count; otherwise idle.
    int seeding = 0, featured = -1, featuredRate = 0;
    for (int i = 0; i < m_session->torrentCount(); ++i) {
        TorrentInfo info = m_session->torrentAt(i);
        if (info.paused || info.completed) continue;
        if (info.progress >= 1.0f) { ++seeding; continue; }
        if (info.downloadRate > featuredRate) {
            featuredRate = info.downloadRate;
            featured = i;
        }
    }
    if (featured >= 0) {
        TorrentInfo info = m_session->torrentAt(featured);
        const QString name = info.name.left(64);
        const QString state = QStringLiteral("%1% · ↓ %2")
            .arg(static_cast<int>(info.progress * 100))
            .arg(formatSpeed(info.downloadRate));
        m_rpc->setActivity(name, state, m_sessionStart);
    } else if (seeding > 0) {
        m_rpc->setActivity(tr_("discord_seeding").arg(seeding),
                           tr_("discord_seeding_state"), m_sessionStart);
    } else {
        m_rpc->setActivity(tr_("discord_idle"), QString(), m_sessionStart);
    }
}

// --- QmlUpdaterBridge ---

QmlUpdaterBridge::QmlUpdaterBridge(QObject *parent)
    : QObject(parent), m_updater(new Updater(this))
{
    connect(m_updater, &Updater::updateAvailable, this,
            [this](const QString &v, const QString &url, const QString &asset) {
        QSettings s;
        if (s.value("skippedUpdateVersion").toString() == v && m_silent)
            return;   // user chose to skip this version on a silent check
        emit updateFound(v, url, asset);
    });
    connect(m_updater, &Updater::noUpdateAvailable, this,
            [this]() { emit noUpdate(m_silent); });
    connect(m_updater, &Updater::downloadProgress, this, [this](qint64 r, qint64 t) {
        emit progress(t > 0 ? static_cast<int>((r * 100) / t) : 0);
    });
    connect(m_updater, &Updater::updateReady, this, &QmlUpdaterBridge::ready);
    connect(m_updater, &Updater::errorOccurred, this, &QmlUpdaterBridge::failed);
}

void QmlUpdaterBridge::check(bool silent)
{
    m_silent = silent;
    m_updater->checkForUpdate();
}

void QmlUpdaterBridge::downloadAndInstall(const QString &url, const QString &assetName)
{
    m_updater->downloadAndInstall(url, assetName);
}

void QmlUpdaterBridge::skipVersion(const QString &version)
{
    QSettings().setValue("skippedUpdateVersion", version);
}
