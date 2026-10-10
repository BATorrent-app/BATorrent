#include <catch2/catch_test_macros.hpp>
#include "services/games/achievementfiles.h"
#include "services/games/achievementformats.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

using AchievementFormats::Format;
using AchievementFormats::parse;

namespace {

void write(const QString &path, const QByteArray &bytes)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    REQUIRE(f.open(QIODevice::WriteOnly));
    f.write(bytes);
}

}

TEST_CASE("Goldberg and gbe write an object keyed by id, EMPRESS sometimes an array", "[achievementfiles]") {
    const auto obj = parse(Format::GoldbergJson, R"({"NIGHT":{"earned":true,"earned_time":1700000000},
        "LOCKED":{"earned":false,"earned_time":0},"PROG":{"earned":true,"earned_time":0,"progress":5,"max_progress":5}})");
    REQUIRE(obj.size() == 2);
    REQUIRE(obj.value("NIGHT") == 1700000000);
    REQUIRE(obj.value("PROG") == 0);
    const auto arr = parse(Format::GoldbergJson, R"([{"name":"A","earned":1,"earned_time":1600000000},{"name":"B","earned":0}])");
    REQUIRE(arr.size() == 1);
    REQUIRE(arr.value("A") == 1600000000);
    REQUIRE(parse(Format::GoldbergJson, "{broken").isEmpty());
}

TEST_CASE("CODEX and RUNE keep a section per achievement, ALI213 the same with other keys", "[achievementfiles]") {
    const auto codex = parse(Format::CodexIni, "\xEF\xBB\xBF; comment\n[NIGHT]\nAchieved=1\nUnlockTime=1700000000\n"
                                                "[LOCKED]\nAchieved=0\n[GEARS]\nAchieved=0\nCurProgress=10\nMaxProgress=10\n"
                                                "[HALF]\nCurProgress=3\nMaxProgress=10\n");
    REQUIRE(codex.size() == 2);
    REQUIRE(codex.value("NIGHT") == 1700000000);
    REQUIRE(codex.contains("GEARS"));   // at its maximum without the flag still counts
    const auto ali = parse(Format::CodexIni, "[A]\r\nHaveAchieved=1\r\nHaveAchievedTime=1650000000\r\n");
    REQUIRE(ali.value("A") == 1650000000);
}

TEST_CASE("OnlineFix, CreamAPI and SKIDROW each spell it their own way", "[achievementfiles]") {
    const auto ofx = parse(Format::OnlineFixIni, "[A]\nachieved=true\ntimestamp=1700000000\n[B]\nAchieved=true\nTimeUnlocked=1700000001000\n[C]\nachieved=false\n");
    REQUIRE(ofx.size() == 2);
    REQUIRE(ofx.value("B") == 1700000001);   // milliseconds come back as seconds
    const auto cream = parse(Format::CreamApiIni, "[X]\nachieved=true\nunlocktime=1600000000\n");
    REQUIRE(cream.value("X") == 1600000000);
    const auto sk = parse(Format::SkidrowIni, "[Achievements]\nA=1@0@0@1700000000\nB=0@0@0@0\n");
    REQUIRE(sk.size() == 1);
    REQUIRE(sk.value("A") == 1700000000);
}

TEST_CASE("3DM and RLD! store hex little-endian numbers", "[achievementfiles]") {
    // 0x6553F100 = 1700000000, little-endian bytes 00 F1 53 65
    const auto tdm = parse(Format::ThreeDmIni, "[State]\nA=0101\nB=0000\n[Time]\nA=00F15365\n");
    REQUIRE(tdm.size() == 1);
    REQUIRE(tdm.value("A") == 1700000000);
    const auto rld = parse(Format::RldIni, "[Steam]\nState=01000000\n[A]\nState=01000000\nTime=00F15365\n[B]\nState=00000000\n");
    REQUIRE(rld.size() == 1);
    REQUIRE(rld.value("A") == 1700000000);
    REQUIRE(parse(Format::RldIni, "[A]\nState=zz\n").isEmpty());
}

TEST_CASE("system paths are built per appid and skip roots this machine lacks", "[achievementfiles]") {
    AchievementFiles::Roots r;
    r.appData = "/R";
    const auto c = AchievementFiles::systemCandidates(1245620, r);
    bool goldberg = false;
    for (const auto &x : c) {
        REQUIRE(x.path.startsWith("/R/"));
        if (x.path == "/R/Goldberg SteamEmu Saves/1245620/achievements.json") goldberg = true;
    }
    REQUIRE(goldberg);
    REQUIRE(AchievementFiles::systemCandidates(0, r).isEmpty());
}

TEST_CASE("unlocks are gathered from every place and the earliest time wins", "[achievementfiles]") {
    QTemporaryDir root;
    AchievementFiles::Roots r;
    r.appData = root.filePath("appdata");
    r.publicDocs = root.filePath("public");
    write(r.appData + "/GSE Saves/42/achievements.json", R"({"A":{"earned":true,"earned_time":2000},"B":{"earned":true,"earned_time":0}})");
    write(r.publicDocs + "/Steam/CODEX/42/achievements.ini", "[A]\nAchieved=1\nUnlockTime=1000\n[B]\nAchieved=1\nUnlockTime=3000\n");
    const QString game = root.filePath("game");
    write(game + "/bin/steam_api64.dll", "x");
    write(game + "/bin/steam_settings/42/achievements.json", R"({"C":{"earned":true,"earned_time":5000}})");
    write(game + "/bin/3DMGAME/Player/stats/achievements.ini", "[State]\nD=0101\n[Time]\nD=00000000\n");

    const auto u = AchievementFiles::readUnlocks(42, game, r);
    REQUIRE(u.size() == 4);
    REQUIRE(u.value("A") == 1000);
    REQUIRE(u.value("B") == 3000);   // a known time beats an unknown one
    REQUIRE(u.value("C") == 5000);
    REQUIRE(u.contains("D"));
    REQUIRE(AchievementFiles::readUnlocks(7, root.filePath("nowhere"), {}).isEmpty());
}

TEST_CASE("gbe's local_save_path is followed relative to the game", "[achievementfiles]") {
    QTemporaryDir root;
    const QString game = root.filePath("game");
    write(game + "/steam_settings/configs.user.ini", "[user::saves]\nlocal_save_path=./saves\n");
    write(game + "/saves/99/achievements.json", R"({"L":{"earned":true,"earned_time":1}})");
    REQUIRE(AchievementFiles::readUnlocks(99, game, {}).value("L") == 1);
}

TEST_CASE("the appid comes from steam_appid.txt, else from an emulator ini", "[achievementfiles]") {
    QTemporaryDir root;
    const QString a = root.filePath("a");
    write(a + "/Game/Binaries/Win64/steam_appid.txt", "1245620\r\n");
    REQUIRE(AchievementFiles::appIdFromGameDir(a) == 1245620);

    const QString b = root.filePath("b");
    write(b + "/steam_api64.dll", "x");
    write(b + "/steam_emu.ini", "[Settings]\nAppId=292030\n");
    REQUIRE(AchievementFiles::appIdFromGameDir(b) == 292030);

    const QString c = root.filePath("c");
    write(c + "/OnlineFix.ini", "[Main]\nRealAppId=1091500\n");
    REQUIRE(AchievementFiles::appIdFromGameDir(c) == 1091500);

    const QString d = root.filePath("d");
    write(d + "/steam_appid.txt", "not a number");
    REQUIRE(AchievementFiles::appIdFromGameDir(d) == 0);
    REQUIRE(AchievementFiles::appIdFromGameDir(root.filePath("missing")) == 0);
}
