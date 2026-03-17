#include "torrentmodel.h"
#include "../core/translator.h"
#include "../core/utils.h"
#include <QColor>

TorrentModel::TorrentModel(SessionManager *session, QObject *parent)
    : QAbstractTableModel(parent), m_session(session)
{
    connect(&m_flashTimer, &QTimer::timeout, this, [this]() {
        m_flashingRows.clear();
        m_flashTimer.stop();
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
        case Peers:     return info.numPeers;
        }
    }

    if (role == Qt::UserRole && index.column() == Progress) {
        return info.progress;
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
        case Peers:     return info.numPeers;
        }
    }

    // State for filtering
    if (role == StateFilterRole) {
        return info.stateString;
    }

    // Flash green background for completed downloads
    if (role == Qt::BackgroundRole && m_flashingRows.contains(index.row())) {
        return QColor(0x30, 0x90, 0x50, 50); // translucent green
    }

    // Color coding for state and paused rows
    if (role == Qt::ForegroundRole) {
        if (info.paused)
            return QColor(120, 120, 120); // dim gray for paused
        if (index.column() == State) {
            QString st = info.stateString;
            if (st == tr_("state_downloading")) return QColor(0x40, 0xA0, 0x40); // green
            if (st == tr_("state_seeding"))     return QColor(0x30, 0x90, 0xD0); // blue
            if (st == tr_("state_finished"))    return QColor(0x30, 0x90, 0xD0); // blue
            if (st == tr_("state_checking"))    return QColor(0xD0, 0xA0, 0x30); // yellow
            if (st == tr_("state_metadata"))    return QColor(0xD0, 0xA0, 0x30); // yellow
        }
    }

    return {};
}

QVariant TorrentModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section) {
    case Name:      return tr_("col_name");
    case Size:      return tr_("col_size");
    case Progress:  return tr_("col_progress");
    case DownSpeed: return tr_("col_down");
    case UpSpeed:   return tr_("col_up");
    case State:     return tr_("col_state");
    case Peers:     return tr_("col_peers");
    }
    return {};
}

void TorrentModel::refresh()
{
    beginResetModel();
    endResetModel();
}

void TorrentModel::flashRow(const QString &torrentName)
{
    // Find the row by name
    for (int i = 0; i < m_session->torrentCount(); ++i) {
        TorrentInfo info = m_session->torrentAt(i);
        if (info.name == torrentName) {
            m_flashingRows.insert(i);
            m_flashTimer.start(2000); // flash lasts 2 seconds
            emit dataChanged(index(i, 0), index(i, ColumnCount - 1));
            break;
        }
    }
}
