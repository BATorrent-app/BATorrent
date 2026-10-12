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
