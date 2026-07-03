// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/security/passwordhash.h"

#include <QCryptographicHash>
#include <QPasswordDigestor>
#include <QRandomGenerator>
#include <QStringList>

namespace {
// ~30-50 ms per derivation: slow enough to kill offline brute-force of a
// leaked QSettings hash, fast enough that a burst of bad Basic-Auth attempts
// can't jank the event loop before the per-IP lockout kicks in.
constexpr int kIterations = 100000;
constexpr int kSaltLen = 16;
constexpr int kKeyLen = 32;

QByteArray derive(const QByteArray &password, const QByteArray &salt, int iterations)
{
    return QPasswordDigestor::deriveKeyPbkdf2(
        QCryptographicHash::Sha256, password, salt, iterations, kKeyLen);
}
}

namespace PasswordHash {

QString hash(const QString &password)
{
    QByteArray salt(kSaltLen, 0);
    QRandomGenerator *rng = QRandomGenerator::system();
    for (int i = 0; i < salt.size(); ++i)
        salt[i] = static_cast<char>(rng->bounded(256));
    const QByteArray dk = derive(password.toUtf8(), salt, kIterations);
    return QStringLiteral("pbkdf2$%1$%2$%3")
        .arg(kIterations)
        .arg(QString::fromLatin1(salt.toHex()), QString::fromLatin1(dk.toHex()));
}

bool verify(const QString &password, const QString &stored)
{
    if (stored.isEmpty()) return false;

    if (isLegacy(stored)) {
        const QByteArray got = QCryptographicHash::hash(
            password.toUtf8(), QCryptographicHash::Sha256).toHex();
        return constantTimeEquals(got, stored.toLatin1());
    }

    const QStringList parts = stored.split(QLatin1Char('$'));
    if (parts.size() != 4 || parts[0] != QLatin1String("pbkdf2")) return false;
    bool ok = false;
    const int iterations = parts[1].toInt(&ok);
    if (!ok || iterations <= 0) return false;
    const QByteArray salt = QByteArray::fromHex(parts[2].toLatin1());
    const QByteArray want = QByteArray::fromHex(parts[3].toLatin1());
    if (salt.isEmpty() || want.isEmpty()) return false;

    return constantTimeEquals(derive(password.toUtf8(), salt, iterations), want);
}

bool isLegacy(const QString &stored)
{
    return !stored.isEmpty() && !stored.startsWith(QLatin1String("pbkdf2$"));
}

bool constantTimeEquals(const QByteArray &a, const QByteArray &b)
{
    if (a.size() != b.size()) return false;
    unsigned char diff = 0;
    for (int i = 0; i < a.size(); ++i)
        diff |= static_cast<unsigned char>(a[i]) ^ static_cast<unsigned char>(b[i]);
    return diff == 0;
}

}
