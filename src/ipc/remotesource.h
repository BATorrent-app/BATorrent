// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_REMOTESOURCE_H
#define BATORRENT_REMOTESOURCE_H

#include <QIODevice>
#include <QMutex>
#include <QWaitCondition>
#include <deque>

// The media child's only way to read a video: a random-access device whose
// bytes come from the UI over the socket, so the decoder needs no file or
// network access at all. Qt's ffmpeg backend reads it from its own demuxer
// threads, never the main one, which is what lets readData() block until the
// UI answers; deliver()/fail()/abort() run on the main thread.
class RemoteSource : public QIODevice
{
    Q_OBJECT
public:
    static constexpr qint32 kReadAhead = 1024 * 1024;
    static constexpr qint64 kCacheBytes = 32LL * 1024 * 1024;

    RemoteSource(qint64 size, int timeoutMs = 120000, QObject *parent = nullptr);

    bool isSequential() const override { return false; }
    qint64 size() const override { return m_size; }

    void deliver(quint32 id, qint64 offset, const QByteArray &bytes);
    void fail(quint32 id);
    void abort();

signals:
    // Emitted from a reader thread: connect it queued.
    void readRequested(quint32 id, qint64 offset, qint32 length);

protected:
    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *, qint64) override { return -1; }

private:
    struct Chunk { qint64 offset; QByteArray bytes; };
    struct Pending { quint32 id; qint64 offset; qint32 length; };

    qint64 copyCached(qint64 at, char *data, qint64 maxSize) const;
    bool pendingCovers(qint64 at) const;

    const qint64 m_size;
    const int m_timeoutMs;
    mutable QMutex m_mutex;
    QWaitCondition m_arrived;
    std::deque<Chunk> m_cache;
    qint64 m_cacheBytes = 0;
    std::deque<Pending> m_pending;
    quint32 m_nextId = 1;
    bool m_aborted = false;
    bool m_failed = false;
};

#endif
