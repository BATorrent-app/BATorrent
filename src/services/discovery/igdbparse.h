// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QVariantList>

// Pure IGDB response → poster-card mapping (no network).
namespace IgdbParse {

// IGDB has no free flag — drop obvious F2P / live-service / MMO titles that
// dominate popularity lists but aren't torrentable. Substrings stay specific
// enough not to wipe paid franchise entries (e.g. "warzone" ≠ "call of duty").
bool isFreeLiveService(const QString &name);

QList<QJsonObject> objectsFromArray(const QJsonArray &arr);
QList<QJsonObject> objectsFromJson(const QByteArray &jsonArray);

// Cover required, de-duped by title, capped. Rating is IGDB /10 → 0..10 scale.
QVariantList gameCards(const QList<QJsonObject> &objs, int cap);
QVariantList gameCardsFromJson(const QByteArray &jsonArray, int cap);

} // namespace IgdbParse
