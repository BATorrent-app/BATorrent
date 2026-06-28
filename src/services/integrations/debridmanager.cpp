// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/integrations/debridmanager.h"

#include "services/integrations/realdebrid.h"
#include "services/integrations/torbox.h"

#include <QSettings>
#include <QVariantMap>

namespace {
constexpr auto kProviderKey = "debrid/provider";
}

DebridManager::DebridManager(QObject *parent)
    : QObject(parent)
{
    m_providers = {new RealDebridClient(this), new TorBoxClient(this)};

    // The active provider's signals surface as the manager's own — QML listens once.
    for (IDebridProvider *p : m_providers) {
        connect(p, &IDebridProvider::accountChanged, this, [this, p] {
            emit providersChanged();
            if (p == active()) emit accountChanged();
        });
        connect(p, &IDebridProvider::busyChanged, this, [this, p] {
            if (p == active()) emit busyChanged();
        });
        connect(p, &IDebridProvider::statusChanged, this, [this, p] {
            if (p == active()) emit statusChanged();
        });
        connect(p, &IDebridProvider::streamReady, this, [this, p](const QString &u, const QString &n) {
            if (p == active()) emit streamReady(u, n);
        });
        connect(p, &IDebridProvider::errorOccurred, this, [this, p](const QString &m) {
            if (p == active()) emit errorOccurred(m);
        });
    }

    const QString saved = QSettings().value(QString::fromLatin1(kProviderKey)).toString();
    for (int i = 0; i < m_providers.size(); ++i)
        if (m_providers[i]->id() == saved) { m_active = i; break; }
}

IDebridProvider *DebridManager::active() const
{
    return m_providers.value(m_active, nullptr);
}

void DebridManager::setProviderId(const QString &id)
{
    for (int i = 0; i < m_providers.size(); ++i) {
        if (m_providers[i]->id() != id || i == m_active) continue;
        // Switching mid-stream would orphan the running job — stop it cleanly first.
        if (active() && active()->busy()) active()->cancelStream();
        m_active = i;
        QSettings().setValue(QString::fromLatin1(kProviderKey), id);
        emit providerChanged();
        emit accountChanged();
        emit busyChanged();
        emit statusChanged();
        return;
    }
}

QString DebridManager::providerId() const { return active() ? active()->id() : QString(); }
QString DebridManager::providerName() const { return active() ? active()->displayName() : QString(); }

QVariantList DebridManager::providers() const
{
    QVariantList out;
    for (IDebridProvider *p : m_providers)
        out << QVariantMap{
            {QStringLiteral("id"), p->id()},
            {QStringLiteral("name"), p->displayName()},
            {QStringLiteral("hasToken"), p->hasToken()},
        };
    return out;
}

bool DebridManager::authed() const { return active() && active()->authed(); }
QString DebridManager::accountName() const { return active() ? active()->accountName() : QString(); }
QString DebridManager::accountPlan() const { return active() ? active()->accountPlan() : QString(); }
QString DebridManager::expiry() const { return active() ? active()->expiry() : QString(); }
bool DebridManager::hasToken() const { return active() && active()->hasToken(); }
bool DebridManager::busy() const { return active() && active()->busy(); }
int DebridManager::progress() const { return active() ? active()->progress() : 0; }
QString DebridManager::status() const { return active() ? active()->status() : QString(); }

void DebridManager::setToken(const QString &token) { if (active()) active()->setToken(token); }
void DebridManager::clearToken() { if (active()) active()->clearToken(); }
void DebridManager::checkAccount() { if (active()) active()->checkAccount(); }
void DebridManager::streamMagnet(const QString &magnet) { if (active()) active()->streamMagnet(magnet); }
void DebridManager::cancelStream() { if (active()) active()->cancelStream(); }
