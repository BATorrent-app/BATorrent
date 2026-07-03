// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QByteArray>
#include <QString>

// Password hashing for the Web UI credential. The stored format is
// "pbkdf2$<iterations>$<saltHex>$<dkHex>"; verify() also accepts the legacy
// bare SHA-256 hex digest so pre-existing installs keep working until their
// first successful login rehashes them (see WebServer::passwordHashUpgraded).
namespace PasswordHash {

QString hash(const QString &password);
bool verify(const QString &password, const QString &stored);
bool isLegacy(const QString &stored);

bool constantTimeEquals(const QByteArray &a, const QByteArray &b);

}
