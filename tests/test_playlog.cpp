#include <catch2/catch_test_macros.hpp>
#include "services/games/playlog.h"

#include <QFile>
#include <QTemporaryDir>

TEST_CASE("a launch counts and stamps last played, a session credits time", "[playlog]") {
    QTemporaryDir dir;
    PlayLog log(dir.filePath("playlog.json"));
    REQUIRE(log.summary("h").launches == 0);

    log.recordLaunch("h", 1000);
    log.recordLaunch("h", 5000);
    REQUIRE(log.summary("h").launches == 2);
    REQUIRE(log.summary("h").lastPlayedMs == 5000);
    REQUIRE(log.summary("h").totalSecs == 0);

    REQUIRE(log.recordSession("h", 5000, 3600));
    REQUIRE(log.summary("h").totalSecs == 3600);
}

TEST_CASE("sessions of 30s or less are a launcher handing off, not play", "[playlog]") {
    QTemporaryDir dir;
    PlayLog log(dir.filePath("playlog.json"));
    REQUIRE_FALSE(log.recordSession("h", 1000, 30));
    REQUIRE_FALSE(log.recordSession("h", 0, 600));
    REQUIRE(log.recordSession("h", 1000, 31));
    REQUIRE(log.summary("h").totalSecs == 31);
    REQUIRE(log.recentSessions("h", 10).size() == 1);
}

TEST_CASE("recent sessions come newest first and respect the limit", "[playlog]") {
    QTemporaryDir dir;
    PlayLog log(dir.filePath("playlog.json"));
    for (int i = 1; i <= 5; ++i) log.recordSession("h", i * 1000, 100 * i);
    const auto recent = log.recentSessions("h", 3);
    REQUIRE(recent.size() == 3);
    REQUIRE(recent.at(0).startMs == 5000);
    REQUIRE(recent.at(2).startMs == 3000);
    REQUIRE(log.recentSessions("unknown", 3).isEmpty());
}

TEST_CASE("sessions past the cap fold into the total instead of vanishing", "[playlog]") {
    QTemporaryDir dir;
    PlayLog log(dir.filePath("playlog.json"));
    for (int i = 1; i <= PlayLog::kMaxSessions + 5; ++i) log.recordSession("h", i, 60);
    REQUIRE(log.recentSessions("h", 1000).size() == PlayLog::kMaxSessions);
    REQUIRE(log.summary("h").totalSecs == qint64(PlayLog::kMaxSessions + 5) * 60);
}

TEST_CASE("the log survives a reload", "[playlog]") {
    QTemporaryDir dir;
    const QString path = dir.filePath("sub/playlog.json");
    {
        PlayLog log(path);
        log.recordLaunch("a", 9000);
        log.recordSession("a", 9000, 120);
        log.importTotals("b", 7200, 4000, 3);
        REQUIRE(log.save());
    }
    PlayLog again(path);
    REQUIRE(again.summary("a").totalSecs == 120);
    REQUIRE(again.summary("a").lastPlayedMs == 9000);
    REQUIRE(again.summary("a").launches == 1);
    REQUIRE(again.recentSessions("a", 5).at(0).secs == 120);
    REQUIRE(again.summary("b").totalSecs == 7200);
    REQUIRE(again.summary("b").launches == 3);
}

TEST_CASE("imported totals never overwrite a game the log already has", "[playlog]") {
    QTemporaryDir dir;
    PlayLog log(dir.filePath("playlog.json"));
    log.recordSession("h", 1000, 600);
    log.importTotals("h", 99999, 1, 50);
    REQUIRE(log.summary("h").totalSecs == 600);
    log.importTotals("empty", 0, 0, 0);
    REQUIRE_FALSE(log.contains("empty"));
}

TEST_CASE("a corrupt or hostile file loads as empty or skips the bad rows", "[playlog]") {
    QTemporaryDir dir;
    const QString path = dir.filePath("playlog.json");
    {
        QFile f(path);
        REQUIRE(f.open(QIODevice::WriteOnly));
        f.write(R"({"v":1,"games":{"h":{"launches":-4,"foldedSecs":"x",
                   "sessions":[[1000,60],[5],"no",[2000,-9],[3000,90]]}}})");
    }
    PlayLog log(path);
    REQUIRE(log.summary("h").launches == 0);
    REQUIRE(log.summary("h").totalSecs == 150);
    REQUIRE(log.recentSessions("h", 10).size() == 2);

    {
        QFile f(path);
        REQUIRE(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        f.write("{not json");
    }
    PlayLog broken(path);
    REQUIRE(broken.summary("h").totalSecs == 0);
}
