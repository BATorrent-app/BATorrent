#include <catch2/catch_test_macros.hpp>
#include "services/integrations/gameexedetect.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

TEST_CASE("dataComplete: progress or manual flag", "[gameexedetect]") {
    REQUIRE(GameExeDetect::dataComplete(true, 0.1f));
    REQUIRE(GameExeDetect::dataComplete(false, 1.0f));
    REQUIRE_FALSE(GameExeDetect::dataComplete(false, 0.99f));
}

TEST_CASE("looksLikeGameFromFiles: exe without video", "[gameexedetect]") {
    REQUIRE(GameExeDetect::looksLikeGameFromFiles(true, false));
    REQUIRE_FALSE(GameExeDetect::looksLikeGameFromFiles(true, true));
    REQUIRE_FALSE(GameExeDetect::looksLikeGameFromFiles(false, false));
}

TEST_CASE("scoreCandidate skips redists and marks installers", "[gameexedetect]") {
    GameExeDetect::Candidate redist;
    redist.fileNameLower = QStringLiteral("vcredist_x64.exe");
    redist.sizeBytes = 50'000'000;
    REQUIRE(GameExeDetect::scoreCandidate(redist).skip);

    GameExeDetect::Candidate setup;
    setup.fileNameLower = QStringLiteral("setup.exe");
    setup.sizeBytes = 10'000'000;
    REQUIRE(GameExeDetect::scoreCandidate(setup).installer);

    GameExeDetect::Candidate root;
    root.fileNameLower = QStringLiteral("game.exe");
    root.relativePath = QStringLiteral("/game.exe");
    root.sizeBytes = 1'000'000;
    const auto rootScore = GameExeDetect::scoreCandidate(root);
    REQUIRE_FALSE(rootScore.skip);
    REQUIRE_FALSE(rootScore.installer);

    GameExeDetect::Candidate deep;
    deep.fileNameLower = QStringLiteral("game.exe");
    deep.relativePath = QStringLiteral("/bin/x/y/game.exe");
    deep.sizeBytes = 1'000'000;
    REQUIRE(GameExeDetect::scoreCandidate(root).score
            > GameExeDetect::scoreCandidate(deep).score);
}

TEST_CASE("scoreCandidate: crash handlers are never the game", "[gameexedetect]") {
    for (const char *n : {"unitycrashhandler64.exe", "unitycrashhandler32.exe", "crash_handler.exe",
                          "crashpad_handler.exe"}) {
        GameExeDetect::Candidate c;
        c.fileNameLower = QString::fromLatin1(n);
        c.sizeBytes = 1'600'000;
        REQUIRE(GameExeDetect::scoreCandidate(c).skip);
    }
}

TEST_CASE("neverTheGame: a saved crash handler is dropped, a game named like a tool is not", "[gameexedetect]") {
    REQUIRE(GameExeDetect::neverTheGame(QStringLiteral("unitycrashhandler64.exe")));
    REQUIRE(GameExeDetect::neverTheGame(QStringLiteral("unins000.exe")));
    // In the skip list (so never auto-picked) but fine when the user chose it.
    REQUIRE_FALSE(GameExeDetect::neverTheGame(QStringLiteral("settings.exe")));
    REQUIRE_FALSE(GameExeDetect::neverTheGame(QStringLiteral("bombanana.exe")));
}

TEST_CASE("scoreCandidate: one folder down still counts as the game's root", "[gameexedetect]") {
    // An extracted archive puts the game in its own folder: "Game/Game.exe".
    GameExeDetect::Candidate inFolder;
    inFolder.fileNameLower = QStringLiteral("game.exe");
    inFolder.relativePath = QStringLiteral("/Game/Game.exe");
    inFolder.sizeBytes = 1'000'000;
    GameExeDetect::Candidate tool;
    tool.fileNameLower = QStringLiteral("tool.exe");
    tool.relativePath = QStringLiteral("/Game/Engine/Binaries/tool.exe");
    tool.sizeBytes = 50'000'000;
    REQUIRE(GameExeDetect::scoreCandidate(inFolder).score > GameExeDetect::scoreCandidate(tool).score);
}

TEST_CASE("autodetect: a Unity game's exe wins over a bigger exe beside it", "[gameexedetect]") {
    // BOMBANANA!'s shape: Game.exe 667 KB next to Game_Data/, plus a 1.6 MB
    // crash handler. Another big helper stands in for anything not on the skip list.
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    const QString game = tmp.path() + QStringLiteral("/BOMBANANA!");
    REQUIRE(QDir().mkpath(game + QStringLiteral("/BOMBANANA_Data")));
    auto write = [](const QString &path, int bytes) {
        QFile f(path);
        REQUIRE(f.open(QIODevice::WriteOnly));
        f.write(QByteArray(bytes, 'x'));
    };
    write(game + QStringLiteral("/BOMBANANA.exe"), 667'648);
    write(game + QStringLiteral("/UnityCrashHandler64.exe"), 1'609'640);
    write(game + QStringLiteral("/helper.exe"), 3'000'000);
    bool installer = true;
    REQUIRE(GameExeDetect::autodetect(tmp.path(), &installer) == game + QStringLiteral("/BOMBANANA.exe"));
    REQUIRE_FALSE(installer);
}

TEST_CASE("scoreCandidate: an exe with a <name>_Data folder beside it is the Unity game", "[gameexedetect]") {
    // Same folder, same size: only the Unity data folder tells the game apart.
    GameExeDetect::Candidate game;
    game.fileNameLower = QStringLiteral("bombanana.exe");
    game.relativePath = QStringLiteral("/BOMBANANA!/BOMBANANA.exe");
    game.sizeBytes = 667'648;
    game.hasUnityData = true;
    GameExeDetect::Candidate other = game;
    other.fileNameLower = QStringLiteral("helper.exe");
    other.sizeBytes = 3'000'000;
    other.hasUnityData = false;
    REQUIRE(GameExeDetect::scoreCandidate(game).score > GameExeDetect::scoreCandidate(other).score);
    // Backslashes and a leading separator count the same as slashes for depth.
    GameExeDetect::Candidate win = other;
    win.relativePath = QStringLiteral("\\BOMBANANA!\\helper.exe");
    REQUIRE(GameExeDetect::scoreCandidate(win).score == GameExeDetect::scoreCandidate(other).score);
}
