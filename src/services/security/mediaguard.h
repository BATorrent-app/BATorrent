// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef MEDIAGUARD_H
#define MEDIAGUARD_H

#include <QHash>
#include <QObject>
#include <QSet>
#include "services/security/mediasniff.h"

class IEngine;

// Holds every video file to MediaSniff before anything plays it, and
// quarantines the ones that turn out not to be video: the torrent is paused,
// the file stops downloading and gets a suffix nothing will open. Nothing is
// deleted. Talks only to IEngine, so it works the same in split-engine mode.
class MediaGuard : public QObject
{
    Q_OBJECT
public:
    enum class Gate { Wait, Allow, Block };

    explicit MediaGuard(IEngine *engine, QObject *parent = nullptr);

    Gate admit(const QString &infoHash, int fileIndex);
    bool isQuarantined(const QString &infoHash, int fileIndex) const;
    int quarantinedFileOf(const QString &infoHash) const;   // -1 when the torrent is clean
    // Re-emits quarantined() for a file already held, so a second try is explained too.
    void remind(const QString &infoHash, int fileIndex);

signals:
    void quarantined(const QString &infoHash, int fileIndex,
                     const QString &fileName, const QString &kindKey);
    void lureFound(const QString &infoHash, int fileIndex, const QString &fileName);

private:
    void onTorrentFinished(const QString &name, const QString &infoHash);
    void quarantine(int torrentIndex, int fileIndex, const QString &key, MediaSniff::Kind kind);
    QString leafName(int torrentIndex, int fileIndex) const;

    IEngine *m_engine;
    QSet<QString> m_cleared;
    QHash<QString, QString> m_quarantined;   // key → MediaSniff::kindKey
};

#endif
