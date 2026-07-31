// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QByteArray>
#include <QMap>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

// Offline-testable DiscoveryService finish wiring: parse HTTP bodies into
// accum/search state, then assemble/rank when the pending counter hits zero.
// DiscoveryService owns QNAM + emit + cache.
namespace DiscoveryFinish {

void ingestTmdbShelf(QMap<int, QVariantMap> &accum,
                     int order,
                     const QString &label,
                     bool isTv,
                     bool ok,
                     const QByteArray &body,
                     const QString &posterBase,
                     const QString &backdropBase);

void ingestTmdbSearch(QVariantList &works,
                      bool ok,
                      const QByteArray &body,
                      const QString &posterBase);

void ingestIgdbSearch(QVariantList &works, bool ok, const QByteArray &body);

// Decrement pending. When it hits 0, write rows/hero from accum and return true.
bool tryFinishShelves(int &pending,
                      const QMap<int, QVariantMap> &accum,
                      QVariantList *rowsOut,
                      QVariantList *heroOut);

// Decrement pending. When it hits 0, write ranked search results and return true.
bool tryFinishSearch(int &pending,
                     const QString &query,
                     const QVariantList &works,
                     QVariantList *rankedOut);

} // namespace DiscoveryFinish
