// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef IDEBRIDPROVIDER_H
#define IDEBRIDPROVIDER_H

#include <QObject>
#include <QString>

// Contract for a debrid service (Real-Debrid, TorBox, …): hand it a magnet, it
// caches/downloads on its own servers, then resolves a direct HTTPS link the
// built-in player streams — no local seeding, no IP exposure.
//
// Providers are not exposed to QML directly; DebridManager owns them and bridges
// the active one. So this is a plain C++ interface (signals only, no Q_PROPERTY).
// Account fields are normalized: name (user/email), plan (premium/free label).
class IDebridProvider : public QObject
{
    Q_OBJECT
public:
    explicit IDebridProvider(QObject *parent = nullptr) : QObject(parent) {}
    ~IDebridProvider() override = default;

    virtual QString id() const = 0;            // stable key, e.g. "realdebrid"
    virtual QString displayName() const = 0;   // "Real-Debrid"

    virtual bool authed() const = 0;           // token valid + premium
    virtual QString accountName() const = 0;
    virtual QString accountPlan() const = 0;
    virtual QString expiry() const = 0;
    virtual bool hasToken() const = 0;
    virtual bool busy() const = 0;
    virtual int progress() const = 0;          // 0..100, active stream job
    virtual QString status() const = 0;        // human stage text

    virtual void setToken(const QString &token) = 0;
    virtual void clearToken() = 0;
    virtual void checkAccount() = 0;
    virtual void streamMagnet(const QString &magnet) = 0;
    virtual void cancelStream() = 0;

signals:
    void accountChanged();
    void busyChanged();
    void statusChanged();
    void streamReady(const QString &url, const QString &name);
    void errorOccurred(const QString &message);
};

#endif // IDEBRIDPROVIDER_H
