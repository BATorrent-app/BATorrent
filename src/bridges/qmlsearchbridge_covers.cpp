// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSearchBridge — poster/cover resolution against MetadataResolver.

#include "bridges/qmlsearchbridge.h"
#include "services/metadata/metadataresolver.h"

void QmlSearchBridge::setResolver(MetadataResolver *r)
{
    m_resolver = r;
    if (!m_resolver) return;
    // Poster fills mutate m_results WITHOUT resultsChanged() on purpose: QML
    // treats that signal as "new result set" (closes the detail panel, resets
    // the view); delegates repaint targeted via coverReady instead.
    connect(m_resolver, &MetadataResolver::metadataReady, this,
            [this](const QString &infoHash, const MetadataResult &meta) {
        if (!meta.valid || meta.posterPath.isEmpty()) return;
        for (auto &v : m_results) {
            QVariantMap m = v.toMap();
            if (m.value(QStringLiteral("coverHash")).toString() == infoHash
                && m.value(QStringLiteral("poster")).toString().isEmpty()) {
                m["poster"] = meta.posterPath;
                v = m;
            }
        }
        emit coverReady(infoHash, meta.posterPath);
    });
}

void QmlSearchBridge::resolveCover(int index)
{
    if (!m_resolver || index < 0 || index >= m_results.size()) return;
    const QVariantMap m = m_results[index].toMap();
    if (!m.value(QStringLiteral("poster")).toString().isEmpty()) return;
    const QString hash = m.value(QStringLiteral("coverHash")).toString();
    if (hash.isEmpty()) return;
    if (m_resolver->hasCached(hash)) {
        const auto meta = m_resolver->cached(hash);
        if (meta.valid && !meta.posterPath.isEmpty()) {
            QVariantMap mm = m;
            mm["poster"] = meta.posterPath;
            m_results[index] = mm;
            emit coverReady(hash, meta.posterPath);
        }
        return;
    }
    m_resolver->resolve(hash, m.value(QStringLiteral("name")).toString());
}

