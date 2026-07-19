// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// Integration test for HttpDownload against a local QTcpServer that serves a
// known payload with real HTTP/1.1 Range support. Exercises the probe →
// segmented download → integrity gate → rename path, the 200 single-stream
// fallback, deterministic resume from a persisted state, and the size-mismatch
// guard. Network classes stay thin; the segment math is unit-tested separately
// in test_rangeplan.cpp.

#include <catch2/catch_test_macros.hpp>

#include "services/downloads/httpdownload.h"
#include "httptestserver.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QFile>
#include <QByteArray>
#include <QUrl>

using httptest::makePayload;
using httptest::RangeServer;

namespace {

// Spin the event loop until finished() fires (or time out). Returns the ok flag.
bool runToFinish(HttpDownload &dl, int timeoutMs = 15000)
{
    httptest::ensureApp();   // event loop for QNAM
    QSignalSpy spy(&dl, &HttpDownload::finished);
    dl.start();
    if (spy.isEmpty() && !spy.wait(timeoutMs)) return false;
    return spy.at(0).at(0).toBool();
}

QByteArray readAll(const QString &path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

} // namespace

TEST_CASE("HttpDownload: segmented download reassembles the exact payload", "[httpdownload]")
{
    const QByteArray payload = makePayload(100000);
    RangeServer server(payload);
    REQUIRE(server.listen(QHostAddress::LocalHost));

    QTemporaryDir dir;
    HttpDownload dl(QStringLiteral("t1"), server.url(), dir.path(), QStringLiteral("out.bin"));

    REQUIRE(runToFinish(dl));
    CHECK(dl.state() == HttpDownload::State::Completed);
    CHECK(dl.totalBytes() == payload.size());
    CHECK(readAll(dl.finalPath()) == payload);
    CHECK_FALSE(QFile::exists(dl.partPath()));   // .part renamed away
}

TEST_CASE("HttpDownload: 200 (no ranges) falls back to a single stream", "[httpdownload]")
{
    const QByteArray payload = makePayload(40000);
    RangeServer server(payload);
    server.ignoreRanges = true;
    REQUIRE(server.listen(QHostAddress::LocalHost));

    QTemporaryDir dir;
    HttpDownload dl(QStringLiteral("t2"), server.url(), dir.path(), QStringLiteral("out.bin"));

    REQUIRE(runToFinish(dl));
    CHECK(dl.state() == HttpDownload::State::Completed);
    CHECK(readAll(dl.finalPath()) == payload);
}

TEST_CASE("HttpDownload: resumes from a persisted half-done segment", "[httpdownload]")
{
    const QByteArray payload = makePayload(80000);
    RangeServer server(payload);
    REQUIRE(server.listen(QHostAddress::LocalHost));

    QTemporaryDir dir;
    const qint64 half = payload.size() / 2;

    // Simulate an interrupted download: a preallocated .part with the first half
    // already correct, and a sidecar state describing one half-received segment.
    HttpDownload dl(QStringLiteral("t3"), server.url(), dir.path(), QStringLiteral("out.bin"));
    {
        QFile part(dl.partPath());
        REQUIRE(part.open(QIODevice::WriteOnly));
        part.resize(payload.size());
        part.seek(0);
        part.write(payload.left(int(half)));
        part.close();
    }
    QVariantMap state;
    state["total"] = qint64(payload.size());
    state["acceptRanges"] = true;
    QVariantList segs;
    QVariantMap seg;
    seg["start"] = qint64(0);
    seg["end"] = qint64(payload.size() - 1);
    seg["received"] = half;                 // second half still to fetch
    segs << seg;
    state["segments"] = segs;
    dl.restoreState(state);

    REQUIRE(runToFinish(dl));
    CHECK(dl.state() == HttpDownload::State::Completed);
    CHECK(readAll(dl.finalPath()) == payload);
}

TEST_CASE("HttpDownload: a short read trips the integrity gate", "[httpdownload]")
{
    const QByteArray payload = makePayload(30000);
    RangeServer server(payload);
    server.truncate = true;   // advertises full size, serves one byte short
    REQUIRE(server.listen(QHostAddress::LocalHost));

    QTemporaryDir dir;
    HttpDownload dl(QStringLiteral("t4"), server.url(), dir.path(), QStringLiteral("out.bin"));

    const bool ok = runToFinish(dl);
    CHECK_FALSE(ok);
    CHECK(dl.state() == HttpDownload::State::Failed);
    CHECK_FALSE(QFile::exists(dl.finalPath()));   // never finalized
}

TEST_CASE("HttpDownload: Content-Disposition sets the output filename", "[httpdownload]")
{
    const QByteArray payload = makePayload(5000);
    RangeServer server(payload);
    server.contentDisposition = "attachment; filename=\"real-name.bin\"";
    REQUIRE(server.listen(QHostAddress::LocalHost));

    QTemporaryDir dir;
    // Empty fileName + no useful URL path segment ⇒ synthesized "download-<id>",
    // which is exactly the marker HttpDownload replaces from Content-Disposition.
    HttpDownload dl(QStringLiteral("t5"),
                    QUrl(QStringLiteral("http://127.0.0.1:%1/").arg(server.serverPort())),
                    dir.path());

    REQUIRE(runToFinish(dl));
    CHECK(dl.fileName() == QStringLiteral("real-name.bin"));
    CHECK(readAll(dl.finalPath()) == payload);
}
