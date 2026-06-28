// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/security/secretstore.h"
#include <QSettings>
#include <QStringList>

#ifdef HAVE_QTKEYCHAIN
#include <QEventLoop>
#include <QTimer>
#include <qt6keychain/keychain.h>

namespace {
// All BATorrent entries live under one keyring service so they're easy to
// inspect / wipe via the OS (Credential Manager on Windows, Keychain on
// macOS, seahorse on Linux).
constexpr const char *kServiceName = "BATorrent";

// Run a heap-allocated QtKeychain job synchronously. Returns true if the job
// finished before timeoutMs. On timeout we return false but keep the job
// alive (autoDelete) so its eventual completion doesn't access destroyed
// memory — this is the bug we used to have: a 1 s timeout on a cold keychain
// (just-unlocked Keychain on macOS, gnome-keyring spinning up on Linux)
// could destroy the job while QtKeychain was still in flight.
bool runJob(QKeychain::Job *job, int timeoutMs = 5000)
{
    QEventLoop loop;
    bool finished = false;
    QObject::connect(job, &QKeychain::Job::finished, &loop,
                     [&]() { finished = true; loop.quit(); });
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    job->setAutoDelete(true);
    job->start();
    loop.exec();
    return finished;
}
}
#endif

SecretStore &SecretStore::instance()
{
    static SecretStore s;
    return s;
}

bool SecretStore::isSecure() const
{
#ifdef HAVE_QTKEYCHAIN
    return true;
#else
    return false;
#endif
}

QString SecretStore::get(const QString &key)
{
    auto it = m_cache.constFind(key);
    if (it != m_cache.constEnd()) return it.value();
#ifdef HAVE_QTKEYCHAIN
    auto *job = new QKeychain::ReadPasswordJob(QString::fromLatin1(kServiceName));
    job->setKey(key);
    if (!runJob(job)) return {};   // timed out — don't cache, retry next time
    QString value = (job->error() == QKeychain::NoError) ? job->textData() : QString();
#else
    QString value = QSettings("BATorrent", "BATorrent").value(key).toString();
#endif
    m_cache.insert(key, value);
    return value;
}

void SecretStore::set(const QString &key, const QString &value)
{
    m_cache.insert(key, value);   // we're the only writer — keep the cache coherent
#ifdef HAVE_QTKEYCHAIN
    if (value.isEmpty()) {
        auto *job = new QKeychain::DeletePasswordJob(QString::fromLatin1(kServiceName));
        job->setKey(key);
        runJob(job);
        return;
    }
    auto *job = new QKeychain::WritePasswordJob(QString::fromLatin1(kServiceName));
    job->setKey(key);
    job->setTextData(value);
    runJob(job);
#else
    QSettings settings("BATorrent", "BATorrent");
    if (value.isEmpty())
        settings.remove(key);
    else
        settings.setValue(key, value);
#endif
}

int SecretStore::migrateFromSettings(const QStringList &keys)
{
    int migrated = 0;
#ifdef HAVE_QTKEYCHAIN
    QSettings settings("BATorrent", "BATorrent");
    for (const QString &key : keys) {
        if (!settings.contains(key)) continue;
        const QString value = settings.value(key).toString();
        if (value.isEmpty()) { settings.remove(key); continue; }
        // Push into keyring before clearing from QSettings so a crash mid-
        // migration doesn't lose the credential.
        set(key, value);
        if (!get(key).isEmpty()) {
            settings.remove(key);
            ++migrated;
        }
    }
#else
    Q_UNUSED(keys);
#endif
    return migrated;
}
