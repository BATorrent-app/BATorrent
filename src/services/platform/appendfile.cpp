// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/platform/appendfile.h"

#include <QDir>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <QFile>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef Q_OS_WIN

bool AppendFile::open(const QString &path)
{
    close();
    // FILE_APPEND_DATA without FILE_WRITE_DATA: the system ignores the file
    // position and puts every write at the current end.
    HANDLE h = CreateFileW(reinterpret_cast<const wchar_t *>(QDir::toNativeSeparators(path).utf16()),
                           FILE_APPEND_DATA | SYNCHRONIZE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    m_handle = h;
    return true;
}

void AppendFile::close()
{
    if (m_handle) CloseHandle(m_handle);
    m_handle = nullptr;
}

bool AppendFile::isOpen() const { return m_handle != nullptr; }

bool AppendFile::append(const QByteArray &bytes)
{
    if (!m_handle) return false;
    DWORD written = 0;
    return WriteFile(m_handle, bytes.constData(), DWORD(bytes.size()), &written, nullptr)
           && written == DWORD(bytes.size());
}

qint64 AppendFile::size() const
{
    LARGE_INTEGER s;
    if (!m_handle || !GetFileSizeEx(m_handle, &s)) return -1;
    return s.QuadPart;
}

#else

bool AppendFile::open(const QString &path)
{
    close();
    m_fd = ::open(QFile::encodeName(path).constData(), O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC, 0644);
    return m_fd >= 0;
}

void AppendFile::close()
{
    if (m_fd >= 0) ::close(m_fd);
    m_fd = -1;
}

bool AppendFile::isOpen() const { return m_fd >= 0; }

bool AppendFile::append(const QByteArray &bytes)
{
    if (m_fd < 0) return false;
    return ::write(m_fd, bytes.constData(), size_t(bytes.size())) == bytes.size();
}

qint64 AppendFile::size() const
{
    struct stat st;
    if (m_fd < 0 || fstat(m_fd, &st) != 0) return -1;
    return qint64(st.st_size);
}

#endif
