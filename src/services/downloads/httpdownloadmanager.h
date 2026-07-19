// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SERVICES_DOWNLOADS_HTTPDOWNLOADMANAGER_H
#define SERVICES_DOWNLOADS_HTTPDOWNLOADMANAGER_H

// Owns the set of direct-HTTP(S) downloads: add/pause/resume/remove, and a
// throttled JSON sidecar so an in-progress download survives a restart (resumed
// on the user's click, not automatically — no surprise network traffic on boot).
// The engine decorator (HttpMergeEngine) presents these as extra rows so they
// flow through the whole Downloads UI just like torrents.

#include "services/downloads/httpdownload.h"

#include <QObject>
#include <QVector>
#include <QString>
#include <QUrl>

class QTimer;

namespace bat {
// Pseudo-info-hash the engine decorator uses for an HTTP download row. It MUST
// be a valid filename: the metadata cache keys poster/json files by hash, so a
// ':' (illegal on Windows) can't appear. The id is QUuid Id128 hex, so the
// "httpdl-" prefix keeps the whole thing filesystem-safe.
inline QString httpRowHash(const QString &id) { return QStringLiteral("httpdl-") + id; }
inline bool isHttpRowHash(const QString &h)   { return h.startsWith(QStringLiteral("httpdl-")); }
inline QString httpRowId(const QString &h)    { return h.mid(7); }   // strlen("httpdl-")
}

class HttpDownloadManager : public QObject
{
    Q_OBJECT
public:
    explicit HttpDownloadManager(QObject *parent = nullptr);
    ~HttpDownloadManager() override;

    // Where finished files land when the caller doesn't specify (mirrors the
    // torrent default save path). Set once at startup from the same source.
    void setDefaultDir(const QString &dir) { m_defaultDir = dir; }
    QString defaultDir() const { return m_defaultDir; }

    // Add a direct URL and start it. `saveDir`/`fileName` empty ⇒ default dir /
    // name from the URL or Content-Disposition. Returns the new download's id.
    QString add(const QUrl &url, const QString &saveDir = QString(),
                const QString &fileName = QString());

    int count() const { return int(m_items.size()); }
    HttpDownload *at(int i) const;                 // nullptr if out of range
    HttpDownload *byId(const QString &id) const;

    void pause(const QString &id);
    void resume(const QString &id);
    void remove(const QString &id, bool deleteFiles);

signals:
    void changed();                                // add/remove/progress/state — refresh the list
    void downloadFinished(const QString &id, const QString &name, bool ok);

private:
    void wire(HttpDownload *d);
    void scheduleSave();                           // throttled persist
    void save() const;
    void load();
    QString storePath() const;

    QVector<HttpDownload *> m_items;
    QString m_defaultDir;
    QTimer *m_saveTimer = nullptr;
};

#endif
