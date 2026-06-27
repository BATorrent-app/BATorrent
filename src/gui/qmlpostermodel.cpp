// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "qmlposterbridge.h"
#include "../torrent/sessionmanager.h"
#include "../app/metadataresolver.h"
#include "../app/discoveryservice.h"
#include "../app/nameparser.h"
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
#include "../webui/webserver.h"
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
            if (meta.valid && !meta.posterPath.isEmpty()) {
                // Poster files are keyed by info-hash, so "fix cover" overwrites
                // the same path — append the file's mtime so the URL changes and
                // QML's (cache:true) Image actually reloads the new art.
                const qint64 mt = QFileInfo(meta.posterPath).lastModified().toMSecsSinceEpoch();
                return meta.posterPath + QStringLiteral("?v=") + QString::number(mt);
            }
        }
        return QString();
    }
    case MetaTitleRole: {
        const ParsedName pn = NameParser::parse(info.name);
        QString title;
        if (m_resolver && m_resolver->hasCached(hash)) {
            auto meta = m_resolver->cached(hash);
            if (meta.valid) title = meta.title;
        }
        // No resolved cover yet (or the lookup failed): fall back to the parsed
        // title (grid/list tiles were blank on placeholder posters), then to the
        // raw name if parsing strips everything.
        if (title.isEmpty()) title = pn.cleanTitle.trimmed();
        if (title.isEmpty()) title = info.name;
        // For an episode, append SxxExx so several episodes of the same show are
        // distinguishable — they share one resolved cover/title otherwise.
        if (pn.contentType == ContentType::Series && pn.season >= 0 && pn.episode >= 0)
            title += QStringLiteral(" S%1E%2")
                         .arg(pn.season, 2, 10, QLatin1Char('0'))
                         .arg(pn.episode, 2, 10, QLatin1Char('0'));
        return title;
    }
    case StateStringRole: return info.stateString;
    case StateDetailRole: return info.stateDetail;
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
        {StateDetailRole, "stateDetail"},
        {DownSpeedRole,   "downSpeed"},
        {UpSpeedRole,     "upSpeed"},
        {CategoryRole,    "category"},
        {NumPeersRole,    "numPeers"},
        {DownRateRole,    "downRate"},
        {UpRateRole,      "upRate"},
        {SizeRole,        "size"},
        {SizeBytesRole,   "sizeBytes"}
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

void QmlPosterModel::syncCount()
{
    int newCount = m_session->torrentCount();
    if (newCount > m_lastCount) {
        // appends always land at the end, so an incremental insert is correct
        beginInsertRows(QModelIndex(), m_lastCount, newCount - 1);
        m_lastCount = newCount;
        endInsertRows();
    } else if (newCount < m_lastCount) {
        // Safety net only: removals are normally handled by removeRow() with the
        // exact index. If we somehow get here desynced, reset to stay correct.
        beginResetModel();
        m_lastCount = newCount;
        endResetModel();
    }
}

void QmlPosterModel::emitRows(bool fullRoles)
{
    if (m_lastCount <= 0) return;
    // The per-second tick only changes live stats; re-reading the immutable
    // roles (poster/name/size) every tick re-ran the cover's layer+MultiEffect
    // pipeline and made the proxy re-sort live. Send only the volatile roles on
    // the tick; full edits (rename/category/restore) use fullRoles.
    static const QList<int> volatileRoles = {
        ProgressRole, StateKeyRole, StateStringRole, DownSpeedRole,
        UpSpeedRole, NumPeersRole, DownRateRole, UpRateRole,
        // size resolves once a magnet's metadata arrives — without it the grid
        // (and list) stay stuck at "0 B" until some full refresh happens.
        SizeRole, SizeBytesRole };
    if (fullRoles)
        emit dataChanged(index(0), index(m_lastCount - 1));
    else
        emit dataChanged(index(0), index(m_lastCount - 1), volatileRoles);
}

void QmlPosterModel::refresh()      { syncCount(); emitRows(false); }
void QmlPosterModel::refreshFull()  { syncCount(); emitRows(true); }

void QmlPosterModel::removeRow(int index)
{
    if (index < 0 || index >= m_lastCount) { syncCount(); return; }
    beginRemoveRows(QModelIndex(), index, index);
    --m_lastCount;
    endRemoveRows();
}

void QmlPosterModel::posterResolved(const QString &hash)
{
    for (int row = 0; row < m_lastCount; ++row) {
        if (m_session->torrentHashAt(row) == hash) {
            emit dataChanged(index(row), index(row), {PosterPathRole, MetaTitleRole});
            return;
        }
    }
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

