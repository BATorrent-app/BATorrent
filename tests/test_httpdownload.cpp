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

#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QFile>
#include <QByteArray>
#include <QUrl>

namespace {

// Catch2WithMain gives us no QCoreApplication; QNAM/QTcpServer need one event
// loop for the whole run. Lazily create a single instance (house pattern).
static int   s_argc = 1;
static char  s_arg0[] = "test_httpdownload";
static char *s_argv[] = { s_arg0, nullptr };

QCoreApplication &app()
{
    static QCoreApplication *a = new QCoreApplication(s_argc, s_argv);
    return *a;
}

QByteArray makePayload(int n)
{
    QByteArray p(n, Qt::Uninitialized);
    for (int i = 0; i < n; ++i)
        p[i] = char((i * 31 + 7) & 0xFF);
    return p;
}

// Minimal one-request-per-connection HTTP/1.1 server. Honours Range for 206
// responses; in `ignoreRanges` mode it always answers 200 with the full body
// and no Accept-Ranges (drives the single-stream fallback). In `truncate` mode
// it advertises the real size but serves one byte short (drives the integrity
// gate). Optionally sets Content-Disposition to test filename extraction.
class RangeServer : public QTcpServer
{
public:
    QByteArray payload;
    bool ignoreRanges = false;
    bool truncate = false;
    QByteArray contentDisposition;   // e.g. "attachment; filename=\"real.bin\""

    explicit RangeServer(QByteArray body, QObject *parent = nullptr)
        : QTcpServer((app(), parent)), payload(std::move(body)) {}

    QUrl url() const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1/file.bin").arg(serverPort()));
    }

protected:
    void incomingConnection(qintptr handle) override
    {
        auto *sock = new QTcpSocket(this);
        sock->setSocketDescriptor(handle);
        connect(sock, &QTcpSocket::readyRead, this, [this, sock]() {
            sock->setProperty("buf", sock->property("buf").toByteArray() + sock->readAll());
            const QByteArray buf = sock->property("buf").toByteArray();
            const int hdrEnd = buf.indexOf("\r\n\r\n");
            if (hdrEnd < 0) return;   // headers not complete yet
            respond(sock, buf.left(hdrEnd));
        });
        connect(sock, &QTcpSocket::disconnected, sock, &QObject::deleteLater);
    }

private:
    void respond(QTcpSocket *sock, const QByteArray &headers)
    {
        long long from = 0, to = payload.size() - 1;
        bool ranged = false;
        for (const QByteArray &line : headers.split('\n')) {
            const QByteArray l = line.trimmed();
            if (l.toLower().startsWith("range:")) {
                const QByteArray spec = l.mid(l.indexOf('=') + 1).trimmed();
                const int dash = spec.indexOf('-');
                if (dash >= 0) {
                    from = spec.left(dash).toLongLong();
                    const QByteArray tail = spec.mid(dash + 1).trimmed();
                    if (!tail.isEmpty()) to = tail.toLongLong();
                    ranged = true;
                }
            }
        }

        // The probe is a bytes=0-0 request; never truncate it, so probing
        // succeeds and the failure surfaces on a real segment instead.
        const bool isProbe = ranged && from == 0 && to == 0;

        QByteArray body, head;
        qint64 contentLen;
        if (ranged && !ignoreRanges) {
            if (to >= payload.size()) to = payload.size() - 1;
            body = payload.mid(int(from), int(to - from + 1));
            contentLen = body.size();
            head = "HTTP/1.1 206 Partial Content\r\n";
            head += "Accept-Ranges: bytes\r\n";
            head += QByteArray("Content-Range: bytes ") + QByteArray::number(from)
                    + '-' + QByteArray::number(to) + '/'
                    + QByteArray::number(payload.size()) + "\r\n";
        } else {
            body = payload;
            contentLen = body.size();
            head = "HTTP/1.1 200 OK\r\n";
        }
        // Advertise the honest length but hang up one byte short: the client
        // sees a premature close and must fail, never finalize (integrity gate).
        if (truncate && !isProbe && body.size() > 0) body.chop(1);
        head += QByteArray("Content-Length: ") + QByteArray::number(contentLen) + "\r\n";
        if (!contentDisposition.isEmpty())
            head += "Content-Disposition: " + contentDisposition + "\r\n";
        head += "Connection: close\r\n\r\n";

        sock->write(head);
        sock->write(body);
        sock->flush();
        sock->disconnectFromHost();
    }
};

// Spin the event loop until finished() fires (or time out). Returns the ok flag.
bool runToFinish(HttpDownload &dl, int timeoutMs = 15000)
{
    app();   // ensure the event loop exists
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
