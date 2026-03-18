#include "torrentmodel.h"
#include "../app/translator.h"
#include "../app/utils.h"
#include <QColor>
#include <QMimeData>

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

    // Custom order for manual sorting
    if (role == CustomOrderRole) {
        return m_customOrder.value(index.row(), index.row());
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

void TorrentModel::flashRow(const QString &torrentName)
{
    for (int i = 0; i < m_session->torrentCount(); ++i) {
        TorrentInfo info = m_session->torrentAt(i);
        if (info.name == torrentName) {
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

    // Build current order if empty
    if (m_customOrder.isEmpty()) {
        for (int i = 0; i < count; ++i)
            m_customOrder[i] = i;
    }

    // Swap custom order values
    int orderFrom = m_customOrder.value(from, from);
    int orderTo = m_customOrder.value(to, to);
    m_customOrder[from] = orderTo;
    m_customOrder[to] = orderFrom;

    emit dataChanged(index(0, 0), index(count - 1, ColumnCount - 1));
}
