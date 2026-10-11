// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "ipc/remotesource.h"
#include <QDeadlineTimer>
#include <algorithm>
#include <cstring>

RemoteSource::RemoteSource(qint64 size, int timeoutMs, QObject *parent)
    : QIODevice(parent), m_size(std::max<qint64>(0, size)), m_timeoutMs(timeoutMs)
{
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);
}

qint64 RemoteSource::copyCached(qint64 at, char *data, qint64 maxSize) const
{
    for (const Chunk &c : m_cache) {
        const qint64 end = c.offset + c.bytes.size();
        if (at < c.offset || at >= end) continue;
        const qint64 n = std::min(maxSize, end - at);
        std::memcpy(data, c.bytes.constData() + (at - c.offset), size_t(n));
        return n;
    }
    return -1;
}

bool RemoteSource::pendingCovers(qint64 at) const
{
    for (const Pending &p : m_pending)
        if (at >= p.offset && at < p.offset + p.length) return true;
    return false;
}

qint64 RemoteSource::readData(char *data, qint64 maxSize)
{
    const qint64 at = pos();
    if (maxSize <= 0) return 0;
    if (at >= m_size) return 0;
    const QDeadlineTimer deadline(m_timeoutMs);
    QMutexLocker lock(&m_mutex);
    for (;;) {
        if (m_aborted || m_failed) return -1;
        const qint64 n = copyCached(at, data, std::min(maxSize, m_size - at));
        if (n >= 0) return n;
        if (!pendingCovers(at)) {
            const qint32 length = qint32(std::min<qint64>(std::max<qint64>(maxSize, kReadAhead), m_size - at));
            const Pending p{ m_nextId++, at, length };
            m_pending.push_back(p);
            lock.unlock();
            emit readRequested(p.id, p.offset, p.length);
            lock.relock();
            continue;
        }
        if (!m_arrived.wait(&m_mutex, deadline)) return -1;
    }
}

void RemoteSource::deliver(quint32 id, qint64 offset, const QByteArray &bytes)
{
    QMutexLocker lock(&m_mutex);
    const auto it = std::find_if(m_pending.begin(), m_pending.end(),
                                 [id](const Pending &p) { return p.id == id; });
    // Only answers to a question this device asked, at the place it asked.
    if (it == m_pending.end() || it->offset != offset || bytes.isEmpty()
        || bytes.size() > it->length || offset + bytes.size() > m_size)
        return;
    m_pending.erase(it);
    m_cache.push_back({ offset, bytes });
    m_cacheBytes += bytes.size();
    while (m_cacheBytes > kCacheBytes && m_cache.size() > 1) {
        m_cacheBytes -= m_cache.front().bytes.size();
        m_cache.pop_front();
    }
    m_arrived.wakeAll();
}

void RemoteSource::fail(quint32 id)
{
    QMutexLocker lock(&m_mutex);
    const auto it = std::find_if(m_pending.begin(), m_pending.end(),
                                 [id](const Pending &p) { return p.id == id; });
    if (it == m_pending.end()) return;
    m_pending.erase(it);
    m_failed = true;
    m_arrived.wakeAll();
}

void RemoteSource::abort()
{
    QMutexLocker lock(&m_mutex);
    m_aborted = true;
    m_arrived.wakeAll();
}
