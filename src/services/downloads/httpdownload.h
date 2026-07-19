// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SERVICES_DOWNLOADS_HTTPDOWNLOAD_H
#define SERVICES_DOWNLOADS_HTTPDOWNLOAD_H

// One direct-HTTP(S) download: probe → segmented (multi-connection Range GET)
// into a preallocated ".part" → integrity-check → rename to the final name.
// Pause/resume/cancel; resumable across restarts via the per-segment received
// offsets (persisted by HttpDownloadManager). Network-driven code is thin; the
// segment math lives in the unit-tested rangeplan.h.

#include "services/downloads/rangeplan.h"

#include <QObject>
#include <QString>
#include <QUrl>
#include <QFile>
#include <QNetworkAccessManager>
#include <QElapsedTimer>
#include <QVector>
#include <QVariantMap>

class QNetworkReply;
class QTimer;

class HttpDownload : public QObject
{
    Q_OBJECT
public:
    enum class State { Queued, Probing, Downloading, Paused, Completed, Failed };
    Q_ENUM(State)

    // `id` is a stable key (used for the .part name and the sidecar file). If
    // savePath/fileName are empty they're filled from the URL / Content-Disposition.
    explicit HttpDownload(const QString &id, const QUrl &url,
                          const QString &saveDir, const QString &fileName = QString(),
                          QObject *parent = nullptr);

    QString id() const { return m_id; }
    QUrl url() const { return m_url; }
    QString fileName() const { return m_fileName; }
    QString finalPath() const;
    QString partPath() const;

    State state() const { return m_state; }
    qint64 totalBytes() const { return m_total; }        // -1 until probed
    qint64 receivedBytes() const;
    double progress() const;                             // 0..1 (0 if size unknown)
    int downloadRate() const { return m_rate; }          // bytes/s, sampled
    QString errorString() const { return m_error; }

    void start();
    void pause();
    void resume();
    void cancel(bool deletePartial);                     // stop for good

    // Resume-state round-trip (HttpDownloadManager persists this as JSON).
    QVariantMap toState() const;
    void restoreState(const QVariantMap &m);             // call before start()

signals:
    void progressed();                                   // bytes/rate moved
    void stateChanged();
    void finished(bool ok);                              // Completed or Failed

private slots:
    void onProbeFinished();
    void onSegmentReadyRead();
    void onSegmentFinished();

private:
    struct Segment {
        bat::ByteRange range;
        qint64 received = 0;                             // bytes written for this seg
        QNetworkReply *reply = nullptr;
        bool done() const { return received >= range.size(); }
    };

    void setState(State s);
    void fail(const QString &why);
    void beginSegments();                                // open .part, spawn replies
    void spawnSegment(int idx);
    void maybeComplete();                                // all segments done?
    void abortReplies();
    static QString fileNameFromReply(QNetworkReply *r, const QUrl &url);

    QString m_id;
    QUrl m_url;
    QString m_saveDir;
    QString m_fileName;
    State m_state = State::Queued;
    qint64 m_total = -1;                                 // unknown until probed
    bool m_acceptRanges = false;
    QString m_error;

    QFile m_file;                                        // the .part (shared, seeked per write)
    QVector<Segment> m_segments;
    int m_conns = 4;

    QNetworkAccessManager m_nam;
    QNetworkReply *m_probe = nullptr;

    // speed sampling
    QTimer *m_sampler = nullptr;
    QElapsedTimer m_clock;
    qint64 m_lastSampledBytes = 0;
    int m_rate = 0;
    qint64 m_lastProgressMs = 0;                         // for stall detection
    bool m_stallRetried = false;
};

#endif
