// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "ipc/sharedsegment.h"
#include <QCryptographicHash>

#ifdef Q_OS_WIN
#  include <windows.h>
#else
#  include <fcntl.h>
#  include <sys/mman.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

#ifdef Q_OS_WIN

std::unique_ptr<SharedSegment> SharedSegment::create(const QString &, qint64 size)
{
    if (size <= 0) return nullptr;
    // Unnamed: the child gets a duplicated handle, so there is no name to find.
    HANDLE h = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                  DWORD(quint64(size) >> 32), DWORD(quint64(size) & 0xFFFFFFFFu), nullptr);
    if (!h) return nullptr;
    void *p = MapViewOfFile(h, FILE_MAP_READ, 0, 0, SIZE_T(size));
    if (!p) { CloseHandle(h); return nullptr; }
    std::unique_ptr<SharedSegment> seg(new SharedSegment);
    seg->m_handle = h;
    seg->m_data = static_cast<uchar *>(p);
    seg->m_size = size;
    return seg;
}

QString SharedSegment::shareWith(qint64 peerPid)
{
    HANDLE peer = OpenProcess(PROCESS_DUP_HANDLE, FALSE, DWORD(peerPid));
    if (!peer) return {};
    HANDLE remote = nullptr;
    const BOOL ok = DuplicateHandle(GetCurrentProcess(), m_handle, peer, &remote,
                                    FILE_MAP_READ | FILE_MAP_WRITE, FALSE, 0);
    CloseHandle(peer);
    return ok ? QString::number(quintptr(remote)) : QString();
}

std::unique_ptr<SharedSegment> SharedSegment::openWritable(const QString &token)
{
    bool ok = false;
    HANDLE h = reinterpret_cast<HANDLE>(quintptr(token.toULongLong(&ok)));
    if (!ok || !h) return nullptr;
    void *p = MapViewOfFile(h, FILE_MAP_WRITE, 0, 0, 0);
    if (!p) { CloseHandle(h); return nullptr; }
    MEMORY_BASIC_INFORMATION info{};
    if (VirtualQuery(p, &info, sizeof info) == 0) { UnmapViewOfFile(p); CloseHandle(h); return nullptr; }
    std::unique_ptr<SharedSegment> seg(new SharedSegment);
    seg->m_handle = h;
    seg->m_data = static_cast<uchar *>(p);
    seg->m_size = qint64(info.RegionSize);
    return seg;
}

void SharedSegment::unpublish() {}

SharedSegment::~SharedSegment()
{
    if (m_data) UnmapViewOfFile(m_data);
    if (m_handle) CloseHandle(m_handle);
}

#else

namespace {

// macOS allows 31 characters for a POSIX shm name.
QString nameFor(const QString &seed)
{
    return QStringLiteral("/batm") + QString::fromLatin1(
        QCryptographicHash::hash(seed.toUtf8(), QCryptographicHash::Sha1).toHex().left(20));
}

}

std::unique_ptr<SharedSegment> SharedSegment::create(const QString &seed, qint64 size)
{
    if (size <= 0) return nullptr;
    const QString name = nameFor(seed);
    const QByteArray n = name.toLatin1();
    const int fd = shm_open(n.constData(), O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd < 0) return nullptr;
    if (ftruncate(fd, off_t(size)) != 0) { close(fd); shm_unlink(n.constData()); return nullptr; }
    void *p = mmap(nullptr, size_t(size), PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    if (p == MAP_FAILED) { shm_unlink(n.constData()); return nullptr; }
    std::unique_ptr<SharedSegment> seg(new SharedSegment);
    seg->m_data = static_cast<uchar *>(p);
    seg->m_size = size;
    seg->m_name = name;
    return seg;
}

QString SharedSegment::shareWith(qint64)
{
    return m_name;
}

std::unique_ptr<SharedSegment> SharedSegment::openWritable(const QString &token)
{
    if (!token.startsWith(QLatin1String("/batm"))) return nullptr;
    const int fd = shm_open(token.toLatin1().constData(), O_RDWR, 0);
    if (fd < 0) return nullptr;
    struct stat st{};
    if (fstat(fd, &st) != 0 || st.st_size <= 0) { close(fd); return nullptr; }
    void *p = mmap(nullptr, size_t(st.st_size), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (p == MAP_FAILED) return nullptr;
    std::unique_ptr<SharedSegment> seg(new SharedSegment);
    seg->m_data = static_cast<uchar *>(p);
    seg->m_size = qint64(st.st_size);
    return seg;
}

void SharedSegment::unpublish()
{
    if (m_name.isEmpty()) return;
    shm_unlink(m_name.toLatin1().constData());
    m_name.clear();
}

SharedSegment::~SharedSegment()
{
    unpublish();
    if (m_data) munmap(m_data, size_t(m_size));
}

#endif
