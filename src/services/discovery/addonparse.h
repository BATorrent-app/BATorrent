// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include "services/discovery/addonmanager.h"
#include "services/platform/translator.h"

#include <QByteArray>
#include <QJsonValue>
#include <QString>

// Pure addon/provider helpers (no network, no QSettings except torrentioLanguageForApp).
namespace AddonParse {

bool isValidInfoHash(const QString &h);
QString normalizeAddonBaseUrl(QString url);
QString streamBaseUrl(const QString &addonUrl, const QString &torrentioLang);

// Torrentio language= tag for a content language. Empty for English / unknown.
QString torrentioLanguageTag(Translator::Language lang);
// preferNativeLang setting + ContentLanguage::current().
QString torrentioLanguageForApp();

QString magnetTrackerParams();
qint64 parseSizeValue(const QJsonValue &v);

QList<TorrentSearchResult> parseProviderResponse(const SearchProvider &p,
                                                 const QByteArray &data);

// Stremio manifest.json body → AddonManifest (url is the install base).
bool parseManifestJson(const QByteArray &data, const QString &baseUrl, AddonManifest *out);

// catalog response metas[] → CatalogItem rows (caller owns dedupe).
QList<CatalogItem> parseCatalogMetas(const QByteArray &data);

// stream response streams[] → StreamResult rows (magnet-only kept).
QList<StreamResult> parseStreamResults(const QByteArray &data, const QString &addonName);

// Legacy apibay-shaped root array (searchTorrents).
QList<TorrentSearchResult> parseApibayArray(const QByteArray &data,
                                            const QString &providerName = QStringLiteral("Torrents"));

} // namespace AddonParse
