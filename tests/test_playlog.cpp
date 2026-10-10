#include <catch2/catch_test_macros.hpp>
#include "services/games/playlog.h"

#include <QDateTime>
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
}

TEST_CASE("daily seconds bucket sessions by the local day they started", "[playlog]") {
    QTemporaryDir dir;
    PlayLog log(dir.filePath("playlog.json"));
    const QDateTime now(QDate(2026, 10, 10), QTime(20, 0));
    const auto at = [](int y, int m, int d, int h) {
        return QDateTime(QDate(y, m, d), QTime(h, 0)).toMSecsSinceEpoch();
    };
    log.recordSession("h", at(2026, 10, 10, 9), 600);    // today
    log.recordSession("h", at(2026, 10, 10, 18), 300);   // today again
    log.recordSession("h", at(2026, 10, 9, 23), 7200);   // yesterday, ran past midnight
    log.recordSession("h", at(2026, 9, 27, 12), 120);    // 13 days back: the first bar
    log.recordSession("h", at(2026, 9, 26, 12), 999);    // 14 days back: outside the window

    const auto days = log.dailySecs("h", 14, now.toMSecsSinceEpoch());
    REQUIRE(days.size() == 14);
    REQUIRE(days.at(13) == 900);
    REQUIRE(days.at(12) == 7200);
    REQUIRE(days.at(0) == 120);
    qint64 sum = 0;
    for (qint64 d : days) sum += d;
    REQUIRE(sum == 900 + 7200 + 120);

    REQUIRE(log.dailySecs("unknown", 14, now.toMSecsSinceEpoch()) == QList<qint64>(14, 0));
    REQUIRE(log.dailySecs("h", -3, now.toMSecsSinceEpoch()).isEmpty());
    REQUIRE(log.dailySecs("h", 1 << 30, now.toMSecsSinceEpoch()).size() == 366);
}

TEST_CASE("sessions past the cap fold into the total instead of vanishing", "[playlog]") {
    QTemporaryDir dir;
    PlayLog log(dir.filePath("playlog.json"));
    for (int i = 1; i <= PlayLog::kMaxSessions + 5; ++i) log.recordSession("h", i, 60);
    REQUIRE(log.summary("h").totalSecs == qint64(PlayLog::kMaxSessions + 5) * 60);
    REQUIRE(log.save());
    REQUIRE(PlayLog(dir.filePath("playlog.json")).summary("h").totalSecs
            == qint64(PlayLog::kMaxSessions + 5) * 60);
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

    {
        QFile f(path);
        REQUIRE(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        f.write("{not json");
    }
    PlayLog broken(path);
    REQUIRE(broken.summary("h").totalSecs == 0);
}
