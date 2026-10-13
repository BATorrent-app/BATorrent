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

TEST_CASE("scoreCandidate: nothing in a repack's redist folder is the game", "[gameexedetect]") {
    // FitGirl's layout after install: Retail\007FirstLight.exe beside _Redist\QuickSFV.EXE.
    GameExeDetect::Candidate tool;
    tool.fileNameLower = QStringLiteral("quicksfv.exe");
    tool.relativePath = QStringLiteral("\\_Redist\\QuickSFV.EXE");
    tool.sizeBytes = 103'424;
    REQUIRE(GameExeDetect::scoreCandidate(tool).skip);
    GameExeDetect::Candidate game;
    game.fileNameLower = QStringLiteral("007firstlight.exe");
    game.relativePath = QStringLiteral("\\Retail\\007FirstLight.exe");
    game.sizeBytes = 64'228'744;
    REQUIRE_FALSE(GameExeDetect::scoreCandidate(game).skip);
}

TEST_CASE("autodetect: a FitGirl folder gives its setup, not the checksum tool", "[gameexedetect]") {
    // DELTARUNE [FitGirl Repack] as downloaded: QuickSFV was taken for the game.
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    REQUIRE(QDir().mkpath(tmp.path() + QStringLiteral("/MD5")));
    auto write = [](const QString &path, int bytes) {
        QFile f(path);
        REQUIRE(f.open(QIODevice::WriteOnly));
        f.write(QByteArray(bytes, 'x'));
    };
    write(tmp.path() + QStringLiteral("/setup.exe"), 3000);
    write(tmp.path() + QStringLiteral("/fg-01.bin"), 100);
    write(tmp.path() + QStringLiteral("/MD5/QuickSFV.EXE"), 2000);
    write(tmp.path() + QStringLiteral("/MD5/helper.exe"), 5000);   // anything else a repack might carry
    bool installer = false;
    REQUIRE(GameExeDetect::autodetect(tmp.path(), &installer) == tmp.path() + QStringLiteral("/setup.exe"));
    REQUIRE(installer);
    REQUIRE(GameExeDetect::neverTheGame(QStringLiteral("quicksfv.exe")));
}

TEST_CASE("autodetect: Portal 2's exe at the root, not the map compiler in bin", "[gameexedetect]") {
    // DODI's Portal 2 as installed: vbsp.exe (1.5 MB) was taken for the game.
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    REQUIRE(QDir().mkpath(tmp.path() + QStringLiteral("/bin")));
    REQUIRE(QDir().mkpath(tmp.path() + QStringLiteral("/_Cracks/Crack [CODEX]/bin")));
    REQUIRE(QDir().mkpath(tmp.path() + QStringLiteral("/Uninstall")));
    auto write = [](const QString &path, int bytes) {
        QFile f(path);
        REQUIRE(f.open(QIODevice::WriteOnly));
        f.write(QByteArray(bytes, 'x'));
    };
    write(tmp.path() + QStringLiteral("/portal2.exe"), 358'912);
    write(tmp.path() + QStringLiteral("/bin/vbsp.exe"), 1'499'040);
    write(tmp.path() + QStringLiteral("/bin/vrad.exe"), 91'040);
    write(tmp.path() + QStringLiteral("/bin/Language Selector.exe"), 36'864);
    write(tmp.path() + QStringLiteral("/_Cracks/Crack [CODEX]/bin/Language Selector.exe"), 36'864);
    write(tmp.path() + QStringLiteral("/Uninstall/unins000.exe"), 1'602'929);
    bool installer = true;
    REQUIRE(GameExeDetect::autodetect(tmp.path(), &installer) == tmp.path() + QStringLiteral("/portal2.exe"));
    REQUIRE_FALSE(installer);
    REQUIRE(GameExeDetect::neverTheGame(QStringLiteral("vbsp.exe")));   // a saved one is dropped
}

// A minimal PE file: just enough header for the subsystem to be read.
static QByteArray fakePe(quint16 subsystem, int padTo)
{
    QByteArray b(padTo, '\0');
    b[0] = 'M'; b[1] = 'Z';
    b[0x3C] = 0x40;                      // e_lfanew
    b[0x40] = 'P'; b[0x41] = 'E';        // "PE" and two zero bytes
    const int sub = 0x40 + 24 + 68;      // optional header + 68
    b[sub] = char(subsystem & 0xff);
    b[sub + 1] = char(subsystem >> 8);
    return b;
}

TEST_CASE("namedFor: the exe named for the game", "[gameexedetect]") {
    REQUIRE(GameExeDetect::namedFor("portal2.exe", "Portal 2"));
    REQUIRE(GameExeDetect::namedFor("007firstlight.exe", "007 First Light"));
    REQUIRE(GameExeDetect::namedFor("sexnatural milfs must be filled.exe", "Sexnatural: Milfs Must Be Filled"));
    REQUIRE_FALSE(GameExeDetect::namedFor("vbsp.exe", "Portal 2"));
    REQUIRE_FALSE(GameExeDetect::namedFor("ori.exe", "Ori"));   // too short to trust
}

TEST_CASE("pickInstalled: the installer's shortcut wins, a console tool never does", "[gameexedetect]") {
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    REQUIRE(QDir().mkpath(tmp.path() + QStringLiteral("/bin")));
    auto write = [](const QString &path, const QByteArray &bytes) {
        QFile f(path);
        REQUIRE(f.open(QIODevice::WriteOnly));
        f.write(bytes);
    };
    const QString game = tmp.path() + QStringLiteral("/Launcher.exe");      // small, not named for the title
    const QString tool = tmp.path() + QStringLiteral("/bin/compiler.exe");  // big console tool
    const QString big = tmp.path() + QStringLiteral("/bin/Engine.exe");     // big window exe
    write(game, fakePe(2, 4096));
    write(tool, fakePe(3, 3'000'000));
    write(big, fakePe(2, 2'000'000));
    REQUIRE(GameExeDetect::isConsoleExe(tool));
    REQUIRE_FALSE(GameExeDetect::isConsoleExe(game));

    // The shortcut names the small exe: it wins over the bigger one.
    REQUIRE(GameExeDetect::pickInstalled(tmp.path(), { QDir::toNativeSeparators(game) }, "Some Game") == game);
    // A shortcut to a console tool, or to somewhere else entirely, is passed over.
    REQUIRE(GameExeDetect::pickInstalled(tmp.path(), { tool, "C:/Windows/notepad.exe" }, "Some Game") == big);
    // Nothing pointed at: the console tool still never wins on size.
    REQUIRE(GameExeDetect::pickInstalled(tmp.path(), {}, "Some Game") == big);
    // A name matching the title outweighs a bigger exe.
    REQUIRE(GameExeDetect::pickInstalled(tmp.path(), {}, "Launcher Game") == big);
    REQUIRE(GameExeDetect::pickInstalled(tmp.path(), {}, "Launcher") == game);
    // Two shortcuts into the folder: the one named for the game.
    REQUIRE(GameExeDetect::pickInstalled(tmp.path(), { big, game }, "Launcher") == game);
    REQUIRE(GameExeDetect::pickInstalled(tmp.path(), { big, game }, "Other") == big);
}
