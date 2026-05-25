// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "torrentmodel.h"
#include "../app/translator.h"
#include "../app/utils.h"
#include "thememanager.h"
#include <QColor>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>

namespace {
QString stateKeyFor(const TorrentInfo &info)
{
    if (info.completed) return QStringLiteral("completed");
    if (info.paused) return QStringLiteral("paused");
    const QString &st = info.stateString;
    if (st == tr_("state_downloading")) return QStringLiteral("downloading");
    if (st == tr_("state_seeding"))     return QStringLiteral("seeding");
    if (st == tr_("state_finished"))    return QStringLiteral("finished");
    if (st == tr_("state_completed"))   return QStringLiteral("completed");
    if (st == tr_("state_queued"))      return QStringLiteral("queued");
    if (st == tr_("state_checking"))    return QStringLiteral("downloading");
    if (st == tr_("state_metadata"))    return QStringLiteral("downloading");
    return QStringLiteral("paused");
}

QColor colorForState(const QString &key)
{
    const auto &tm = ThemeManager::instance();
    if (key == QLatin1String("downloading")) return QColor(tm.stateDownloadingColor());
    if (key == QLatin1String("seeding"))     return QColor(tm.stateSeedingColor());
    if (key == QLatin1String("finished"))    return QColor(tm.stateFinishedColor());
    if (key == QLatin1String("completed"))   return QColor(tm.stateCompletedColor());
    if (key == QLatin1String("error"))       return QColor(tm.stateErrorColor());
    return QColor(tm.statePausedColor());
}

QPixmap makeDotIcon(const QColor &c, bool glow)
{
    constexpr int kIconSize = 18;
    constexpr int kDotSize = 6;
    QPixmap pm(kIconSize, kIconSize);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    const QPointF center(kIconSize / 2.0, kIconSize / 2.0);
    if (glow) {
        QColor halo = c;
        halo.setAlphaF(0.30);
        p.setBrush(halo);
        p.setPen(Qt::NoPen);
        p.drawEllipse(center, kDotSize, kDotSize);
    }
    p.setBrush(c);
    p.setPen(Qt::NoPen);
    p.drawEllipse(center, kDotSize / 2.0, kDotSize / 2.0);
    return pm;
}
}

TorrentModel::TorrentModel(SessionManager *session, QObject *parent)
    : QAbstractTableModel(parent), m_session(session)
{
    connect(&m_flashTimer, &QTimer::timeout, this, [this]() {
        m_flashingRows.clear();
        m_flashTimer.stop();
        if (rowCount() > 0)
            emit dataChanged(index(0, 0), index(rowCount() - 1, ColumnCount - 1));
    });
}

int TorrentModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_session->torrentCount();
}

int TorrentModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return ColumnCount;
}

QVariant TorrentModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_session->torrentCount())
        return {};

    TorrentInfo info = m_session->torrentAt(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case Name:      return info.name;
        case Size:      return formatSize(info.totalSize);
        case Progress:  return QString::number(info.progress * 100.0, 'f', 1) + "%";
        case DownSpeed: return formatSpeed(info.downloadRate);
        case UpSpeed:   return formatSpeed(info.uploadRate);
        case State:     return info.stateString;
        case Category:  return info.category;
        case Peers:     return info.numPeers;
        }
    }

    if (role == Qt::UserRole && index.column() == Progress) {
        return info.progress;
    }

    if (role == StateKeyRole) {
        return stateKeyFor(info);
    }

    // Status dot before the name. DecorationRole on column 0 makes Qt
    // render the icon inline before the text — no custom delegate needed.
    if (role == Qt::DecorationRole && index.column() == Name) {
        const QString key = stateKeyFor(info);
        const bool glow = (key == QLatin1String("downloading")
                           || key == QLatin1String("seeding"));
        return makeDotIcon(colorForState(key), glow);
    }

    // Raw values for sorting
    if (role == SortRole) {
        switch (index.column()) {
        case Name:      return info.name.toLower();
        case Size:      return info.totalSize;
        case Progress:  return info.progress;
        case DownSpeed: return info.downloadRate;
        case UpSpeed:   return info.uploadRate;
        case State:     return info.stateString;
        case Category:  return info.category;
        case Peers:     return info.numPeers;
        }
    }

    // Category for filtering
    if (role == CategoryFilterRole) {
        return info.category;
    }

    // State for filtering
    if (role == StateFilterRole) {
        return info.stateString;
    }

    if (role == CustomOrderRole) {
        return m_customOrder.value(index.row(), index.row());
    }

    if (role == InfoHashRole) {
        return m_session->torrentHashAt(index.row());
    }

    // Flash the row in accent-tint when a torrent just finished — picked up
    // by the row background; lasts ~2 s via m_flashTimer.
    if (role == Qt::BackgroundRole && m_flashingRows.contains(index.row())) {
        QColor c(ThemeManager::instance().accentColor());
        c.setAlphaF(0.18f);
        return c;
    }

    // Color coding aligned with the design palette (no greens / blues).
    if (role == Qt::ForegroundRole) {
        const auto &tm = ThemeManager::instance();
        const QString key = stateKeyFor(info);

        if (index.column() == DownSpeed) {
            if (info.downloadRate > 0 && key == QLatin1String("downloading"))
                return QColor(tm.stateDownloadingColor());
            return QColor(tm.dimColor());
        }
        if (index.column() == UpSpeed) {
            if (info.uploadRate > 0)
                return QColor(tm.stateSeedingColor());
            return QColor(tm.dimColor());
        }
        if (index.column() == State) {
            return QColor(colorForState(key));
        }
        if (key == QLatin1String("paused") || key == QLatin1String("queued"))
            return QColor(tm.mutedColor());
    }

    return {};
}

QVariant TorrentModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return {};

    if (role == Qt::DisplayRole) {
        QString label;
        switch (section) {
        case Name:      label = tr_("col_name"); break;
        case Size:      label = tr_("col_size"); break;
        case Progress:  label = tr_("col_progress"); break;
        case DownSpeed: label = tr_("col_down"); break;
        case UpSpeed:   label = tr_("col_up"); break;
        case State:     label = tr_("col_state"); break;
        case Category:  label = tr_("col_category"); break;
        case Peers:     label = tr_("col_peers"); break;
        default: return {};
        }
        return label.toUpper();
    }

    if (role == Qt::FontRole) {
        QFont f;
        f.setPointSize(9);
        f.setWeight(QFont::Black);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
        return f;
    }

    if (role == Qt::ForegroundRole)
        return QColor(ThemeManager::instance().mutedColor());

    return {};
}

void TorrentModel::refresh()
{
    int newCount = m_session->torrentCount();
    if (newCount != m_lastCount) {
        // Row count changed — must reset
        beginResetModel();
        m_lastCount = newCount;
        endResetModel();
    } else if (newCount > 0) {
        // Just data changed — preserves selection
        emit dataChanged(index(0, 0), index(newCount - 1, ColumnCount - 1));
    }
}

void TorrentModel::flashRow(const QString &infoHash)
{
    if (infoHash.isEmpty()) return;
    for (int i = 0; i < m_session->torrentCount(); ++i) {
        if (m_session->torrentHashAt(i) == infoHash) {
            m_flashingRows.insert(i);
            m_flashTimer.start(2000);
            emit dataChanged(index(i, 0), index(i, ColumnCount - 1));
            break;
        }
    }
}

Qt::ItemFlags TorrentModel::flags(const QModelIndex &idx) const
{
    Qt::ItemFlags f = QAbstractTableModel::flags(idx);
    if (idx.isValid())
        f |= Qt::ItemIsDragEnabled;
    f |= Qt::ItemIsDropEnabled;
    return f;
}

Qt::DropActions TorrentModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

QStringList TorrentModel::mimeTypes() const
{
    return {"application/x-batorrent-row"};
}

QMimeData *TorrentModel::mimeData(const QModelIndexList &indexes) const
{
    auto *mime = new QMimeData;
    if (!indexes.isEmpty()) {
        QByteArray data;
        data.setNum(indexes.first().row());
        mime->setData("application/x-batorrent-row", data);
    }
    return mime;
}

bool TorrentModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                                 int row, int /*column*/, const QModelIndex &parent)
{
    if (action != Qt::MoveAction)
        return false;

    QByteArray encoded = data->data("application/x-batorrent-row");
    int from = encoded.toInt();
    int to = parent.isValid() ? parent.row() : row;
    if (to < 0) to = m_session->torrentCount() - 1;

    if (from == to || from < 0 || to < 0)
        return false;

    moveRow(from, to);
    return true;
}

void TorrentModel::moveRow(int from, int to)
{
    int count = m_session->torrentCount();
    if (from < 0 || from >= count || to < 0 || to >= count || from == to)
        return;

    // Tell the session to actually shift the handle in its vector. This also
    // affects every other place that uses indices (queue logic, save-resume
    // ordering, the WebUI hash lookup, etc.) so the visible order and the
    // underlying state stay in sync.
    beginResetModel();
    m_session->setTorrentQueuePosition(from, to);
    endResetModel();
}
