// SPDX-License-Identifier: MIT
// BATorrent Memory Leak Test Suite (Catch2 v3)
//
// Tests object lifecycle, Qt parent-child cleanup, repeated create/destroy
// cycles, and resource leaks. Works standalone and is enhanced by:
//   - ASan         (cmake -DBAT_ASAN=ON)
//   - CRT Debug    (cmake -DBAT_CRT_DEBUG=ON, Windows only)
//   - Dr. Memory   (cmake --build . --target test_memory_drmemory)

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <QPointer>
#include <QElapsedTimer>

#include "webui/webserver.h"
#include "torrent/sessionmanager.h"
#include "gui/torrentmodel.h"
#include "gui/torrentfilter.h"
#include "app/utils.h"

// ============================================================================
//  CRT Debug Heap (Windows)
// ============================================================================

#ifdef BAT_CRT_DEBUG_HEAP
#  include <crtdbg.h>

struct CrtDebugGuard {
    _CrtMemState snapBefore{};

    CrtDebugGuard() { _CrtMemCheckpoint(&snapBefore); }

    ~CrtDebugGuard() {
        _CrtMemState snapAfter{}, diff{};
        _CrtMemCheckpoint(&snapAfter);
        if (_CrtMemDifference(&diff, &snapBefore, &snapAfter)) {
            _CrtMemDumpStatistics(&diff);
            // Don't FAIL here — ASan/DrMem give better diagnostics.
            // This just dumps to Output window in MSVC debugger.
        }
    }
};
#  define CRT_GUARD() CrtDebugGuard _crt_guard
#else
#  define CRT_GUARD() ((void)0)
#endif

// ============================================================================
//  Qt app singleton
// ============================================================================

static int   s_argc = 1;
static char  s_arg0[] = "test_memory";
static char *s_argv[] = { s_arg0, nullptr };

static QCoreApplication &app() {
    static QCoreApplication a(s_argc, s_argv);
    return a;
}

// Event-loop-safe HTTP request for in-process server tests
static QByteArray sendHttp(quint16 port, const QByteArray &req, int timeout = 3000)
{
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

// ============================================================================
//  WEBSERVER LIFECYCLE
// ============================================================================

TEST_CASE("Memory: WebServer start/stop cycles", "[memory][webserver]")
{
    CRT_GUARD();
    app();
    SessionManager session;
    WebServer server(&session);

    for (int i = 0; i < 100; ++i) {
        REQUIRE(server.start(static_cast<quint16>(18300 + (i % 50)), false));
        REQUIRE(server.isRunning());
        server.stop();
        REQUIRE_FALSE(server.isRunning());
    }
}

TEST_CASE("Memory: WebServer double stop", "[memory][webserver]")
{
    CRT_GUARD();
    app();
    SessionManager session;
    WebServer server(&session);

    REQUIRE(server.start(18350, false));
    server.stop();
    server.stop(); // must not crash or double-free
    REQUIRE_FALSE(server.isRunning());
}

TEST_CASE("Memory: WebServer double start (replaces old)", "[memory][webserver]")
{
    CRT_GUARD();
    app();
    SessionManager session;
    WebServer server(&session);

    REQUIRE(server.start(18351, false));
    REQUIRE(server.start(18352, false)); // old server freed internally
    REQUIRE(server.isRunning());
    server.stop();
}

TEST_CASE("Memory: WebServer destructor frees port", "[memory][webserver]")
{
    CRT_GUARD();
    app();
    SessionManager session;

    {
        WebServer tmp(&session);
        REQUIRE(tmp.start(18353, false));
    } // destructor

    // Port should be free
    WebServer again(&session);
    REQUIRE(again.start(18353, false));
    again.stop();
}

// ============================================================================
//  CLIENT CONNECTION CLEANUP
// ============================================================================

TEST_CASE("Memory: 50 connections cleaned up", "[memory][connections]")
{
    CRT_GUARD();
    app();
    SessionManager session;
    WebServer server(&session);
    REQUIRE(server.start(18360, false));

    for (int i = 0; i < 50; ++i) {
        auto r = sendHttp(18360, "GET /api/status HTTP/1.1\r\nHost: localhost\r\n\r\n", 2000);
        Q_UNUSED(r);
    }

    QCoreApplication::processEvents();
    REQUIRE(server.isRunning());
    server.stop();
}

TEST_CASE("Memory: abrupt disconnect cleanup", "[memory][connections]")
{
    CRT_GUARD();
    app();
    SessionManager session;
    WebServer server(&session);
    REQUIRE(server.start(18361, false));

    for (int i = 0; i < 30; ++i) {
        QTcpSocket sock;
        sock.connectToHost("127.0.0.1", 18361);
        QElapsedTimer t; t.start();
        while (sock.state() != QAbstractSocket::ConnectedState && t.elapsed() < 500)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        if (sock.state() == QAbstractSocket::ConnectedState) {
            sock.write("GET /api/sta"); // partial request
            sock.flush();
            QCoreApplication::processEvents();
            sock.abort(); // hard disconnect
        }
        QCoreApplication::processEvents();
    }

    QCoreApplication::processEvents();
    REQUIRE(server.isRunning());
    server.stop();
}

// ============================================================================
//  TORRENT MODEL MEMORY
// ============================================================================

TEST_CASE("Memory: TorrentModel refresh x1000", "[memory][model]")
{
    CRT_GUARD();
    app();
    SessionManager session;
    TorrentModel model(&session);

    for (int i = 0; i < 1000; ++i)
        model.refresh();

    REQUIRE(model.rowCount() == 0);
}

TEST_CASE("Memory: TorrentFilter cycle x500", "[memory][model]")
{
    CRT_GUARD();
    app();
    SessionManager session;
    TorrentModel model(&session);
    TorrentFilter filter;
    filter.setSourceModel(&model);

    QStringList states = {"", "__active__", "Downloading", "Seeding", "Paused", ""};
    for (int i = 0; i < 500; ++i) {
        filter.setStateFilter(states[i % states.size()]);
        filter.setNameFilter(i % 2 == 0 ? "test" : "");
        model.refresh();
    }

    REQUIRE(filter.rowCount() == 0);
}

TEST_CASE("Memory: filter create/destroy x100", "[memory][model]")
{
    CRT_GUARD();
    app();
    SessionManager session;
    TorrentModel model(&session);

    for (int i = 0; i < 100; ++i) {
        auto *f = new TorrentFilter;
        f->setSourceModel(&model);
        f->setStateFilter("Downloading");
        f->setNameFilter("test");
        delete f;
    }
}

// ============================================================================
//  QT PARENT-CHILD OWNERSHIP
// ============================================================================

TEST_CASE("Memory: parent delete cleans up WebServer + SessionManager", "[memory][ownership]")
{
    CRT_GUARD();
    app();
    auto *parent = new QObject;
    auto *session = new SessionManager(parent);
    auto *server  = new WebServer(session, parent);

    QPointer<WebServer>      pSrv(server);
    QPointer<SessionManager> pSes(session);

    REQUIRE_FALSE(pSrv.isNull());
    REQUIRE_FALSE(pSes.isNull());

    delete parent;

    REQUIRE(pSrv.isNull());
    REQUIRE(pSes.isNull());
}

TEST_CASE("Memory: parent delete cleans up Model + Filter", "[memory][ownership]")
{
    CRT_GUARD();
    app();
    auto *parent  = new QObject;
    auto *session = new SessionManager(parent);
    auto *model   = new TorrentModel(session, parent);
    auto *filter  = new TorrentFilter(parent);
    filter->setSourceModel(model);

    QPointer<TorrentModel>  pM(model);
    QPointer<TorrentFilter> pF(filter);

    delete parent;

    REQUIRE(pM.isNull());
    REQUIRE(pF.isNull());
}

// ============================================================================
//  SETTINGS CHURN (string alloc/dealloc)
// ============================================================================

TEST_CASE("Memory: SessionManager settings churn", "[memory][settings]")
{
    CRT_GUARD();
    app();
    SessionManager session;

    for (int i = 0; i < 100; ++i) {
        session.setDownloadLimit(i * 10);
        session.setUploadLimit(i * 5);
        session.setMaxConnections(50 + i);
        session.setDhtEnabled(i % 2 == 0);
        session.setEncryptionMode(i % 3);
        session.setSeedRatioLimit(static_cast<float>(i) / 10.0f);
    }

    REQUIRE(session.encryptionMode() == (99 % 3));
}

TEST_CASE("Memory: proxy settings churn", "[memory][settings]")
{
    CRT_GUARD();
    app();
    SessionManager session;

    for (int i = 0; i < 50; ++i) {
        session.setProxySettings(1, "proxy.example.com", 1080 + i, "user", "pass");
        session.setProxySettings(0, "", 0, "", "");
    }

    REQUIRE(session.proxyType() == 0);
}

TEST_CASE("Memory: scheduler toggle churn", "[memory][settings]")
{
    CRT_GUARD();
    app();
    SessionManager session;
    session.setAltSpeedLimits(100, 50);

    for (int i = 0; i < 200; ++i) {
        session.setSchedulerEnabled(true);
        session.setScheduleFromHour(i % 24);
        session.setScheduleToHour((i + 6) % 24);
        session.setScheduleDays(i % 128);
        session.setSchedulerEnabled(false);
    }

    REQUIRE_FALSE(session.schedulerEnabled());
}

// ============================================================================
//  IP FILTER MEMORY
// ============================================================================

TEST_CASE("Memory: IP filter clear cycles", "[memory][ipfilter]")
{
    CRT_GUARD();
    app();
    SessionManager session;

    for (int i = 0; i < 50; ++i)
        session.clearIpFilter();

    REQUIRE(session.ipFilterCount() == 0);
}

// ============================================================================
//  MIME DATA LIFECYCLE
// ============================================================================

TEST_CASE("Memory: mimeData create/delete x100", "[memory][mime]")
{
    CRT_GUARD();
    app();
    SessionManager session;
    TorrentModel model(&session);

    for (int i = 0; i < 100; ++i) {
        QModelIndexList list;
        QMimeData *m = model.mimeData(list);
        delete m;
    }
}

// ============================================================================
//  FORMAT FUNCTIONS (high-volume string alloc)
// ============================================================================

TEST_CASE("Memory: formatSize/formatSpeed x100k calls", "[memory][utils]")
{
    CRT_GUARD();

    for (int i = 0; i < 100'000; ++i) {
        volatile auto s1 = formatSize(static_cast<qint64>(i) * 1024);
        volatile auto s2 = formatSpeed(i * 100);
        Q_UNUSED(s1);
        Q_UNUSED(s2);
    }
}

// ============================================================================
//  STRESS: many connections + server alive
// ============================================================================

TEST_CASE("Memory: 200 connections stress", "[memory][stress]")
{
    CRT_GUARD();
    app();
    SessionManager session;
    WebServer server(&session);
    REQUIRE(server.start(18370, false));

    QElapsedTimer timer;
    timer.start();
    int success = 0;

    for (int i = 0; i < 200; ++i) {
        auto r = sendHttp(18370,
            "GET /api/status HTTP/1.1\r\nHost: localhost\r\n\r\n", 2000);
        if (r.contains("200")) ++success;
    }

    INFO("Stress: " << success << "/200 in " << timer.elapsed() << " ms");
    REQUIRE(server.isRunning());
    REQUIRE(success >= 140); // ≥70%
    server.stop();
}
