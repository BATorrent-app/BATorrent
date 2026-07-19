// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SERVICES_DOWNLOADS_FILEHOSTRESOLVER_H
#define SERVICES_DOWNLOADS_FILEHOSTRESOLVER_H

// Turn a file-host *page* URL into the direct-download URL the HTTP engine can
// fetch. Kept to deterministic, API-free rewrites (no captcha, no wait page) so
// it stays a pure, unit-tested function: pixeldrain's viewer path maps 1:1 to
// its file API. An already-direct link — or a host we don't special-case — is
// returned unchanged; HttpDownload's probe + integrity gate then either fetch it
// or reject an HTML error page cleanly. Hosts that need an API token or a wait
// page (gofile, 1fichier, mediafire) are deliberately out of scope here — a
// future async resolver handles those.

#include <QUrl>
#include <QString>
#include <QRegularExpression>

namespace bat {

inline QUrl directDownloadUrl(const QUrl &page)
{
    const QString host = page.host();
    const QString path = page.path();

    // pixeldrain: https://pixeldrain.com/u/<id>  →  /api/file/<id> (raw bytes).
    // The /l/ list pages and everything else fall through unchanged.
    if (host == QLatin1String("pixeldrain.com") || host.endsWith(QLatin1String(".pixeldrain.com"))) {
        static const QRegularExpression re(QStringLiteral("^/u/([A-Za-z0-9]+)"));
        const auto m = re.match(path);
        if (m.hasMatch())
            return QUrl(QStringLiteral("https://pixeldrain.com/api/file/") + m.captured(1));
    }

    return page;   // already-direct, or a host we don't rewrite — try as-is
}

} // namespace bat

#endif
