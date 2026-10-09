// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include <catch2/catch_test_macros.hpp>

#include "services/integrations/updater.h"

#include <QJsonDocument>

namespace {

QJsonDocument parse(const char *json)
{
    return QJsonDocument::fromJson(QByteArray(json));
}

QString tagOf(const QJsonObject &o)
{
    return o.value(QStringLiteral("tag_name")).toString();
}

} // namespace

TEST_CASE("the stable endpoint answers with one release", "[updatetrack]")
{
    const auto doc = parse(R"({"tag_name":"v4.9.0","prerelease":false})");
    REQUIRE(tagOf(Updater::pickRelease(doc, false)) == "v4.9.0");
}

TEST_CASE("a prerelease is only offered on the beta track", "[updatetrack]")
{
    const auto doc = parse(R"([
        {"tag_name":"v4.10.0-beta1","prerelease":true},
        {"tag_name":"v4.9.0","prerelease":false}
    ])");
    REQUIRE(tagOf(Updater::pickRelease(doc, true))  == "v4.10.0-beta1");
    REQUIRE(tagOf(Updater::pickRelease(doc, false)) == "v4.9.0");
}

TEST_CASE("the highest version wins, not the first entry", "[updatetrack]")
{
    // Gitee does not promise an order, and a beta cut from an older branch
    // must never be handed out as an upgrade just for sitting at the top.
    const auto doc = parse(R"([
        {"tag_name":"v4.8.1-beta2","prerelease":true},
        {"tag_name":"v4.9.0","prerelease":false},
        {"tag_name":"v4.7.0","prerelease":false}
    ])");
    REQUIRE(tagOf(Updater::pickRelease(doc, true)) == "v4.9.0");
}

TEST_CASE("4.10 is newer than 4.9, not older", "[updatetrack]")
{
    const auto doc = parse(R"([
        {"tag_name":"v4.9.0","prerelease":false},
        {"tag_name":"v4.10.0","prerelease":false}
    ])");
    REQUIRE(tagOf(Updater::pickRelease(doc, true)) == "v4.10.0");
}

TEST_CASE("drafts are never offered, on either track", "[updatetrack]")
{
    const auto doc = parse(R"([
        {"tag_name":"v5.0.0","draft":true,"prerelease":false},
        {"tag_name":"v4.9.0","prerelease":false}
    ])");
    REQUIRE(tagOf(Updater::pickRelease(doc, true))  == "v4.9.0");
    REQUIRE(tagOf(Updater::pickRelease(doc, false)) == "v4.9.0");
}

TEST_CASE("nothing qualifying yields nothing", "[updatetrack]")
{
    SECTION("a list of only prereleases, on the stable track") {
        const auto doc = parse(R"([{"tag_name":"v5.0.0-rc1","prerelease":true}])");
        REQUIRE(Updater::pickRelease(doc, false).isEmpty());
    }
    SECTION("an empty list") {
        REQUIRE(Updater::pickRelease(parse("[]"), true).isEmpty());
    }
    SECTION("a malformed body") {
        REQUIRE(Updater::pickRelease(parse("not json"), true).isEmpty());
    }
    SECTION("an object with no tag") {
        REQUIRE(Updater::pickRelease(parse(R"({"message":"Not Found"})"), true).isEmpty());
    }
}

// The way off the beta track. Someone running 4.10.0-beta1 has to be offered
// 4.10.0 when it ships, or they sit on a prerelease for ever: the two differ
// only by the suffix, and an int-split would call them equal.
TEST_CASE("a final release supersedes its own prerelease", "[updatetrack]")
{
    REQUIRE(Updater::compareVersions("4.10.0", "4.10.0-beta1") > 0);
    REQUIRE(Updater::compareVersions("4.10.0-beta1", "4.10.0") < 0);
    REQUIRE(Updater::compareVersions("4.10.0-beta2", "4.10.0-beta1") > 0);

    const auto doc = QJsonDocument::fromJson(QByteArray(R"([
        {"tag_name":"v4.10.0-beta1","prerelease":true},
        {"tag_name":"v4.10.0","prerelease":false}
    ])"));
    REQUIRE(tagOf(Updater::pickRelease(doc, true)) == "v4.10.0");
}
