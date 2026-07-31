// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/discovery/addonfinish.h"

#include "services/discovery/addonparse.h"

namespace AddonFinish {

ReplyOutcome applyCatalogReply(quint32 activeGen,
                               int &pending,
                               QList<CatalogItem> &results,
                               quint32 replyGen,
                               bool ok,
                               const QByteArray &body)
{
    if (replyGen != activeGen)
        return ReplyOutcome::Stale;

    --pending;
    if (ok) {
        for (const auto &item : AddonParse::parseCatalogMetas(body)) {
            bool dup = false;
            for (const auto &existing : results) {
                if (existing.id == item.id) {
                    dup = true;
                    break;
                }
            }
            if (!dup)
                results.append(item);
        }
    }
    return pending <= 0 ? ReplyOutcome::Finished : ReplyOutcome::Progress;
}

ReplyOutcome applyStreamReply(quint32 activeGen,
                              int &pending,
                              QList<StreamResult> &results,
                              quint32 replyGen,
                              bool ok,
                              const QByteArray &body,
                              const QString &addonName)
{
    if (replyGen != activeGen)
        return ReplyOutcome::Stale;

    --pending;
    if (ok) {
        for (const auto &r : AddonParse::parseStreamResults(body, addonName))
            results.append(r);
    }
    return pending <= 0 ? ReplyOutcome::Finished : ReplyOutcome::Progress;
}

} // namespace AddonFinish
