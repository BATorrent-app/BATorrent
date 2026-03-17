#ifndef TORRENTMODEL_H
#define TORRENTMODEL_H

#include <QAbstractTableModel>
#include "../core/sessionmanager.h"

class TorrentModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Column {
        Name = 0,
        Size,
        Progress,
        DownSpeed,
        UpSpeed,
        State,
        Peers,
        ColumnCount
    };

    explicit TorrentModel(SessionManager *session, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Raw data roles for sorting
    static constexpr int SortRole = Qt::UserRole + 10;
    static constexpr int StateFilterRole = Qt::UserRole + 11;

public slots:
    void refresh();

private:
    SessionManager *m_session;
};

#endif
