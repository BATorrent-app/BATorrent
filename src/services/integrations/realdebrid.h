// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef REALDEBRID_H
#define REALDEBRID_H

#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>

// Real-Debrid integration: hand a magnet to the user's RD account, let RD
// cache/download it on their servers, then unrestrict the file to a direct
// HTTPS link the built-in player streams (no local seeding, no IP exposure).
//
// Auth is a private API token (real-debrid.com/apitoken), stored via
// SecretStore. One stream job runs at a time; the pipeline is:
//   addMagnet -> selectFiles(video) -> poll info until "downloaded"
//   -> unrestrict the largest video link -> streamReady(url).
class RealDebridClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool authed READ authed NOTIFY accountChanged)
    Q_PROPERTY(QString username READ username NOTIFY accountChanged)
    Q_PROPERTY(QString accountType READ accountType NOTIFY accountChanged)
    Q_PROPERTY(QString expiry READ expiry NOTIFY accountChanged)
    Q_PROPERTY(bool hasToken READ hasToken NOTIFY accountChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(int progress READ progress NOTIFY statusChanged)   // 0..100, stream job
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)   // human stage text
    Q_PROPERTY(QVariantList torrents READ torrents NOTIFY torrentsChanged)

public:
    explicit RealDebridClient(QObject *parent = nullptr);

    bool authed() const { return m_authed; }
    QString username() const { return m_username; }
    QString accountType() const { return m_type; }
    QString expiry() const { return m_expiry; }
    bool hasToken() const { return !m_token.isEmpty(); }
    bool busy() const { return m_busy; }
    int progress() const { return m_progress; }
    QString status() const { return m_status; }
    QVariantList torrents() const { return m_torrents; }

    // Persist + validate a freshly pasted token (empty clears it).
    Q_INVOKABLE void setToken(const QString &token);
    Q_INVOKABLE void clearToken();
    // Re-validate the stored token (GET /user); updates the account properties.
    Q_INVOKABLE void checkAccount();

    // Full pipeline for a magnet -> emits streamReady(url, name) on success.
    Q_INVOKABLE void streamMagnet(const QString &magnet);
    Q_INVOKABLE void cancelStream();

    // Cloud library: the account's RD torrents (for a future "Cloud" view).
    Q_INVOKABLE void refreshTorrents();
    Q_INVOKABLE void deleteRemote(const QString &id);

signals:
    void accountChanged();
    void busyChanged();
    void statusChanged();
    void torrentsChanged();
    void streamReady(const QString &url, const QString &name);
    void errorOccurred(const QString &message);

private:
    QNetworkRequest authedRequest(const QString &path) const;
    QNetworkReply *get(const QString &path);
    QNetworkReply *post(const QString &path, const QByteArray &form);
    void loadToken();

    void onAddMagnet(QNetworkReply *r);
    void pollJob();
    void onJobInfo(QNetworkReply *r);
    void selectFiles(const QString &id, const QString &fileIds);
    void unrestrict(const QString &link);
    void onUnrestrict(QNetworkReply *r);
    void setBusy(bool b);
    void setStatus(const QString &s, int progress = -1);
    void failJob(const QString &msg);

    // Pure: index into the SELECTED-files (== links) order of the best video
    // to stream, or -1. Exposed for unit tests.
    static int pickBestLinkIndex(const QJsonArray &files);
    static bool looksLikeVideo(const QString &path);

    QNetworkAccessManager m_nam;
    QString m_token;

    bool m_authed = false;
    QString m_username, m_type, m_expiry;

    bool m_busy = false;
    int m_progress = 0;
    QString m_status;
    QVariantList m_torrents;

    // active stream job
    QString m_jobId;
    QString m_jobName;
    bool m_filesSelected = false;
    QTimer m_pollTimer;
    int m_pollTries = 0;
};

#endif // REALDEBRID_H
