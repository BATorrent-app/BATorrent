// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef REALDEBRID_H
#define REALDEBRID_H

#include "services/integrations/idebridprovider.h"

#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QString>
#include <QTimer>

// Real-Debrid (real-debrid.com) debrid provider. Auth is a private API token
// (real-debrid.com/apitoken), stored via SecretStore. One stream job at a time:
//   addMagnet -> selectFiles(video) -> poll info until "downloaded"
//   -> unrestrict the largest video link -> streamReady(url).
class RealDebridClient : public IDebridProvider
{
    Q_OBJECT
public:
    explicit RealDebridClient(QObject *parent = nullptr);

    QString id() const override { return QStringLiteral("realdebrid"); }
    QString displayName() const override { return QStringLiteral("Real-Debrid"); }

    bool authed() const override { return m_authed; }
    QString accountName() const override { return m_username; }
    QString accountPlan() const override { return m_type; }
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

    QNetworkAccessManager m_nam;
    QString m_token;

    bool m_authed = false;
    QString m_username, m_type, m_expiry;

    bool m_busy = false;
    int m_progress = 0;
    QString m_status;

    // active stream job
    QString m_jobId;
    QString m_jobName;
    bool m_filesSelected = false;
    QTimer m_pollTimer;
    int m_pollTries = 0;
};

#endif // REALDEBRID_H
