#include <catch2/catch_test_macros.hpp>
#include "services/games/achievementwatch.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

namespace {

void write(const QString &path, const QByteArray &bytes)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    REQUIRE(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(bytes);
}

struct Spy {
    QList<QStringList> calls;
    QString key;
};

}

TEST_CASE("what was unlocked before the game started is not news", "[achievementwatch]") {
    QTemporaryDir root;
    AchievementFiles::Roots r;
    r.appData = root.filePath("appdata");
    const QString file = r.appData + "/GSE Saves/10/achievements.json";
    write(file, R"({"OLD":{"earned":true,"earned_time":1}})");

    AchievementWatch w(r);
    Spy spy;
    QObject::connect(&w, &AchievementWatch::unlocked,
                     [&spy](const QString &key, int, const QString &, const QStringList &names) {
        spy.key = key; spy.calls << names;
    });
    w.start("game", 10, root.filePath("game"));
    REQUIRE(w.watching("game"));
    w.checkNow();
    REQUIRE(spy.calls.isEmpty());

    write(file, R"({"OLD":{"earned":true,"earned_time":1},"NEW":{"earned":true,"earned_time":2}})");
    w.checkNow();
    REQUIRE(spy.calls.size() == 1);
    REQUIRE(spy.key == QLatin1String("game"));
    REQUIRE(spy.calls[0] == QStringList{ "NEW" });

    w.checkNow();
    REQUIRE(spy.calls.size() == 1);   // said once, not on every check
}

TEST_CASE("the unlock that lands as the game quits is still caught", "[achievementwatch]") {
    QTemporaryDir root;
    AchievementFiles::Roots r;
    r.appData = root.filePath("appdata");
    AchievementWatch w(r);
    QStringList got;
    QObject::connect(&w, &AchievementWatch::unlocked,
                     [&got](const QString &, int, const QString &, const QStringList &names) { got += names; });
    w.start("g", 20, QString());
    write(r.appData + "/Goldberg SteamEmu Saves/20/achievements.json", R"({"LAST":{"earned":true,"earned_time":9}})");
    w.stop("g");
    REQUIRE(got == QStringList{ "LAST" });
    REQUIRE_FALSE(w.watching("g"));
}

TEST_CASE("no appid, nothing to watch", "[achievementwatch]") {
    AchievementWatch w(AchievementFiles::Roots{});
    w.start("g", 0, QString());
    REQUIRE_FALSE(w.watching("g"));
    w.stop("never-started");   // harmless
}
