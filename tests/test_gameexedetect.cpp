#include <catch2/catch_test_macros.hpp>
#include "services/integrations/gameexedetect.h"

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
