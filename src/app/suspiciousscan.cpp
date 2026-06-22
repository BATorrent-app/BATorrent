// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "suspiciousscan.h"
#include <QFileInfo>
#include <QSet>

namespace {

// Executable / script extensions that have no business inside a movie or album.
const QSet<QString> &execExts()
{
    static const QSet<QString> s = {
        "exe","scr","bat","cmd","com","pif","lnk","js","vbs",
        "hta","wsf","ps1","jar","apk","msi"
    };
    return s;
}

const QSet<QString> &videoExts()
{
    static const QSet<QString> s = {
        "mkv","mp4","avi","mov","wmv","m4v","ts","flv","webm","m2ts","mpg","mpeg"
    };
    return s;
}

const QSet<QString> &audioExts()
{
    static const QSet<QString> s = {
        "mp3","flac","wav","aac","ogg","m4a","wma","opus","alac","aiff"
    };
    return s;
}

// Extensions a double-extension lure masquerades as (what the user thinks they
// are opening). Kept broad: media, images, documents, subtitles, archives.
const QSet<QString> &lureExts()
{
    static const QSet<QString> s = {
        "mkv","mp4","avi","mov","wmv","m4v","ts","flv","webm","mpg","mpeg",
        "mp3","flac","wav","aac","ogg","m4a",
        "jpg","jpeg","png","gif","bmp","webp",
        "pdf","doc","docx","xls","xlsx","ppt","txt","srt","sub","epub","zip","rar"
    };
    return s;
}

QString lower(const QString &s) { return s.toLower(); }

}

namespace SuspiciousScan {

QList<Finding> scan(const QList<ScanFile> &files)
{
    QList<Finding> out;

    qint64 totalBytes = 0, mediaBytes = 0;
    for (const auto &f : files) {
        totalBytes += f.size;
        const QString ext = lower(QFileInfo(f.path).suffix());
        if (videoExts().contains(ext) || audioExts().contains(ext))
            mediaBytes += f.size;
    }
    // "media torrent" = the payload is overwhelmingly video/audio. In a game or
    // app torrent the big .exe/.bin/.pak dominates, so a cutscene .mp4 won't
    // trip the exe-in-media check below — only the always-on double-ext does.
    const bool mediaDominant = totalBytes > 0 && mediaBytes * 10 >= totalBytes * 6;

    for (const auto &f : files) {
        const QString name = lower(QFileInfo(f.path).fileName());
        const QString ext  = lower(QFileInfo(f.path).suffix());

        // Double extension (lure.ext.exec): malware regardless of torrent type.
        const QString base = lower(QFileInfo(f.path).completeBaseName()); // strips final ext
        const QString innerExt = lower(QFileInfo(base).suffix());
        if (execExts().contains(ext) && lureExts().contains(innerExt) && !innerExt.isEmpty()) {
            out.append({ f.path, QStringLiteral("double_ext") });
            continue;
        }

        if (!execExts().contains(ext)) continue;

        if (mediaDominant) {
            static const QStringList codecish = {
                "setup","install","installer","codec","player","activex",
                "crack","keygen","activator","patch"
            };
            bool codec = false;
            for (const auto &kw : codecish)
                if (name.contains(kw)) { codec = true; break; }
            out.append({ f.path, codec ? QStringLiteral("fake_codec")
                                        : QStringLiteral("exe_in_media") });
        }
    }

    return out;
}

}
