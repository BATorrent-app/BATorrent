#include <catch2/catch_test_macros.hpp>
#include "services/discovery/addonfinish.h"

#include <QByteArray>
#include <QList>

namespace {

QByteArray catalogBody(const char *id, const char *name)
{
    return QByteArrayLiteral(R"({ "metas": [ { "id": ")")
           + QByteArray(id)
           + QByteArrayLiteral(R"(", "type": "movie", "name": ")")
           + QByteArray(name)
           + QByteArrayLiteral(R"(", "poster": "p", "releaseInfo": "2020" } ] })");
}

QByteArray streamBody(const char *hash)
{
    return QByteArrayLiteral(R"({
      "streams": [
        { "infoHash": ")")
           + QByteArray(hash)
           + QByteArrayLiteral(R"(", "title": "1080p",
          "behaviorHints": { "videoSize": 100 } }
      ]
    })");
}

} // namespace

TEST_CASE("applyCatalogReply success parses and finishes", "[addonfinish]") {
    QList<CatalogItem> results;
    int pending = 1;
    const auto out = AddonFinish::applyCatalogReply(
        /*activeGen=*/1, pending, results, /*replyGen=*/1, /*ok=*/true,
        catalogBody("tt1", "One"));
    REQUIRE(out == AddonFinish::ReplyOutcome::Finished);
    REQUIRE(pending == 0);
    REQUIRE(results.size() == 1);
    REQUIRE(results.at(0).id == QLatin1String("tt1"));
}

TEST_CASE("applyCatalogReply error body still progresses pending", "[addonfinish]") {
    QList<CatalogItem> results;
    int pending = 2;
    auto out = AddonFinish::applyCatalogReply(
        1, pending, results, 1, /*ok=*/false, catalogBody("tt1", "One"));
    REQUIRE(out == AddonFinish::ReplyOutcome::Progress);
    REQUIRE(pending == 1);
    REQUIRE(results.isEmpty());

    out = AddonFinish::applyCatalogReply(
        1, pending, results, 1, /*ok=*/true, QByteArrayLiteral("{ \"metas\": [] }"));
    REQUIRE(out == AddonFinish::ReplyOutcome::Finished);
    REQUIRE(results.isEmpty());
}

TEST_CASE("applyCatalogReply dedupes by id across replies", "[addonfinish]") {
    QList<CatalogItem> results;
    int pending = 2;
    REQUIRE(AddonFinish::applyCatalogReply(
                1, pending, results, 1, true, catalogBody("tt1", "One"))
            == AddonFinish::ReplyOutcome::Progress);
    REQUIRE(AddonFinish::applyCatalogReply(
                1, pending, results, 1, true, catalogBody("tt1", "One Dup"))
            == AddonFinish::ReplyOutcome::Finished);
    REQUIRE(results.size() == 1);
    REQUIRE(results.at(0).name == QLatin1String("One"));
}

TEST_CASE("applyCatalogReply stale gen drops without mutating", "[addonfinish]") {
    QList<CatalogItem> results;
    int pending = 1;
    const auto stale = AddonFinish::applyCatalogReply(
        /*activeGen=*/2, pending, results, /*replyGen=*/1, /*ok=*/true,
        catalogBody("tt9", "Stale"));
    REQUIRE(stale == AddonFinish::ReplyOutcome::Stale);
    REQUIRE(pending == 1);
    REQUIRE(results.isEmpty());
}

TEST_CASE("applyStreamReply success and stale gen race", "[addonfinish]") {
    QList<StreamResult> results;
    int pending = 1;

    REQUIRE(AddonFinish::applyStreamReply(
                1, pending, results, 1, true,
                streamBody("0123456789abcdef0123456789abcdef01234567"),
                QStringLiteral("Torrentio"))
            == AddonFinish::ReplyOutcome::Finished);
    REQUIRE(results.size() == 1);
    REQUIRE(results.at(0).addonName == QLatin1String("Torrentio"));

    pending = 1;
    results.clear();
    REQUIRE(AddonFinish::applyStreamReply(
                /*activeGen=*/5, pending, results, /*replyGen=*/4, true,
                streamBody("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"),
                QStringLiteral("Old"))
            == AddonFinish::ReplyOutcome::Stale);
    REQUIRE(pending == 1);
    REQUIRE(results.isEmpty());
}

TEST_CASE("applyStreamReply error body finishes when last pending", "[addonfinish]") {
    QList<StreamResult> results;
    int pending = 1;
    REQUIRE(AddonFinish::applyStreamReply(
                1, pending, results, 1, /*ok=*/false,
                streamBody("0123456789abcdef0123456789abcdef01234567"),
                QStringLiteral("X"))
            == AddonFinish::ReplyOutcome::Finished);
    REQUIRE(results.isEmpty());
}
