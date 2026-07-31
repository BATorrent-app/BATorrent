// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/platform/settingsbackup.h"

#include <QJsonDocument>
#include <QJsonValue>
#include <QUrl>
#include <cstring>

namespace SettingsBackup {

namespace {
constexpr char kMagic[] = "BATBACKUP1\n";
constexpr int kMagicLen = 11;
constexpr quint32 kMaxNameLen = 4096;
constexpr quint64 kMaxPayloadLen = 1073741824ULL; // 1 GiB
}

QString localPath(const QString &pathOrUrl)
{
    return pathOrUrl.startsWith(QStringLiteral("file:"))
               ? QUrl(pathOrUrl).toLocalFile()
               : pathOrUrl;
}

QStringList exportSecretKeys()
{
    return {
        QStringLiteral("proxyPass"),
        QStringLiteral("plexToken"),
        QStringLiteral("jellyfinApiKey"),
        QStringLiteral("webUiPasswordHash"),
    };
}

bool isExportSecret(const QString &key)
{
    return exportSecretKeys().contains(key);
}

QJsonObject settingsObjectFromMap(const QVariantMap &map, bool stripSecrets)
{
    QJsonObject obj;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        if (stripSecrets && isExportSecret(it.key()))
            continue;
        obj.insert(it.key(), QJsonValue::fromVariant(it.value()));
    }
    return obj;
}

QByteArray pack(const QList<QPair<QString, QByteArray>> &entries)
{
    QByteArray archive(kMagic);
    const quint32 count = quint32(entries.size());
    archive.append(reinterpret_cast<const char *>(&count), 4);
    for (const auto &e : entries) {
        const QByteArray nb = e.first.toUtf8();
        const quint32 nl = quint32(nb.size());
        const quint64 dl = quint64(e.second.size());
        archive.append(reinterpret_cast<const char *>(&nl), 4);
        archive.append(nb);
        archive.append(reinterpret_cast<const char *>(&dl), 8);
        archive.append(e.second);
    }
    return archive;
}

UnpackResult unpack(const QByteArray &data)
{
    UnpackResult out;
    if (!data.startsWith(kMagic))
        return out;
    const char *p = data.constData() + kMagicLen;
    const char *end = data.constData() + data.size();
    if (p + 4 > end)
        return out;
    quint32 count = 0;
    memcpy(&count, p, 4);
    p += 4;
    out.entries.reserve(int(count));
    for (quint32 i = 0; i < count; ++i) {
        if (end - p < 4)
            return {};
        quint32 nl = 0;
        memcpy(&nl, p, 4);
        p += 4;
        if (nl > kMaxNameLen || static_cast<ptrdiff_t>(nl) > end - p)
            return {};
        const QString name = QString::fromUtf8(p, int(nl));
        p += nl;
        if (end - p < 8)
            return {};
        quint64 dl = 0;
        memcpy(&dl, p, 8);
        p += 8;
        if (dl > kMaxPayloadLen || static_cast<ptrdiff_t>(dl) > end - p)
            return {};
        out.entries.append({name, QByteArray(p, int(dl))});
        p += dl;
    }
    out.ok = true;
    return out;
}

} // namespace SettingsBackup
