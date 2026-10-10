// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_MEDIAFEED_H
#define BATORRENT_MEDIAFEED_H

#include <QByteArray>
#include <QFile>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

// The UI side of RemoteSource: answers the child's byte-range requests from a
// local file or the local stream server, so the sandboxed decoder reads
// nothing itself. Every request is the child's word and is checked as such.
class MediaFeed : public QObject
{
    Q_OBJECT
public:
    static constexpr qint32 kMaxRequest = 4 * 1024 * 1024;
    static constexpr int kMaxPending = 16;

    explicit MediaFeed(const QUrl &source, QObject *parent = nullptr);
    ~MediaFeed() override;

    void start();
    qint64 size() const { return m_size; }
    void request(quint32 id, qint64 offset, qint32 length);

signals:
    void ready(qint64 size);
    void failed(const QString &why);
    void data(quint32 id, qint64 offset, const QByteArray &bytes);
    void refused(quint32 id);

private:
    struct Pending { quint32 id; qint64 offset; qint32 length; };

    void startHttp();
    void streamFrom(qint64 offset);
    bool replyStartsAtBuffer();
    void onHttpData();
    void servePending();
    void trimBuffer();

    QUrl m_source;
    qint64 m_size = -1;
    QFile m_file;

    QNetworkAccessManager *m_nam = nullptr;
    QPointer<QNetworkReply> m_reply;
    bool m_replyChecked = false;
    QByteArray m_buf;          // bytes [m_bufStart, m_bufStart + m_buf.size())
    qint64 m_bufStart = 0;
    QList<Pending> m_pending;
};

#endif
