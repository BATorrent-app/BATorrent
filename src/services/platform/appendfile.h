// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_APPENDFILE_H
#define BATORRENT_APPENDFILE_H

// A file several processes append to at once (the app, the engine and the
// media child all write batorrent.log). Every write lands whole at the end,
// whatever the others did: the system appends it (FILE_APPEND_DATA on
// Windows, O_APPEND elsewhere) instead of each process writing at the end it
// last saw, which overwrote lines. The file can be renamed or deleted while
// open (log rotation, "clear log").

#include <QByteArray>
#include <QString>

class AppendFile
{
public:
    AppendFile() = default;
    ~AppendFile() { close(); }
    AppendFile(const AppendFile &) = delete;
    AppendFile &operator=(const AppendFile &) = delete;

    bool open(const QString &path);
    void close();
    bool isOpen() const;
    // One write call, so one line is never split by another process's.
    bool append(const QByteArray &bytes);
    // The size of the file this handle points at (which may no longer be the
    // one at the path, after a rotation).
    qint64 size() const;

private:
#ifdef Q_OS_WIN
    void *m_handle = nullptr;
#else
    int m_fd = -1;
#endif
};

#endif
