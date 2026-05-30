// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef QMLPOSTERBRIDGE_H
#define QMLPOSTERBRIDGE_H

#include <QAbstractListModel>
#include <QColor>
#include <QSortFilterProxyModel>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QVector>

class SessionManager;
class MetadataResolver;

class QmlPosterModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        StateKeyRole,
        InfoHashRole,
        ProgressRole,
        PosterPathRole,
        MetaTitleRole,
        StateStringRole,
        DownSpeedRole,
        UpSpeedRole,
        SizeRole,
        CategoryRole,
        NumPeersRole,
        DownRateRole,
        UpRateRole,
        SizeBytesRole
    };

    explicit QmlPosterModel(SessionManager *session, MetadataResolver *resolver,
                            QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void refresh();
    void moveRow(int from, int to);

private:
    SessionManager *m_session;
    MetadataResolver *m_resolver;
    int m_lastCount = 0;
};

class QmlTorrentFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit QmlTorrentFilterProxy(QObject *parent = nullptr);

    Q_INVOKABLE void setFilterState(const QString &state);
    Q_INVOKABLE void setSearchText(const QString &text);
    Q_INVOKABLE void setSortColumn(const QString &column, bool ascending);
    Q_INVOKABLE void clearSort();
    Q_INVOKABLE int mapToSource(int proxyRow) const;
    Q_INVOKABLE int mapFromSource(int sourceRow) const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &l, const QModelIndex &r) const override;

private:
    QString m_filterState = QStringLiteral("all");
    QString m_searchText;
    QString m_sortColumn;
};

class QmlSessionBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int torrentCount READ torrentCount NOTIFY statsChanged)
    Q_PROPERTY(int activeCount READ activeCount NOTIFY statsChanged)
    Q_PROPERTY(int downloadingCount READ downloadingCount NOTIFY statsChanged)
    Q_PROPERTY(int seedingCount READ seedingCount NOTIFY statsChanged)
    Q_PROPERTY(int pausedCount READ pausedCount NOTIFY statsChanged)
    Q_PROPERTY(int completedCount READ completedCount NOTIFY statsChanged)
    Q_PROPERTY(QString totalDownSpeed READ totalDownSpeed NOTIFY statsChanged)
    Q_PROPERTY(QString totalUpSpeed READ totalUpSpeed NOTIFY statsChanged)
    Q_PROPERTY(QString totalDownloaded READ totalDownloaded NOTIFY statsChanged)
    Q_PROPERTY(QString totalUploaded READ totalUploaded NOTIFY statsChanged)
    Q_PROPERTY(QString globalRatio READ globalRatio NOTIFY statsChanged)
    Q_PROPERTY(QVariantList downloadHistory READ downloadHistory NOTIFY historyChanged)
    Q_PROPERTY(QVariantList uploadHistory READ uploadHistory NOTIFY historyChanged)
    Q_PROPERTY(int historyMaxBytes READ historyMaxBytes NOTIFY historyChanged)

    Q_PROPERTY(QString selectedName READ selectedName NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedSavePath READ selectedSavePath NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedSize READ selectedSize NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedHash READ selectedHash NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedDownloaded READ selectedDownloaded NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedUploaded READ selectedUploaded NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedSpeed READ selectedSpeed NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedDownSpeed READ selectedDownSpeed NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedUpSpeed READ selectedUpSpeed NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedEta READ selectedEta NOTIFY selectionChanged)
    Q_PROPERTY(int selectedSeeds READ selectedSeeds NOTIFY selectionChanged)
    Q_PROPERTY(int selectedPeers READ selectedPeers NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedRatio READ selectedRatio NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedState READ selectedState NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedPoster READ selectedPoster NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedDescription READ selectedDescription NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedMetaTitle READ selectedMetaTitle NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedMetaInfo READ selectedMetaInfo NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedForceStart READ selectedForceStart NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedSuperSeeding READ selectedSuperSeeding NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedCompleted READ selectedCompleted NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedAtFullProgress READ selectedAtFullProgress NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedPaused READ selectedPaused NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList selectedPeerList READ selectedPeerList NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList selectedFiles READ selectedFiles NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList selectedTrackers READ selectedTrackers NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList selectedPieces READ selectedPieces NOTIFY selectionChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)

public:
    explicit QmlSessionBridge(SessionManager *session, MetadataResolver *resolver, QObject *parent = nullptr);

    int torrentCount() const;
    int activeCount() const;
    int downloadingCount() const;
    int seedingCount() const;
    int pausedCount() const;
    int completedCount() const;
    QString totalDownSpeed() const;
    QString totalUpSpeed() const;
    QString totalDownloaded() const;
    QString totalUploaded() const;
    QString globalRatio() const;
    QVariantList downloadHistory() const;
    QVariantList uploadHistory() const;
    int historyMaxBytes() const;

    Q_INVOKABLE void setSelectedIndex(int index);
    Q_INVOKABLE void setSelectedRows(const QList<int> &rows);
    Q_INVOKABLE QList<int> selectedRows() const;
    Q_INVOKABLE void pauseSelected();
    Q_INVOKABLE void resumeSelected();
    Q_INVOKABLE void removeSelected();
    Q_INVOKABLE void removeSelectedWithFiles();
    Q_INVOKABLE void pauseAll();
    Q_INVOKABLE void resumeAll();
    Q_INVOKABLE void addTorrentFile(const QString &filePath);
    Q_INVOKABLE void addMagnetUri(const QString &uri);
    Q_INVOKABLE void copyMagnetLink();
    Q_INVOKABLE void copyInfoHash();
    Q_INVOKABLE void openSaveFolder();
    Q_INVOKABLE QString defaultSavePath() const;
    Q_INVOKABLE void setSelectedForceStart(bool on);
    Q_INVOKABLE void setSelectedSuperSeeding(bool on);
    Q_INVOKABLE void markSelectedCompleted();
    Q_INVOKABLE void unmarkSelectedCompleted();
    Q_INVOKABLE void forceRecheckSelected();
    Q_INVOKABLE void forceReannounceSelected();
    Q_INVOKABLE void queueUpSelected();
    Q_INVOKABLE void queueDownSelected();
    Q_INVOKABLE void toggleSelectedPause();
    Q_INVOKABLE void smartPaste();

    QString selectedName() const;
    QString selectedSavePath() const;
    QString selectedSize() const;
    QString selectedHash() const;
    QString selectedDownloaded() const;
    QString selectedUploaded() const;
    QString selectedSpeed() const;
    QString selectedDownSpeed() const;
    QString selectedUpSpeed() const;
    QString selectedEta() const;
    int selectedSeeds() const;
    int selectedPeers() const;
    QString selectedRatio() const;
    QString selectedState() const;
    QString selectedPoster() const;
    QString selectedDescription() const;
    QString selectedMetaTitle() const;
    QString selectedMetaInfo() const;
    bool selectedForceStart() const;
    bool selectedSuperSeeding() const;
    bool selectedCompleted() const;
    bool selectedAtFullProgress() const;
    bool selectedPaused() const;
    QVariantList selectedPeerList() const;
    QVariantList selectedFiles() const;
    QVariantList selectedTrackers() const;
    QVariantList selectedPieces() const;
    bool hasSelection() const;

    void emitStats();

signals:
    void statsChanged();
    void selectionChanged();
    void historyChanged();
    void queueRefreshNeeded();
    void queueMoved(int from, int to);

private slots:
    void sampleSpeeds();

private:
    SessionManager *m_session;
    MetadataResolver *m_resolver;
    int m_selectedIndex = -1;
    QList<int> m_selectedRows;

    QTimer m_sampleTimer;
    QVector<int> m_downloadHistory;
    QVector<int> m_uploadHistory;
    static constexpr int HistoryMaxPoints = 60;
};

class QmlThemeBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString themeName READ themeName WRITE setThemeName NOTIFY changed)
    Q_PROPERTY(bool anime READ anime WRITE setAnime NOTIFY changed)

public:
    explicit QmlThemeBridge(QObject *parent = nullptr);

    QString themeName() const;
    void setThemeName(const QString &n);
    bool anime() const;
    void setAnime(bool on);

signals:
    void changed();

private:
    QString m_themeName;
    bool m_anime = true;
};

#endif
