// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QVariantMap>

// Settings export (JSON, secrets stripped) and full backup archive (BATBACKUP1).
// Pure format/policy leaves — I/O stays in the settings bridge.
namespace SettingsBackup {

QString localPath(const QString &pathOrUrl);

// Keys omitted from JSON export (not from full backup — restore needs them).
QStringList exportSecretKeys();
bool isExportSecret(const QString &key);

QJsonObject settingsObjectFromMap(const QVariantMap &map, bool stripSecrets);

// Pack/unpack the BATBACKUP1 binary format. unpack() rejects truncated or
// oversized frames the same way fullRestore used to inline.
QByteArray pack(const QList<QPair<QString, QByteArray>> &entries);

struct UnpackResult {
    bool ok = false;
    QList<QPair<QString, QByteArray>> entries;
};
UnpackResult unpack(const QByteArray &data);

} // namespace SettingsBackup
