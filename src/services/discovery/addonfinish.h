// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include "services/discovery/addonmanager.h"

#include <QByteArray>
#include <QList>
#include <QString>

// Offline-testable AddonManager HTTP reply finish: gen-counter stale drop,
// parse+dedupe, pending → Finished. AddonManager owns QNAM + emit.
namespace AddonFinish {

enum class ReplyOutcome { Stale, Progress, Finished };

// Stale (replyGen != activeGen): no pending change, no results mutation.
// Else: decrement pending; on ok, parse catalog metas and append de-duped by id.
ReplyOutcome applyCatalogReply(quint32 activeGen,
                               int &pending,
                               QList<CatalogItem> &results,
                               quint32 replyGen,
                               bool ok,
                               const QByteArray &body);

ReplyOutcome applyStreamReply(quint32 activeGen,
                              int &pending,
                              QList<StreamResult> &results,
                              quint32 replyGen,
                              bool ok,
                              const QByteArray &body,
                              const QString &addonName);

} // namespace AddonFinish
