// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QDir>
#include <QDesktopServices>
#include <QFileInfo>
#include <QLocale>
#include <QProcess>
#include <QUrl>

#if defined(Q_OS_WIN)
#include <QThread>
#include <shlobj.h>
#endif

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
        // Reveal via the shell API, NOT `explorer.exe /select,`. The
        // command-line form silently opens the user's Documents (or the last
        // folder) whenever the path contains spaces, unicode, or a comma — the
        // "lands in a random folder" bug. SHOpenFolderAndSelectItems takes a
        // PIDL, so nothing is parsed from a string. (Same approach qBittorrent
        // uses.) Runs on a short-lived thread because it needs its own COM apt.
        const QString target = native;
        auto *thread = QThread::create([target]() {
            if (SUCCEEDED(::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))) {
                if (PIDLIST_ABSOLUTE pidl = ::ILCreateFromPathW(reinterpret_cast<const wchar_t *>(target.utf16()))) {
                    // Split into parent folder + child and select the child, rather
                    // than passing the item itself as pidlFolder with no children.
                    // For a folder that latter form makes Explorer *enter* it (or, on
                    // some builds, open the parent with nothing highlighted) — the
                    // "lands in Downloads, folder not selected" bug. An explicit child
                    // highlights it inside its parent for both files and folders.
                    if (PIDLIST_ABSOLUTE parent = ::ILClone(pidl); parent && ::ILRemoveLastID(parent)) {
                        PCUITEMID_CHILD child = ::ILFindLastID(pidl);
                        ::SHOpenFolderAndSelectItems(parent, 1, &child, 0);
                        ::ILFree(parent);
                    } else {
                        if (parent) ::ILFree(parent);
                        ::SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);
                    }
                    ::ILFree(pidl);
                }
                ::CoUninitialize();
            }
        });
        QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
        thread->start();
    } else {
        // Fell back to an ancestor directory — just open it (no selection).
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

// Global speed unit. 0 = bytes (B/s, KB/s, MB/s), 1 = bits (b/s, Kbps, Mbps).
// Set once at app startup from QSettings; read on every formatSpeed call.
// A static int instead of QSettings query each call keeps the hot path
// allocation-free.
inline int &g_speedUnit() { static int u = 0; return u; }
inline void setSpeedUnit(int unit) { g_speedUnit() = unit; }

inline QString formatSize(qint64 bytes)
{
    const QLocale loc = QLocale::system();
    if (bytes < 1024) return loc.toString(bytes) + " B";
    if (bytes < 1024 * 1024) return loc.toString(bytes / 1024.0, 'f', 1) + " KB";
    if (bytes < 1024LL * 1024 * 1024) return loc.toString(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
    return loc.toString(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
}

inline QString formatSpeed(int bps)
{
    const QLocale loc = QLocale::system();
    if (g_speedUnit() == 1) {
        // Bits-per-second display. Convert bytes-per-second → bits by *8.
        const double bits = bps * 8.0;
        if (bits < 1000)    return loc.toString(int(bits)) + " b/s";
        if (bits < 1000000) return loc.toString(bits / 1000.0, 'f', 1) + " Kbps";
        return loc.toString(bits / 1000000.0, 'f', 1) + " Mbps";
    }
    if (bps < 1024) return loc.toString(bps) + " B/s";
    if (bps < 1024 * 1024) return loc.toString(bps / 1024.0, 'f', 1) + " KB/s";
    return loc.toString(bps / (1024.0 * 1024.0), 'f', 1) + " MB/s";
}

#endif
