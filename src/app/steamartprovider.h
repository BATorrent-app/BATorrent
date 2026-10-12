// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QQuickAsyncImageProvider>

// image://steamart/<hero|logo>/<appid>: a game's Steam library art. The store
// keeps it under hashed paths only the store API knows, so this asks
// IStoreBrowseService once per game, then fetches the file. A game with no
// such art answers with an error, which lets ChainImage try its next source
// and TitleMark fall back to the title in text.
class SteamArtProvider : public QQuickAsyncImageProvider
{
public:
    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;
};
