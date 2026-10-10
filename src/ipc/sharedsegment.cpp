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

QString SharedSegment::nameFor(const QString &seed)
{
    const QString hash = QString::fromLatin1(
        QCryptographicHash::hash(seed.toUtf8(), QCryptographicHash::Sha1).toHex().left(20));
#ifdef Q_OS_WIN
    return QStringLiteral("Local\\batm") + hash;
#else
    return QStringLiteral("/batm") + hash;
#endif
}

#ifdef Q_OS_WIN

std::unique_ptr<SharedSegment> SharedSegment::create(const QString &name, qint64 size)
{
    if (size <= 0) return nullptr;
    HANDLE h = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                  DWORD(quint64(size) >> 32), DWORD(quint64(size) & 0xFFFFFFFFu),
                                  reinterpret_cast<LPCWSTR>(name.utf16()));
    if (!h) return nullptr;
    if (GetLastError() == ERROR_ALREADY_EXISTS) { CloseHandle(h); return nullptr; }
    void *p = MapViewOfFile(h, FILE_MAP_WRITE, 0, 0, SIZE_T(size));
    if (!p) { CloseHandle(h); return nullptr; }
    std::unique_ptr<SharedSegment> seg(new SharedSegment);
    seg->m_handle = h;
    seg->m_data = static_cast<uchar *>(p);
    seg->m_size = size;
    return seg;
}

std::unique_ptr<SharedSegment> SharedSegment::openReadOnly(const QString &name)
{
    HANDLE h = OpenFileMappingW(FILE_MAP_READ, FALSE, reinterpret_cast<LPCWSTR>(name.utf16()));
    if (!h) return nullptr;
    void *p = MapViewOfFile(h, FILE_MAP_READ, 0, 0, 0);
    if (!p) { CloseHandle(h); return nullptr; }
    MEMORY_BASIC_INFORMATION info{};
    if (VirtualQuery(p, &info, sizeof info) == 0) { UnmapViewOfFile(p); CloseHandle(h); return nullptr; }
    std::unique_ptr<SharedSegment> seg(new SharedSegment);
    seg->m_handle = h;
    seg->m_data = static_cast<uchar *>(p);
    seg->m_size = qint64(info.RegionSize);
    return seg;
}

void SharedSegment::release(const QString &) {}   // a mapping dies with its last handle

SharedSegment::~SharedSegment()
{
    if (m_data) UnmapViewOfFile(m_data);
    if (m_handle) CloseHandle(m_handle);
}

#else

std::unique_ptr<SharedSegment> SharedSegment::create(const QString &name, qint64 size)
{
    if (size <= 0) return nullptr;
    const QByteArray n = name.toLatin1();
    const int fd = shm_open(n.constData(), O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd < 0) return nullptr;
    if (ftruncate(fd, off_t(size)) != 0) { close(fd); shm_unlink(n.constData()); return nullptr; }
    void *p = mmap(nullptr, size_t(size), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (p == MAP_FAILED) { shm_unlink(n.constData()); return nullptr; }
    std::unique_ptr<SharedSegment> seg(new SharedSegment);
    seg->m_data = static_cast<uchar *>(p);
    seg->m_size = size;
    return seg;
}

std::unique_ptr<SharedSegment> SharedSegment::openReadOnly(const QString &name)
{
    const QByteArray n = name.toLatin1();
    const int fd = shm_open(n.constData(), O_RDONLY, 0);
    if (fd < 0) return nullptr;
    shm_unlink(n.constData());
    struct stat st{};
    if (fstat(fd, &st) != 0 || st.st_size <= 0) { close(fd); return nullptr; }
    void *p = mmap(nullptr, size_t(st.st_size), PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    if (p == MAP_FAILED) return nullptr;
    std::unique_ptr<SharedSegment> seg(new SharedSegment);
    seg->m_data = static_cast<uchar *>(p);
    seg->m_size = qint64(st.st_size);
    return seg;
}

void SharedSegment::release(const QString &name)
{
    shm_unlink(name.toLatin1().constData());
}

SharedSegment::~SharedSegment()
{
    if (m_data) munmap(m_data, size_t(m_size));
}

#endif
