#ifndef TORRENTMODEL_H
#define TORRENTMODEL_H

#include <QAbstractTableModel>
#include <QSet>
#include <QTimer>
#include <QMap>
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
    static constexpr int CustomOrderRole = Qt::UserRole + 12;

    // Drag & drop support
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent) override;

    void moveRow(int from, int to);

public slots:
    void refresh();
    void flashRow(const QString &torrentName);

private:
    SessionManager *m_session;
    int m_lastCount = 0;
    QMap<int, int> m_customOrder; // source row -> custom order
    QSet<int> m_flashingRows;
    QTimer m_flashTimer;
};

#endif
