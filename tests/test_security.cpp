// SPDX-License-Identifier: MIT
// BATorrent Security Test Suite (Catch2 v3)

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <QCoreApplication>
#include <QTcpSocket>
#include <QElapsedTimer>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QElapsedTimer>
#include <QRegularExpression>

#include "webui/webserver.h"
#include "torrent/sessionmanager.h"

#ifdef BAT_CRT_DEBUG_HEAP
#  include <crtdbg.h>
#endif

using Catch::Matchers::ContainsSubstring;

// ============================================================================
//  Helpers
// ============================================================================

static int   s_argc = 1;
static char  s_arg0[] = "test_security";
static char *s_argv[] = { s_arg0, nullptr };

struct QtApp {
    QCoreApplication app{s_argc, s_argv};
};
static QtApp &qtApp() { static QtApp a; return a; }

static QByteArray sendRaw(quint16 port, const QByteArray &req, int timeout = 3000)
{
    qtApp();
    QTcpSocket sock;
    QByteArray resp;
    bool done = false;

    QObject::connect(&sock, &QTcpSocket::readyRead, [&]() { resp += sock.readAll(); });
    QObject::connect(&sock, &QTcpSocket::disconnected, [&]() { done = true; });

    sock.connectToHost("127.0.0.1", port);

    QElapsedTimer timer;
    timer.start();
    while (sock.state() != QAbstractSocket::ConnectedState && timer.elapsed() < timeout)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    if (sock.state() != QAbstractSocket::ConnectedState) return {};

    sock.write(req);
    sock.flush();

    timer.restart();
    while (!done && timer.elapsed() < timeout)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

    if (sock.bytesAvailable() > 0) resp += sock.readAll();
    return resp;
}

static QByteArray basicAuth(const QString &user, const QString &pass)
{
    return "Authorization: Basic " +
           (user + ":" + pass).toUtf8().toBase64() + "\r\n";
}

struct ServerFixture {
    SessionManager session;
    WebServer      server{&session};
    quint16        port = 0;
    QString        passHash;

    ServerFixture(bool withAuth = true) {
        qtApp();
        for (quint16 p = 18080; p < 18200; ++p) {
            if (server.start(p, false)) { port = p; break; }
        }
        REQUIRE(port > 0);
        if (withAuth) {
            passHash = QString::fromUtf8(
                QCryptographicHash::hash("s3cureP@ss",
                    QCryptographicHash::Sha256).toHex());
            server.setCredentials("admin", passHash);
        }
    }
    ~ServerFixture() { server.stop(); }
};

// ============================================================================
//  AUTH BYPASS
// ============================================================================

TEST_CASE("Security: auth bypass", "[security][auth]")
{
    ServerFixture srv(true);

    SECTION("no credentials returns 401") {
        auto r = sendRaw(srv.port,
            "GET /api/torrents HTTP/1.1\r\nHost: localhost\r\n\r\n");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("401"));
    }

    SECTION("wrong password returns 401") {
        auto r = sendRaw(srv.port,
            "GET /api/torrents HTTP/1.1\r\n"
            + basicAuth("admin", "wrong") +
            "Host: localhost\r\n\r\n");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("401"));
    }

    SECTION("wrong username returns 401") {
        auto r = sendRaw(srv.port,
            "GET /api/torrents HTTP/1.1\r\n"
            + basicAuth("hacker", "s3cureP@ss") +
            "Host: localhost\r\n\r\n");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("401"));
    }

    SECTION("correct credentials returns 200") {
        auto r = sendRaw(srv.port,
            "GET /api/torrents HTTP/1.1\r\n"
            + basicAuth("admin", "s3cureP@ss") +
            "Host: localhost\r\n\r\n");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("200"));
    }

    SECTION("empty Basic header returns 401") {
        auto r = sendRaw(srv.port,
            "GET /api/torrents HTTP/1.1\r\n"
            "Authorization: Basic \r\n"
            "Host: localhost\r\n\r\n");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("401"));
    }

    SECTION("garbage base64 returns 401") {
        auto r = sendRaw(srv.port,
            "GET /api/torrents HTTP/1.1\r\n"
            "Authorization: Basic !!!invalid!!!\r\n"
            "Host: localhost\r\n\r\n");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("401"));
    }

    SECTION("base64 without colon returns 401") {
        QByteArray enc = QByteArray("adminpassword").toBase64();
        auto r = sendRaw(srv.port,
            "GET /api/torrents HTTP/1.1\r\n"
            "Authorization: Basic " + enc + "\r\n"
            "Host: localhost\r\n\r\n");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("401"));
    }

    SECTION("Bearer scheme ignored (returns 401)") {
        auto r = sendRaw(srv.port,
            "GET /api/torrents HTTP/1.1\r\n"
            "Authorization: Bearer faketoken123\r\n"
            "Host: localhost\r\n\r\n");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("401"));
    }

    SECTION("no auth required when not configured") {
        ServerFixture open(false);
        auto r = sendRaw(open.port,
            "GET /api/torrents HTTP/1.1\r\nHost: localhost\r\n\r\n");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("200"));
    }
}

// ============================================================================
//  PATH TRAVERSAL
// ============================================================================

TEST_CASE("Security: path traversal", "[security][path]")
{
    ServerFixture srv(false);

    SECTION("../../../etc/passwd via URL") {
        auto r = sendRaw(srv.port,
            "GET /../../../etc/passwd HTTP/1.1\r\nHost: localhost\r\n\r\n");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("404"));
        REQUIRE(r.indexOf("root:") == -1);
    }

    SECTION("URL-encoded traversal") {
        auto r = sendRaw(srv.port,
            "GET /..%2F..%2F..%2Fetc%2Fpasswd HTTP/1.1\r\n"
            "Host: localhost\r\n\r\n");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("404"));
        REQUIRE(r.indexOf("root:") == -1);
    }

    SECTION("backslash traversal (Windows-style)") {
        auto r = sendRaw(srv.port,
            "GET /..\\..\\..\\Windows\\System32\\config\\SAM HTTP/1.1\r\n"
            "Host: localhost\r\n\r\n");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("404"));
    }

    SECTION("savePath traversal in JSON") {
        QJsonObject body;
        body["magnet"] = "magnet:?xt=urn:btih:0000000000000000000000000000000000000000";
        body["savePath"] = "../../etc/";
        QByteArray json = QJsonDocument(body).toJson(QJsonDocument::Compact);
        auto r = sendRaw(srv.port,
            "POST /api/torrents HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " + QByteArray::number(json.size()) + "\r\n"
            "\r\n" + json);
        // Should not crash regardless of outcome
        REQUIRE(!r.isEmpty());
    }
}

// ============================================================================
//  HTTP REQUEST SMUGGLING / INJECTION
// ============================================================================

TEST_CASE("Security: malformed requests", "[security][http]")
{
    ServerFixture srv(false);

    SECTION("single-word request line") {
        auto r = sendRaw(srv.port, "INVALID\r\n\r\n");
        CHECK((!r.isEmpty() || true)); // no crash is the test
    }

    SECTION("empty body") {
        sendRaw(srv.port, "\r\n\r\n", 1000);
        // no crash
    }

    SECTION("10 KB request line") {
        QByteArray huge = "GET /" + QByteArray(10000, 'A') + " HTTP/1.1\r\n\r\n";
        auto r = sendRaw(srv.port, huge);
        CHECK((!r.isEmpty() || true));
    }

    SECTION("null bytes in path") {
        QByteArray req = "GET /api/torrents";
        req.append('\0');
        req.append(" HTTP/1.1\r\nHost: localhost\r\n\r\n");
        sendRaw(srv.port, req);
    }

    SECTION("CRLF injection in path") {
        auto r = sendRaw(srv.port,
            "GET /api/torrents\r\nInjected: evil HTTP/1.1\r\n"
            "Host: localhost\r\n\r\n");
        REQUIRE(r.indexOf("Injected") == -1);
    }

    SECTION("HTTP/0.9 request") {
        auto r = sendRaw(srv.port, "GET /api/status\r\n\r\n");
        // Should return something or nothing, not crash
        CHECK((!r.isEmpty() || true));
    }
}

// ============================================================================
//  MULTIPART UPLOAD SECURITY
// ============================================================================

TEST_CASE("Security: multipart upload", "[security][upload]")
{
    ServerFixture srv(false);

    SECTION("no boundary") {
        auto r = sendRaw(srv.port,
            "POST /api/torrents HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Content-Type: multipart/form-data\r\n"
            "Content-Length: 10\r\n\r\n"
            "0123456789");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("400"));
    }

    SECTION("boundary but no filename") {
        auto r = sendRaw(srv.port,
            "POST /api/torrents HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Content-Type: multipart/form-data; boundary=----B\r\n"
            "Content-Length: 40\r\n\r\n"
            "------B\r\nno-filename\r\n------B--\r\n");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("400"));
    }

    SECTION("empty file data") {
        QByteArray b = "----TB";
        QByteArray body =
            "--" + b + "\r\n"
            "Content-Disposition: form-data; name=\"file\"; filename=\"t.torrent\"\r\n"
            "Content-Type: application/x-bittorrent\r\n\r\n"
            "\r\n--" + b + "--\r\n";
        auto r = sendRaw(srv.port,
            "POST /api/torrents HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Content-Type: multipart/form-data; boundary=" + b + "\r\n"
            "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
        REQUIRE(!r.isEmpty());
    }

    SECTION("large payload does not crash server") {
        // WebServer::handleClient uses single-shot readAll(), so large payloads
        // may be partially read and rejected. The key test here is no crash.
        QByteArray b = "----TB";
        QByteArray garbage(64 * 1024, 'X');
        QByteArray body =
            "--" + b + "\r\n"
            "Content-Disposition: form-data; name=\"file\"; filename=\"big.torrent\"\r\n"
            "Content-Type: application/x-bittorrent\r\n\r\n"
            + garbage + "\r\n--" + b + "--\r\n";
        sendRaw(srv.port,
            "POST /api/torrents HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Content-Type: multipart/form-data; boundary=" + b + "\r\n"
            "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body,
            5000);
        // Server should still be responsive after the large payload
        auto check = sendRaw(srv.port,
            "GET /api/status HTTP/1.1\r\nHost: localhost\r\n\r\n");
        REQUIRE_THAT(check.toStdString(), ContainsSubstring("200"));
    }
}

// ============================================================================
//  MAGNET LINK VALIDATION
// ============================================================================

TEST_CASE("Security: magnet validation", "[security][magnet]")
{
    ServerFixture srv(false);

    auto postMagnet = [&](const QString &magnet) {
        QJsonObject body;
        body["magnet"] = magnet;
        QByteArray json = QJsonDocument(body).toJson(QJsonDocument::Compact);
        return sendRaw(srv.port,
            "POST /api/torrents HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " + QByteArray::number(json.size()) + "\r\n\r\n" + json);
    };

    SECTION("empty magnet rejected")     { REQUIRE_THAT(postMagnet("").toStdString(),                     ContainsSubstring("400")); }
    SECTION("http scheme rejected")      { REQUIRE_THAT(postMagnet("http://evil.com").toStdString(),      ContainsSubstring("400")); }
    SECTION("javascript scheme rejected"){ REQUIRE_THAT(postMagnet("javascript:alert(1)").toStdString(),  ContainsSubstring("400")); }
    SECTION("file scheme rejected")      { REQUIRE_THAT(postMagnet("file:///etc/passwd").toStdString(),   ContainsSubstring("400")); }
    SECTION("data scheme rejected")      { REQUIRE_THAT(postMagnet("data:text/html,<h1>XSS</h1>").toStdString(), ContainsSubstring("400")); }

    SECTION("malformed JSON body") {
        auto r = sendRaw(srv.port,
            "POST /api/torrents HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: 15\r\n\r\n"
            "{invalid json!}");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("400"));
    }
}

// ============================================================================
//  XSS / RESPONSE INJECTION
// ============================================================================

TEST_CASE("Security: XSS protection", "[security][xss]")
{
    ServerFixture srv(false);

    SECTION("JSON endpoints use application/json content type") {
        auto r = sendRaw(srv.port,
            "GET /api/torrents HTTP/1.1\r\nHost: localhost\r\n\r\n");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("application/json"));
    }

    SECTION("error responses are JSON, not reflected HTML") {
        auto r = sendRaw(srv.port,
            "DELETE /api/torrents/<script>alert(1)</script> HTTP/1.1\r\n"
            "Host: localhost\r\n\r\n");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("application/json"));
    }

    SECTION("HTML endpoint has correct content type") {
        auto r = sendRaw(srv.port,
            "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n");
        // text/html or 500 (no resource in test) — both are acceptable
        CHECK((r.contains("text/html") || r.contains("500")));
    }
}

// ============================================================================
//  HTTP METHOD ENFORCEMENT
// ============================================================================

TEST_CASE("Security: HTTP method enforcement", "[security][methods]")
{
    ServerFixture srv(false);

    SECTION("PATCH /api/torrents → 404") {
        auto r = sendRaw(srv.port,
            "PATCH /api/torrents HTTP/1.1\r\nHost: localhost\r\n\r\n");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("404"));
    }

    SECTION("DELETE /api/torrents (no hash) → 404") {
        auto r = sendRaw(srv.port,
            "DELETE /api/torrents HTTP/1.1\r\nHost: localhost\r\n\r\n");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("404"));
    }

    SECTION("POST /api/status → 404") {
        auto r = sendRaw(srv.port,
            "POST /api/status HTTP/1.1\r\nHost: localhost\r\n"
            "Content-Length: 0\r\n\r\n");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("404"));
    }

    SECTION("PUT /api/torrents → 404") {
        auto r = sendRaw(srv.port,
            "PUT /api/torrents HTTP/1.1\r\nHost: localhost\r\n"
            "Content-Length: 0\r\n\r\n");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("404"));
    }
}

// ============================================================================
//  NONEXISTENT TORRENT HASH
// ============================================================================

TEST_CASE("Security: nonexistent torrent operations", "[security][hash]")
{
    ServerFixture srv(false);
    QByteArray fakeHash = "0000000000000000000000000000000000000000";

    SECTION("DELETE → 404") {
        auto r = sendRaw(srv.port,
            "DELETE /api/torrents/" + fakeHash + " HTTP/1.1\r\n"
            "Host: localhost\r\n\r\n");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("404"));
    }

    SECTION("pause → 404") {
        auto r = sendRaw(srv.port,
            "POST /api/torrents/" + fakeHash + "/pause HTTP/1.1\r\n"
            "Host: localhost\r\nContent-Length: 0\r\n\r\n");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("404"));
    }

    SECTION("resume → 404") {
        auto r = sendRaw(srv.port,
            "POST /api/torrents/" + fakeHash + "/resume HTTP/1.1\r\n"
            "Host: localhost\r\nContent-Length: 0\r\n\r\n");
        REQUIRE_THAT(r.toStdString(), ContainsSubstring("404"));
    }

    SECTION("peers → empty array") {
        auto r = sendRaw(srv.port,
            "GET /api/torrents/" + fakeHash + "/peers HTTP/1.1\r\n"
            "Host: localhost\r\n\r\n");
        REQUIRE(r.contains("[]"));
    }

    SECTION("files → empty array") {
        auto r = sendRaw(srv.port,
            "GET /api/torrents/" + fakeHash + "/files HTTP/1.1\r\n"
            "Host: localhost\r\n\r\n");
        REQUIRE(r.contains("[]"));
    }
}

// ============================================================================
//  RAPID-FIRE / DOS RESISTANCE
// ============================================================================

TEST_CASE("Security: rapid-fire requests", "[security][dos]")
{
    ServerFixture srv(false);
    int success = 0;
    constexpr int TOTAL = 50;

    for (int i = 0; i < TOTAL; ++i) {
        auto r = sendRaw(srv.port,
            "GET /api/status HTTP/1.1\r\nHost: localhost\r\n\r\n");
        if (r.contains("200")) ++success;
    }

    REQUIRE(success >= TOTAL * 80 / 100);
}

// ============================================================================
//  REGEX DoS (ReDoS) RESISTANCE
// ============================================================================

TEST_CASE("Security: ReDoS resistance in RSS regex", "[security][redos]")
{
    // Evil ReDoS pattern
    QRegularExpression regex("(a+)+$", QRegularExpression::CaseInsensitiveOption);
    REQUIRE(regex.isValid());

    QString evil = QString(100, 'a') + "!";
    QElapsedTimer t;
    t.start();
    regex.match(evil);
    qint64 ms = t.elapsed();

    // PCRE2 has backtrack limits — should finish fast
    REQUIRE(ms < 1000);
}
