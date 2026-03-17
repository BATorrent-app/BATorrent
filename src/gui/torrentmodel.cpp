#include "torrentmodel.h"
#include "../core/translator.h"
#include "../core/utils.h"

TorrentModel::TorrentModel(SessionManager *session, QObject *parent)
    : QAbstractTableModel(parent), m_session(session)
{
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
