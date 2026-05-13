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
// highlighted/selected if the platform supports it. Falls back to opening
// the parent folder when selection isn't supported.
inline void revealInFileManager(const QString &path)
{
    if (path.isEmpty()) return;
    QFileInfo info(path);
    const QString native = QDir::toNativeSeparators(info.absoluteFilePath());

#if defined(Q_OS_WIN)
    // explorer.exe /select,"C:\path\to\file" opens the folder with the file
    // highlighted. The comma after /select is required and there must be
    // no space after it.
    QProcess::startDetached("explorer.exe", {QStringLiteral("/select,") + native});
#elif defined(Q_OS_MACOS)
    // -R / --reveal opens Finder with the file selected.
    QProcess::startDetached("open", {"-R", info.absoluteFilePath()});
#else
    // Most Linux file managers (Nautilus, Dolphin, Nemo, Caja, ...) advertise
    // the FreeDesktop FileManager1 D-Bus interface, which can highlight a
    // specific URI. Try it first, fall back to opening the parent dir.
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
#endif
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
