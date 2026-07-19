// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// HttpDownloadManager: add/finish/remove and the persistence round-trip that
// lets a download survive a restart. Uses the shared local Range server.

#include <catch2/catch_test_macros.hpp>

#include "services/downloads/httpdownloadmanager.h"
#include "services/downloads/httpdownload.h"
#include "httptestserver.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QFile>

using httptest::makePayload;
using httptest::RangeServer;

namespace {

// The sidecar lives in the (test-mode) AppData store and is loaded on every
// manager construction — wipe it so each case starts from an empty list.
void freshStore()
{
    httptest::ensureApp();
    const QString p = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                      + QStringLiteral("/downloads/http.json");
    QFile::remove(p);
    QFile::remove(p + QStringLiteral(".corrupt"));
}

bool waitFinished(HttpDownloadManager &mgr, int timeoutMs = 15000)
{
    httptest::ensureApp();
    QSignalSpy spy(&mgr, &HttpDownloadManager::downloadFinished);
    if (spy.isEmpty() && !spy.wait(timeoutMs)) return false;
    return spy.at(0).at(2).toBool();   // (id, name, ok)
}

} // namespace

TEST_CASE("HttpDownloadManager: add downloads a file and tracks it", "[httpmanager]")
{
    freshStore();
    const QByteArray payload = makePayload(60000);
    RangeServer server(payload);
    REQUIRE(server.listen(QHostAddress::LocalHost));

    QTemporaryDir dir;
    HttpDownloadManager mgr;
    const QString id = mgr.add(server.url(), dir.path());

    REQUIRE(!id.isEmpty());
    CHECK(mgr.count() == 1);
    CHECK(mgr.byId(id) != nullptr);
    CHECK(mgr.at(0) == mgr.byId(id));

    REQUIRE(waitFinished(mgr));
    CHECK(mgr.byId(id)->state() == HttpDownload::State::Completed);

    QFile f(mgr.byId(id)->finalPath());
    REQUIRE(f.open(QIODevice::ReadOnly));
    CHECK(f.readAll() == payload);
}

TEST_CASE("HttpDownloadManager: remove with files deletes and drops the row", "[httpmanager]")
{
    freshStore();
    const QByteArray payload = makePayload(20000);
    RangeServer server(payload);
    REQUIRE(server.listen(QHostAddress::LocalHost));

    QTemporaryDir dir;
    HttpDownloadManager mgr;
    const QString id = mgr.add(server.url(), dir.path());
    REQUIRE(waitFinished(mgr));
    const QString path = mgr.byId(id)->finalPath();
    REQUIRE(QFile::exists(path));

    mgr.remove(id, /*deleteFiles=*/true);
    CHECK(mgr.count() == 0);
    CHECK(mgr.byId(id) == nullptr);
    CHECK_FALSE(QFile::exists(path));
}

TEST_CASE("HttpDownloadManager: a completed download survives a restart", "[httpmanager]")
{
    freshStore();

    const QByteArray payload = makePayload(30000);
    RangeServer server(payload);
    REQUIRE(server.listen(QHostAddress::LocalHost));

    QTemporaryDir dir;
    QString id, fileName;
    {
        HttpDownloadManager mgr;
        id = mgr.add(server.url(), dir.path());
        REQUIRE(waitFinished(mgr));
        fileName = mgr.byId(id)->fileName();
    }   // dtor flushes the sidecar

    HttpDownloadManager restored;   // reads the sidecar on construction
    REQUIRE(restored.count() == 1);
    HttpDownload *d = restored.byId(id);
    REQUIRE(d != nullptr);
    CHECK(d->state() == HttpDownload::State::Completed);
    CHECK(d->fileName() == fileName);
}
