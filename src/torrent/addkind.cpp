// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "torrent/addkind.h"

#include <QByteArray>
#include <QRegularExpression>
#include <QUrl>

namespace {

bool startsWithCi(const QString &s, const char *p)
{
    return s.startsWith(QLatin1String(p), Qt::CaseInsensitive);
}

// Xunlei thunder:// links: base64 of "AA" + the real URL + "ZZ". Decoding is
// the first step, not a case of its own, because what comes out is usually a
// magnet and sometimes an ordinary link.
QString unwrapThunder(const QString &s)
{
    if (!startsWithCi(s, "thunder://")) return s;
    QString dec = QString::fromUtf8(
        QByteArray::fromBase64(s.mid(10).toLatin1())).trimmed();
    if (startsWithCi(dec, "AA") && dec.endsWith(QLatin1String("ZZ"), Qt::CaseInsensitive))
        dec = dec.mid(2, dec.size() - 4);
    return dec.trimmed();
}

} // namespace

AddTarget classifyAdd(const QString &input)
{
    QString s = unwrapThunder(input.trimmed());
    if (s.isEmpty()) return {};

    if (startsWithCi(s, "magnet:") || startsWithCi(s, "bittorrent:"))
        return { AddKind::Magnet, s };

    // A bare info-hash is what people paste off a forum post. 40 hex for v1,
    // 64 for v2.
    static const QRegularExpression hashRe(
        QStringLiteral("^(?:[0-9a-fA-F]{40}|[0-9a-fA-F]{64})$"));
    if (hashRe.match(s).hasMatch())
        return { AddKind::Magnet, QStringLiteral("magnet:?xt=urn:btih:") + s };

    if (startsWithCi(s, "http://") || startsWithCi(s, "https://")) {
        // Ask the path, not the whole string: a query like ?ref=x.torrent on a
        // plain download would otherwise send it to the engine, and a tracker
        // URL ending in .torrent with ?key=… would not reach it.
        const QString path = QUrl(s).path();
        return { path.endsWith(QLatin1String(".torrent"), Qt::CaseInsensitive)
                     ? AddKind::TorrentUrl : AddKind::WebFile, s };
    }

    if (startsWithCi(s, "file://")) {
        const QString local = QUrl(s).toLocalFile();
        if (local.endsWith(QLatin1String(".torrent"), Qt::CaseInsensitive))
            return { AddKind::TorrentFile, local };
        return {};
    }

    if (s.endsWith(QLatin1String(".torrent"), Qt::CaseInsensitive))
        return { AddKind::TorrentFile, s };

    return {};
}
