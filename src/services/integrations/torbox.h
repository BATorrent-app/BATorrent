// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef TORBOX_H
#define TORBOX_H

#include "services/integrations/idebridprovider.h"

#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QString>
#include <QTimer>

// TorBox (torbox.app) debrid provider. Auth is an API key (torbox.app/settings),
// stored via SecretStore. One stream job at a time:
//   createtorrent(magnet) -> poll mylist until download_finished
//   -> requestdl on the largest video file -> streamReady(url).
class TorBoxClient : public IDebridProvider
{
    Q_OBJECT
public:
    explicit TorBoxClient(QObject *parent = nullptr);

    QString id() const override { return QStringLiteral("torbox"); }
    QString displayName() const override { return QStringLiteral("TorBox"); }

    bool authed() const override { return m_authed; }
    QString accountName() const override { return m_email; }
    QString accountPlan() const override { return m_plan; }
    QString expiry() const override { return m_expiry; }
    bool hasToken() const override { return !m_token.isEmpty(); }
    bool busy() const override { return m_busy; }
    int progress() const override { return m_progress; }
    QString status() const override { return m_status; }

    void setToken(const QString &token) override;
    void clearToken() override;
    void checkAccount() override;
    void streamMagnet(const QString &magnet) override;
    void cancelStream() override;

private:
    QNetworkRequest authedRequest(const QString &path) const;
    QNetworkReply *get(const QString &path);
    void loadToken();

    void onCreate(QNetworkReply *r);
    void pollJob();
    void onJobInfo(QNetworkReply *r);
    void requestLink(int fileId);
    void onRequestLink(QNetworkReply *r, const QString &name);
    void setBusy(bool b);
    void setStatus(const QString &s, int progress = -1);
    void failJob(const QString &msg);

    QNetworkAccessManager m_nam;
    QString m_token;

    bool m_authed = false;
    QString m_email, m_plan, m_expiry;

    bool m_busy = false;
    int m_progress = 0;
    QString m_status;

    // active stream job
    QString m_jobId;       // TorBox torrent_id, as string
    QString m_jobName;
    QTimer m_pollTimer;
    int m_pollTries = 0;
};

#endif // TORBOX_H
