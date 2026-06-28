// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef DEBRIDMANAGER_H
#define DEBRIDMANAGER_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVector>

class IDebridProvider;

// The single QML-facing debrid object. Owns every provider (Real-Debrid, TorBox),
// tracks which one is active, and delegates the IDebridProvider contract to it so
// the UI stays provider-agnostic — one "Stream via debrid" path, one settings row.
class DebridManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString providerId READ providerId WRITE setProviderId NOTIFY providerChanged)
    Q_PROPERTY(QString providerName READ providerName NOTIFY providerChanged)
    Q_PROPERTY(QVariantList providers READ providers NOTIFY providersChanged)
    Q_PROPERTY(bool authed READ authed NOTIFY accountChanged)
    Q_PROPERTY(QString accountName READ accountName NOTIFY accountChanged)
    Q_PROPERTY(QString accountPlan READ accountPlan NOTIFY accountChanged)
    Q_PROPERTY(QString expiry READ expiry NOTIFY accountChanged)
    Q_PROPERTY(bool hasToken READ hasToken NOTIFY accountChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(int progress READ progress NOTIFY statusChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    explicit DebridManager(QObject *parent = nullptr);

    QString providerId() const;
    QString providerName() const;
    QVariantList providers() const;   // [{ id, name, hasToken }] for the picker
    bool authed() const;
    QString accountName() const;
    QString accountPlan() const;
    QString expiry() const;
    bool hasToken() const;
    bool busy() const;
    int progress() const;
    QString status() const;

    void setProviderId(const QString &id);

    Q_INVOKABLE void setToken(const QString &token);
    Q_INVOKABLE void clearToken();
    Q_INVOKABLE void checkAccount();
    Q_INVOKABLE void streamMagnet(const QString &magnet);
    Q_INVOKABLE void cancelStream();

signals:
    void providerChanged();
    void providersChanged();
    void accountChanged();
    void busyChanged();
    void statusChanged();
    void streamReady(const QString &url, const QString &name);
    void errorOccurred(const QString &message);

private:
    IDebridProvider *active() const;

    QVector<IDebridProvider *> m_providers;
    int m_active = 0;
};

#endif // DEBRIDMANAGER_H
