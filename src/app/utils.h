// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QDir>
#include <QDesktopServices>
#include <QFileInfo>
#include <QProcess>
#include <QUrl>

// Open the system's file manager pointed at `path`, with that path
// highlighted/selected if the platform supports it. If the path itself
// doesn't exist on disk we walk up to the nearest ancestor that does and
// open that — crucial because Windows Explorer treats "/select,<missing>"
// as "no selection at all" and silently opens the user's Documents folder
// instead of the intended save path.
inline void revealInFileManager(const QString &path)
{
    if (path.isEmpty()) return;
    QFileInfo info(path);

    // Walk up until we find something real. This handles the common case
    // where the caller pointed at a partial filename (e.g. before libtorrent
    // has allocated the file, or when the on-disk name has a .!bt suffix
    // the caller didn't account for).
    bool exists = info.exists();
    if (!exists) {
        QDir parent = info.absoluteDir();
        while (!parent.exists() && parent.cdUp()) {}
        if (!parent.exists()) return; // give up — nothing to open
        info = QFileInfo(parent.absolutePath());
    }

    const QString native = QDir::toNativeSeparators(info.absoluteFilePath());

#if defined(Q_OS_WIN)
    if (exists) {
        // explorer.exe /select,"C:\path\to\file" opens the folder with the
        // file highlighted. The comma after /select is required and there
        // must be no space after it.
        QProcess::startDetached("explorer.exe", {QStringLiteral("/select,") + native});
    } else {
        // We fell back to a directory — Explorer opens it directly with no
        // /select so we don't trigger the "missing target → Documents"
        // behavior.
        QProcess::startDetached("explorer.exe", {native});
    }
#elif defined(Q_OS_MACOS)
    if (exists)
        QProcess::startDetached("open", {"-R", info.absoluteFilePath()});
    else
        QProcess::startDetached("open", {info.absoluteFilePath()});
#else
    if (exists) {
        // Most Linux file managers (Nautilus, Dolphin, Nemo, Caja, ...)
        // advertise the FreeDesktop FileManager1 D-Bus interface, which
        // can highlight a specific URI. Try it first, fall back to opening
        // the parent dir.
        QStringList args = {
            "--session",
            "--dest=org.freedesktop.FileManager1",
            "--type=method_call",
            "/org/freedesktop/FileManager1",
            "org.freedesktop.FileManager1.ShowItems",
            QString("array:string:%1")
                .arg(QUrl::fromLocalFile(info.absoluteFilePath()).toString()),
            "string:"
        };
        if (!QProcess::startDetached("dbus-send", args))
            QDesktopServices::openUrl(QUrl::fromLocalFile(info.absolutePath()));
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(info.absoluteFilePath()));
    }
#endif
}

// Resolve the on-disk path for a torrent's root and pass it to
// revealInFileManager. For a single-file torrent that's currently being
// downloaded, the real file name carries the ".!bt" suffix the incomplete-
// file rename adds; just appending info.name to savePath would point at a
// path that doesn't exist yet. We try the bare name first (completed
// torrents), then the suffixed variant (in-progress single-file), then
// fall back to the save folder itself.
inline void revealTorrentRoot(const QString &savePath, const QString &name)
{
    if (savePath.isEmpty() || name.isEmpty()) {
        if (!savePath.isEmpty())
            revealInFileManager(savePath);
        return;
    }
    const QString base = savePath + "/" + name;
    if (QFileInfo::exists(base))
        revealInFileManager(base);
    else if (QFileInfo::exists(base + ".!bt"))
        revealInFileManager(base + ".!bt");
    else
        revealInFileManager(savePath);
}

inline QString formatSize(qint64 bytes)
{
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    if (bytes < 1024LL * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
    return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
}

inline QString formatSpeed(int bps)
{
    if (bps < 1024) return QString::number(bps) + " B/s";
    if (bps < 1024 * 1024) return QString::number(bps / 1024.0, 'f', 1) + " KB/s";
    return QString::number(bps / (1024.0 * 1024.0), 'f', 1) + " MB/s";
}

#endif
