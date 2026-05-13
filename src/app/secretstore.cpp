// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "secretstore.h"
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

// Block on a QtKeychain Job until it emits finished() or the timeout fires.
// QtKeychain is async by design; for our use case (a handful of secrets read
// once on startup, written from a settings dialog) a brief synchronous wait
// is the right trade-off vs. plumbing async-everywhere through the app.
void runJob(QKeychain::Job *job)
{
    QEventLoop loop;
    QObject::connect(job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(1000, &loop, &QEventLoop::quit);
    job->start();
    loop.exec();
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
#ifdef HAVE_QTKEYCHAIN
    QKeychain::ReadPasswordJob job(QString::fromLatin1(kServiceName));
    job.setAutoDelete(false);
    job.setKey(key);
    runJob(&job);
    if (job.error() != QKeychain::NoError)
        return {};
    return job.textData();
#else
    return QSettings("BATorrent", "BATorrent").value(key).toString();
#endif
}

void SecretStore::set(const QString &key, const QString &value)
{
#ifdef HAVE_QTKEYCHAIN
    if (value.isEmpty()) {
        QKeychain::DeletePasswordJob job(QString::fromLatin1(kServiceName));
        job.setAutoDelete(false);
        job.setKey(key);
        runJob(&job);
        return;
    }
    QKeychain::WritePasswordJob job(QString::fromLatin1(kServiceName));
    job.setAutoDelete(false);
    job.setKey(key);
    job.setTextData(value);
    runJob(&job);
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
