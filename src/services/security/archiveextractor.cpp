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

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

namespace {

#ifdef Q_OS_WIN
// Ties the extractor to this process and keeps it out of the user's way. A
// 15 GB unpack at full priority, reading and writing the same disk, made the
// whole PC lag; and when the app was closed mid-way the extractor kept going
// on its own. In a job that kills on close it dies with the engine, and at
// background CPU, I/O and memory priority it yields to whatever the user does.
void tameExtractor(qint64 pid)
{
    static HANDLE job = [] {
        HANDLE j = CreateJobObjectW(nullptr, nullptr);
        if (j) {
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION info = {};
            info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            SetInformationJobObject(j, JobObjectExtendedLimitInformation, &info, sizeof(info));
        }
        return j;
    }();
    HANDLE h = OpenProcess(PROCESS_SET_QUOTA | PROCESS_TERMINATE | PROCESS_SET_INFORMATION,
                           FALSE, DWORD(pid));
    if (!h) return;
    if (job) AssignProcessToJobObject(job, h);
    SetPriorityClass(h, BELOW_NORMAL_PRIORITY_CLASS);
    // I/O priority has no documented per-process setter for another process;
    // ntdll's ProcessIoPriority (class 33) is what Task Manager-style tools use.
    using NtSetInfo = LONG(NTAPI *)(HANDLE, ULONG, PVOID, ULONG);
    static const auto ntSet = reinterpret_cast<NtSetInfo>(
        GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtSetInformationProcess"));
    if (ntSet) {
        ULONG ioVeryLow = 0;
        ntSet(h, 33, &ioVeryLow, sizeof(ioVeryLow));
    }
    // SetProcessInformation is Windows 8+ and the build targets 7 (0x0601), so
    // it is looked up; class 0 is ProcessMemoryPriority, 2 is MEMORY_PRIORITY_LOW.
    using SetProcInfo = BOOL(WINAPI *)(HANDLE, int, LPVOID, DWORD);
    static const auto setInfo = reinterpret_cast<SetProcInfo>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetProcessInformation"));
    if (setInfo) {
        ULONG memLow = 2;
        setInfo(h, 0, &memLow, sizeof(memLow));
    }
    CloseHandle(h);
}
#endif

// The last "NN%" the extractor printed: UnRAR's counts the whole archive, 7-Zip's (-bsp1) too.
int lastPercent(const QByteArray &out)
{
    static const QRegularExpression re(QStringLiteral("(\\d{1,3})%"));
    int pct = -1;
    auto it = re.globalMatch(QString::fromLatin1(out));
    while (it.hasNext()) pct = it.next().captured(1).toInt();
    return pct > 100 ? -1 : pct;
}

}

ArchiveExtractor::ArchiveExtractor(QObject *parent)
    : QObject(parent)
{
}

QString ArchiveExtractor::inProgressMarker(const QString &dir)
{
    return QDir(dir).filePath(QStringLiteral(".batorrent-extracting"));
}

void ArchiveExtractor::extract(const QString &savePath, const QString &torrentName,
                               const QString &priorityPassword, const QString &infoHash)
{
    QDir dir(savePath);

    // Scope strictly to THIS torrent's own content. Scanning the shared save root
    // would pick up every sibling torrent's archives and extract them all at
    // once: a flood of extractor processes that could freeze the machine.
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
    const QString marker = inProgressMarker(QFileInfo(archives.first()).absolutePath());
    { QFile m(marker); if (m.open(QIODevice::WriteOnly)) m.close(); }

    // Serialize: one archive at a time so a multi-archive torrent never opens a
    // swarm of extractor windows. Each archive still retries every password first.
    auto processArchive = std::make_shared<std::function<void(int)>>();
    *processArchive = [this, archives, processArchive, priorityPassword, infoHash, allOk, marker](int ai) {
        if (ai >= archives.size()) {
            // A failed unpack keeps the marker: what it left is not a game.
            if (*allOk) QFile::remove(marker);
            if (!infoHash.isEmpty()) emit extractionCompleted(infoHash, *allOk);
            return;
        }
        const QString archive = archives.at(ai);
        QFileInfo fi(archive);
        QString extractDir = fi.absolutePath();

        // A user-typed password (manual extract) is tried first, then the
        // group's own when the name says whose release it is (OnlineFix's
        // -OFME), then no password, the saved ones, and the other known groups'.
        QStringList attempts;
        auto add = [&attempts](const QString &p) { if (!attempts.contains(p)) attempts << p; };
        if (!priorityPassword.isEmpty()) add(priorityPassword);
        for (const QString &p : ArchiveScan::taggedPasswords(fi.fileName())) add(p);
        add(QString());
        for (const QString &p : m_passwords) add(p);
        for (const QString &p : ArchiveScan::knownPasswords(fi.fileName())) add(p);

        const int total = archives.size();
        auto tryExtract = [this, archive, extractDir, attempts, processArchive, ai, allOk, infoHash, total](int attemptIdx) {
            auto self = std::make_shared<std::function<void(int)>>();
            *self = [this, archive, extractDir, attempts, self, processArchive, ai, allOk, infoHash, total](int idx) {
                if (idx >= attempts.size()) {
                    qInfo() << "[extract] extraction failed (all passwords tried):" << archive;
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
                         << QStringLiteral("-y") << QStringLiteral("-aoa")
                         << QStringLiteral("-bsp1");   // progress percentages on stdout
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

                qInfo() << "[extract] auto-extract attempt" << idx << ":" << program
                        << (password.isEmpty() ? "(no password)" : "(with password)") << archive;

                auto *proc = new QProcess(this);
                proc->setProcessChannelMode(QProcess::MergedChannels);
                // Overall progress: this archive's share of the torrent's archives.
                auto lastPct = std::make_shared<int>(-1);
                connect(proc, &QProcess::readyReadStandardOutput, this, [this, proc, infoHash, ai, total, lastPct]() {
                    const int pct = lastPercent(proc->readAllStandardOutput());
                    if (pct < 0 || infoHash.isEmpty()) return;
                    const int overall = (ai * 100 + pct) / total;
                    if (overall == *lastPct) return;
                    *lastPct = overall;
                    emit extractionProgress(infoHash, overall);
                });
#ifdef Q_OS_WIN
                connect(proc, &QProcess::started, this, [proc]() { tameExtractor(proc->processId()); });
#endif
                connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                        this, [this, proc, archive, idx, self, processArchive, ai](int exitCode, QProcess::ExitStatus) {
                    proc->deleteLater();
                    if (exitCode == 0) {
                        qInfo() << "[extract] extraction complete:" << archive;
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
                // No finished() fires when the tool isn't installed (unrar/7z
                // absent on a GUI app's PATH): without this the whole extract
                // state machine stalls forever and extractionCompleted never
                // emits. Treat a failure-to-start as "this attempt failed".
                connect(proc, &QProcess::errorOccurred, this,
                        [proc, idx, self](QProcess::ProcessError e) {
                    if (e != QProcess::FailedToStart) return;   // finished() handles the rest
                    qWarning() << "[extract] tool failed to start — is it installed?";
                    proc->deleteLater();
                    (*self)(idx + 1);
                });
                proc->start(program, args);
            };
            (*self)(attemptIdx);
        };
        tryExtract(0);
    };
    (*processArchive)(0);
}
