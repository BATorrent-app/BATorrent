// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QDir>
#include <QDebug>
#include <QHash>
#include <QDesktopServices>
#include <QFileInfo>
#include <QLocale>
#include <QProcess>
#include <QFile>
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

    // qBittorrent's rule, copied faithfully: a folder is *opened* (you land
    // inside it, seeing only that torrent's files), a single file has its
    // parent opened with the file *selected*. Dropping the user into a shared
    // save folder like Downloads — possibly tens of thousands of files — with
    // nothing useful selected is exactly the failure we're avoiding. The
    // walk-up above can also leave `info` at an ancestor directory, which this
    // treats as the folder case.
    const bool isDir = info.isDir();
    const QString full = info.absoluteFilePath();
    const QString native = QDir::toNativeSeparators(full);

    qInfo().noquote() << "[reveal] in=" << path << "| resolved=" << native
                      << "| isDir=" << isDir << "| existed=" << exists;

#if defined(Q_OS_WIN)
    if (isDir) {
        QProcess::startDetached("explorer.exe", {native});
    } else {
        // Select the file via the shell API, NOT `explorer.exe /select,`. The
        // command-line form silently opens Documents whenever the path has
        // spaces, unicode, or a comma. SHOpenFolderAndSelectItems takes a PIDL,
        // so nothing is parsed from a string. (qBittorrent's openFolderSelect.)
        // Runs on a short-lived thread because it needs its own COM apartment.
        const QString target = native;
        auto *thread = QThread::create([target]() {
            if (SUCCEEDED(::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))) {
                if (PIDLIST_ABSOLUTE pidl = ::ILCreateFromPathW(reinterpret_cast<const wchar_t *>(target.utf16()))) {
                    ::SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);
                    ::ILFree(pidl);
                }
                ::CoUninitialize();
            }
        });
        QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
        thread->start();
    }
#elif defined(Q_OS_MACOS)
    if (isDir)
        QProcess::startDetached("open", {full});
    else
        QProcess::startDetached("open", {"-R", full});
#else
    if (isDir) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(full));
    } else {
        // Most Linux file managers (Nautilus, Dolphin, Nemo, Caja, ...)
        // advertise the FreeDesktop FileManager1 D-Bus interface, which can
        // highlight a specific URI. Try it first, fall back to the parent dir.
        QStringList args = {
            "--session",
            "--dest=org.freedesktop.FileManager1",
            "--type=method_call",
            "/org/freedesktop/FileManager1",
            "org.freedesktop.FileManager1.ShowItems",
            QString("array:string:%1").arg(QUrl::fromLocalFile(full).toString()),
            "string:"
        };
        if (!QProcess::startDetached("dbus-send", args))
            QDesktopServices::openUrl(QUrl::fromLocalFile(info.absolutePath()));
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

// Prefer a content-sniffing player (VLC/mpv/IINA) — they play a still-downloading
// or ".!bt"-suffixed file that the OS default handler would refuse — then fall back.
inline bool launchMediaPlayer(const QString &path)
{
#if defined(Q_OS_MACOS)
    for (const QString &app : {QStringLiteral("VLC"), QStringLiteral("IINA"),
                               QStringLiteral("mpv"), QStringLiteral("QuickTime Player")})
        if (QProcess::startDetached(QStringLiteral("open"), {QStringLiteral("-a"), app, path}))
            return true;
    return QProcess::startDetached(QStringLiteral("open"), {path});
#elif defined(Q_OS_WIN)
    static const QStringList exes = {
        QStringLiteral("C:/Program Files/VideoLAN/VLC/vlc.exe"),
        QStringLiteral("C:/Program Files (x86)/VideoLAN/VLC/vlc.exe"),
        QStringLiteral("C:/Program Files/mpv/mpv.exe")};
    for (const QString &exe : exes)
        if (QFile::exists(exe) && QProcess::startDetached(exe, {path}))
            return true;
    for (const QString &cmd : {QStringLiteral("vlc"), QStringLiteral("mpv")})
        if (QProcess::startDetached(cmd, {path})) return true;
    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
#else
    for (const QString &cmd : {QStringLiteral("vlc"), QStringLiteral("mpv"),
                               QStringLiteral("celluloid")})
        if (QProcess::startDetached(cmd, {path})) return true;
    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
#endif
}

// Torrent site APIs ship titles with raw HTML entities ("1966&ndash;1968",
// "Tom &amp; Jerry"). Entity-only decode — no tag interpretation, so a "<"
// in a release name passes through untouched.
inline QString decodeHtmlEntities(QString s)
{
    if (!s.contains(QLatin1Char('&'))) return s;
    static const QHash<QString, QString> named = {
        {"amp", "&"}, {"lt", "<"}, {"gt", ">"}, {"quot", "\""}, {"apos", "'"},
        {"nbsp", QString(QChar(0x00A0))}, {"ndash", QString(QChar(0x2013))},
        {"mdash", QString(QChar(0x2014))}, {"hellip", QString(QChar(0x2026))},
        {"lsquo", QString(QChar(0x2018))}, {"rsquo", QString(QChar(0x2019))},
        {"ldquo", QString(QChar(0x201C))}, {"rdquo", QString(QChar(0x201D))},
        {"trade", QString(QChar(0x2122))}, {"copy", QString(QChar(0x00A9))},
        {"reg", QString(QChar(0x00AE))}, {"deg", QString(QChar(0x00B0))},
        {"middot", QString(QChar(0x00B7))}, {"bull", QString(QChar(0x2022))},
        {"eacute", QString(QChar(0x00E9))}, {"egrave", QString(QChar(0x00E8))},
        {"agrave", QString(QChar(0x00E0))}, {"ccedil", QString(QChar(0x00E7))},
        {"uuml", QString(QChar(0x00FC))}, {"ouml", QString(QChar(0x00F6))},
        {"auml", QString(QChar(0x00E4))}, {"ntilde", QString(QChar(0x00F1))},
    };
    QString out;
    out.reserve(s.size());
    int i = 0;
    while (i < s.size()) {
        if (s[i] != QLatin1Char('&')) { out += s[i++]; continue; }
        const int semi = s.indexOf(QLatin1Char(';'), i + 1);
        if (semi < 0 || semi - i > 10) { out += s[i++]; continue; }
        const QString body = s.mid(i + 1, semi - i - 1);
        if (body.startsWith(QLatin1Char('#'))) {
            bool ok = false;
            const uint code = body.startsWith(QLatin1String("#x"), Qt::CaseInsensitive)
                                  ? body.mid(2).toUInt(&ok, 16)
                                  : body.mid(1).toUInt(&ok, 10);
            if (ok && code > 0 && code <= 0x10FFFF) {
                const char32_t c = code;
                out += QString::fromUcs4(&c, 1);
                i = semi + 1;
                continue;
            }
        } else if (named.contains(body)) {
            out += named.value(body);
            i = semi + 1;
            continue;
        }
        out += s[i++];
    }
    return out;
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
