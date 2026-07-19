// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include <catch2/catch_test_macros.hpp>

#include "services/downloads/filehostresolver.h"

using bat::directDownloadUrl;

TEST_CASE("directDownloadUrl: pixeldrain viewer maps to the file API", "[filehost]")
{
    CHECK(directDownloadUrl(QUrl("https://pixeldrain.com/u/abc123")).toString()
          == "https://pixeldrain.com/api/file/abc123");
    // trailing segments / query on the viewer URL don't change the file id
    CHECK(directDownloadUrl(QUrl("https://pixeldrain.com/u/XyZ9/anything?x=1")).toString()
          == "https://pixeldrain.com/api/file/XyZ9");
}

TEST_CASE("directDownloadUrl: non-viewer pixeldrain paths pass through", "[filehost]")
{
    // list pages aren't a single file — leave them alone
    const QUrl list("https://pixeldrain.com/l/somelist");
    CHECK(directDownloadUrl(list) == list);
}

TEST_CASE("directDownloadUrl: already-direct and unknown hosts are unchanged", "[filehost]")
{
    const QUrl direct("https://cdn.example.com/games/setup.zip");
    CHECK(directDownloadUrl(direct) == direct);

    const QUrl gofile("https://gofile.io/d/AbCdE");   // needs an API — out of scope, passthrough
    CHECK(directDownloadUrl(gofile) == gofile);
}
