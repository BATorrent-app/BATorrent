// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SECRETSTORE_H
#define SECRETSTORE_H

#include <QString>

// Synchronous wrapper over QtKeychain. When QtKeychain isn't available at
// build time the wrapper falls back to QSettings — the app still works,
// just without OS-level encryption.
//
// Migration: callers that previously read plaintext from QSettings can
// invoke migrateFromSettings() on startup; existing values are copied into
// the keyring and removed from the settings store.
class SecretStore
{
public:
    static SecretStore &instance();

    // Returns the stored secret for `key`, or empty if not set. Blocks the
    // calling thread briefly while QtKeychain talks to the OS keyring (with
    // a 1 s timeout fallback).
    QString get(const QString &key);

    // Stores `value` under `key`. Empty values delete the entry.
    void set(const QString &key, const QString &value);

    // Best-effort one-time migration: for each key listed, if it exists in
    // QSettings, copy it into the keyring and clear the QSettings entry.
    // Returns how many keys were migrated.
    int migrateFromSettings(const QStringList &keys);

    // True when a real keyring backend is available. False when we're
    // falling back to QSettings — caller may want to surface that fact to
    // the user so they understand secrets aren't encrypted on disk.
    bool isSecure() const;

private:
    SecretStore() = default;
};

#endif
