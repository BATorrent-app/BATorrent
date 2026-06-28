// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/security/archiveextractor.h"

#include "services/platform/translator.h"
#include "services/security/archivescan.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>

#include <functional>
#include <memory>

ArchiveExtractor::ArchiveExtractor(QObject *parent)
    : QObject(parent)
{
}

void ArchiveExtractor::extract(const QString &savePath, const QString &torrentName,
                               const QString &priorityPassword, const QString &infoHash)
{
    QDir dir(savePath);

    // Scope strictly to THIS torrent's own content. Scanning the shared save root
    // used to pick up every sibling torrent's archives and extract them all at
    // once — a flood of extractor processes that could freeze the machine.
    // ArchiveScan owns the format + multi-part rules (which volume is the first,
    // which are continuation parts) and is unit-tested separately.
    QStringList archives;
    const QString content = dir.filePath(torrentName);
    const QFileInfo contentInfo(content);
    if (contentInfo.isDir()) {                          // multi-file torrent → its own folder
        QStringList names;
        for (const auto &fi : QDir(content).entryInfoList(QDir::Files))
            names << fi.fileName();
        for (const QString &n : ArchiveScan::archivesToExtract(names))
            archives << QDir(content).filePath(n);
    } else if (contentInfo.isFile()) {                  // single-file torrent that is an archive
        if (!ArchiveScan::archivesToExtract({contentInfo.fileName()}).isEmpty())
            archives << content;
    }

    if (archives.isEmpty()) return;

    if (!infoHash.isEmpty()) emit extractionStarted(infoHash);
    auto allOk = std::make_shared<bool>(true);

    // Serialize: one archive at a time so a multi-archive torrent never opens a
    // swarm of extractor windows. Each archive still retries every password first.
    auto processArchive = std::make_shared<std::function<void(int)>>();
    *processArchive = [this, archives, processArchive, priorityPassword, infoHash, allOk](int ai) {
        if (ai >= archives.size()) {
            if (!infoHash.isEmpty()) emit extractionCompleted(infoHash, *allOk);
            return;
        }
        const QString archive = archives.at(ai);
        QFileInfo fi(archive);
        QString extractDir = fi.absolutePath();

        // A user-typed password (manual extract) is tried first, then no
        // password, then each saved default.
        QStringList attempts;
        if (!priorityPassword.isEmpty()) attempts << priorityPassword;
        attempts << QString();
        attempts << m_passwords;

        auto tryExtract = [this, archive, extractDir, attempts, processArchive, ai, allOk](int attemptIdx) {
            auto self = std::make_shared<std::function<void(int)>>();
            *self = [this, archive, extractDir, attempts, self, processArchive, ai, allOk](int idx) {
                if (idx >= attempts.size()) {
                    qDebug() << "[extract] extraction failed (all passwords tried):" << archive;
                    *allOk = false;
                    emit extractError(tr_("extract_failed").arg(QFileInfo(archive).fileName()));
                    (*processArchive)(ai + 1);          // move on to the next archive
                    return;
                }

                QString password = attempts[idx];
                QStringList args;
                QString program;

#ifdef Q_OS_WIN
                // Prefer 7-Zip (silent CLI, every format). Many Windows users
                // only have WinRAR, though, so fall back to it: UnRAR.exe is a
                // silent CLI for .rar, and WinRAR.exe handles every format.
                QString sevenZip;
                for (const QString &path : {
                         QStringLiteral("C:/Program Files/7-Zip/7z.exe"),
                         QStringLiteral("C:/Program Files (x86)/7-Zip/7z.exe")})
                    if (QFile::exists(path)) { sevenZip = path; break; }
                if (sevenZip.isEmpty())
                    sevenZip = QStandardPaths::findExecutable(QStringLiteral("7z"));

                QString winrarDir;
                for (const QString &dir : {
                         QStringLiteral("C:/Program Files/WinRAR/"),
                         QStringLiteral("C:/Program Files (x86)/WinRAR/")})
                    if (QFile::exists(dir + QStringLiteral("WinRAR.exe"))) { winrarDir = dir; break; }

                const bool isRar = archive.endsWith(QStringLiteral(".rar"), Qt::CaseInsensitive);
                if (!sevenZip.isEmpty()) {
                    program = sevenZip;
                    args << QStringLiteral("x") << archive
                         << QStringLiteral("-o") + extractDir
                         << QStringLiteral("-y") << QStringLiteral("-aoa");
                    if (!password.isEmpty())
                        args << QStringLiteral("-p") + password;
                } else if (isRar && !winrarDir.isEmpty()
                           && QFile::exists(winrarDir + QStringLiteral("UnRAR.exe"))) {
                    program = winrarDir + QStringLiteral("UnRAR.exe");
                    args << QStringLiteral("x") << QStringLiteral("-y") << QStringLiteral("-o+")
                         << (password.isEmpty() ? QStringLiteral("-p-")        // never prompt
                                                : QStringLiteral("-p") + password)
                         << archive << extractDir + QStringLiteral("/");
                } else if (!winrarDir.isEmpty()) {
                    program = winrarDir + QStringLiteral("WinRAR.exe");
                    args << QStringLiteral("x") << QStringLiteral("-y")
                         << QStringLiteral("-ibck") << QStringLiteral("-inul");
                    if (!password.isEmpty())
                        args << QStringLiteral("-p") + password;
                    args << archive << extractDir + QStringLiteral("/");
                } else {
                    program = QStringLiteral("7z.exe");   // last resort: rely on PATH
                    args << QStringLiteral("x") << archive
                         << QStringLiteral("-o") + extractDir
                         << QStringLiteral("-y") << QStringLiteral("-aoa");
                    if (!password.isEmpty())
                        args << QStringLiteral("-p") + password;
                }
#else
                if (archive.endsWith(QStringLiteral(".rar"))) {
                    program = QStringLiteral("unrar");
                    args << QStringLiteral("x") << QStringLiteral("-o+");
                    if (!password.isEmpty())
                        args << QStringLiteral("-p") + password;
                    args << archive << extractDir + QStringLiteral("/");
                } else if (archive.endsWith(QStringLiteral(".zip"))) {
                    program = QStringLiteral("unzip");
                    if (!password.isEmpty())
                        args << QStringLiteral("-P") << password;
                    args << QStringLiteral("-o") << archive
                         << QStringLiteral("-d") << extractDir;
                } else {
                    program = QStringLiteral("7z");
                    args << QStringLiteral("x") << archive
                         << QStringLiteral("-o") + extractDir
                         << QStringLiteral("-y");
                    if (!password.isEmpty())
                        args << QStringLiteral("-p") + password;
                }
#endif

                qDebug() << "[extract] auto-extract attempt" << idx << ":" << program
                         << (password.isEmpty() ? "(no password)" : "(with password)");

                auto *proc = new QProcess(this);
                connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                        this, [this, proc, archive, idx, self, processArchive, ai](int exitCode, QProcess::ExitStatus) {
                    proc->deleteLater();
                    if (exitCode == 0) {
                        qDebug() << "[extract] extraction complete:" << archive;
                        if (m_deleteAfter) {
                            QFile::remove(archive);
                            QFileInfo fi(archive);
                            QDir dir = fi.absoluteDir();
                            QString baseName = fi.completeBaseName();
                            for (const auto &related : dir.entryInfoList(QDir::Files)) {
                                QString name = related.fileName();
                                if (name.startsWith(baseName) &&
                                    (name.endsWith(QStringLiteral(".rar")) ||
                                     name.contains(QRegularExpression(QStringLiteral("\\.(r\\d+|part\\d+\\.rar)$"))))) {
                                    QFile::remove(related.absoluteFilePath());
                                }
                            }
                        }
                        (*processArchive)(ai + 1);     // success → next archive in the queue
                    } else {
                        (*self)(idx + 1);
                    }
                });
                proc->start(program, args);
            };
            (*self)(attemptIdx);
        };
        tryExtract(0);
    };
    (*processArchive)(0);
}
