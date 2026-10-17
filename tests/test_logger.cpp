// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include <catch2/catch_test_macros.hpp>

#include "services/platform/logger.h"

TEST_CASE("a session with no close marker after its opening was cut short", "[logger]")
{
    const QByteArray open = "T [INFO] --- log opened ---\n";
    const QByteArray closed = "T [INFO] --- log closed ---\n";
    const QByteArray line = "T [INFO] [session] saving resume data\n";
    REQUIRE(Logger::sessionLeftOpen(open + line));                      // killed mid-session
    REQUIRE_FALSE(Logger::sessionLeftOpen(open + line + closed));       // clean exit
    // Shutdown lines after the marker (the engine child exits after the app)
    // used to push it out of the last 200 bytes and read as a crash.
    REQUIRE_FALSE(Logger::sessionLeftOpen(open + closed + line + line + line + line + line));
    // Only the last session counts: an old crash followed by a clean run is clean.
    REQUIRE_FALSE(Logger::sessionLeftOpen(open + line + open + closed));
    REQUIRE(Logger::sessionLeftOpen(open + closed + open + line));
    REQUIRE_FALSE(Logger::sessionLeftOpen(QByteArray()));               // first run, empty log
}

#include "services/platform/appendfile.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <thread>

// The app and the engine write one log through their own handles. Each used to
// write at the end it last saw, so one overwrote the other's lines. Two handles
// on two threads stand in for the two processes.
TEST_CASE("two writers appending to one file lose no line", "[logger]")
{
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString path = dir.filePath("shared.log");
    AppendFile a, b;
    REQUIRE(a.open(path));
    REQUIRE(b.open(path));
    const int n = 2000;
    auto writer = [n](AppendFile *f, char who) {
        for (int i = 0; i < n; ++i)
            f->append(QByteArray(1, who) + " line " + QByteArray::number(i) + " ............................\n");
    };
    std::thread ta(writer, &a, 'A');
    std::thread tb(writer, &b, 'B');
    ta.join();
    tb.join();
    a.close();
    b.close();

    QFile f(path);
    REQUIRE(f.open(QIODevice::ReadOnly));
    const QList<QByteArray> lines = f.readAll().split('\n');
    int countA = 0, countB = 0, broken = 0;
    for (const QByteArray &l : lines) {
        if (l.isEmpty()) continue;
        if (!l.endsWith("............................")) { ++broken; continue; }
        if (l.startsWith("A line ")) ++countA;
        else if (l.startsWith("B line ")) ++countB;
        else ++broken;
    }
    REQUIRE(broken == 0);
    REQUIRE(countA == n);
    REQUIRE(countB == n);
}

// Rotation renames the file under a writer that still has it open; the writer
// can tell (its file is no longer the one at the path) and reopen.
TEST_CASE("an append file survives being renamed under it", "[logger]")
{
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString path = dir.filePath("batorrent.log");
    AppendFile w;
    REQUIRE(w.open(path));
    REQUIRE(w.append("before\n"));
    REQUIRE(QDir(dir.path()).rename("batorrent.log", "batorrent.log.1"));
    REQUIRE_FALSE(QFile::exists(path));
    REQUIRE(w.append("still to the old one\n"));
    REQUIRE(w.open(path));
    REQUIRE(w.append("after\n"));
    w.close();
    QFile cur(path);
    REQUIRE(cur.open(QIODevice::ReadOnly));
    REQUIRE(cur.readAll() == "after\n");
}
