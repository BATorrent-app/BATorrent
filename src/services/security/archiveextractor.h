// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef ARCHIVEEXTRACTOR_H
#define ARCHIVEEXTRACTOR_H

#include <QObject>
#include <QString>
#include <QStringList>

// Post-download archive extraction, lifted out of SessionManager. Spawns the
// platform extractor (7-Zip / WinRAR / unrar / unzip) one archive at a time,
// retrying each saved password. ArchiveScan owns the format + multi-part rules;
// this owns the process orchestration. The engine holds an instance and forwards
// its signals to the IEngine extraction signals.
class ArchiveExtractor : public QObject
{
    Q_OBJECT
public:
    explicit ArchiveExtractor(QObject *parent = nullptr);

    void setPasswords(const QStringList &passwords) { m_passwords = passwords; }
    void setDeleteAfter(bool deleteAfter) { m_deleteAfter = deleteAfter; }

    // Extract the archives inside this torrent's own content (savePath/torrentName).
    // priorityPassword (manual extract) is tried before the saved defaults.
    void extract(const QString &savePath, const QString &torrentName,
                 const QString &priorityPassword, const QString &infoHash);

signals:
    void extractionStarted(const QString &infoHash);
    void extractionCompleted(const QString &infoHash, bool success);
    void extractError(const QString &message);

private:
    QStringList m_passwords;
    bool m_deleteAfter = false;
};

#endif // ARCHIVEEXTRACTOR_H
