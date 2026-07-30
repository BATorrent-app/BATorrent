#include <catch2/catch_test_macros.hpp>
#include "services/integrations/gameinstall.h"

using GameInstall::DeriveIn;
using GameInstall::PendingIn;
using GameInstall::PendingAction;

TEST_CASE("derive: playing outranks everything", "[gameinstall]") {
    DeriveIn in;
    in.running = true;
    in.hasExePath = true;
    in.exeExists = true;
    in.completed = true;
    in.overlay = GameInstall::Failed;
    REQUIRE(GameInstall::derive(in) == GameInstall::Playing);
}

TEST_CASE("derive: overlay wins when not running", "[gameinstall]") {
    DeriveIn in;
    in.overlay = GameInstall::Extracting;
    in.completed = true;
    REQUIRE(GameInstall::derive(in) == GameInstall::Extracting);

    in.overlay = GameInstall::Installing;
    REQUIRE(GameInstall::derive(in) == GameInstall::Installing);

    in.overlay = GameInstall::NeedsSetup;
    REQUIRE(GameInstall::derive(in) == GameInstall::NeedsSetup);
}

TEST_CASE("derive: live exe path is Ready even while downloading", "[gameinstall]") {
    DeriveIn in;
    in.hasExePath = true;
    in.exeExists = true;
    in.completed = false;
    REQUIRE(GameInstall::derive(in) == GameInstall::Ready);
}

TEST_CASE("derive: never Ready with a dead exe path", "[gameinstall]") {
    // Regression: completed + stale gameExe/<hash> used to report Ready.
    DeriveIn in;
    in.hasExePath = true;
    in.exeExists = false;
    in.completed = true;
    REQUIRE(GameInstall::derive(in) == GameInstall::ReadyToInstall);
    REQUIRE(GameInstall::shouldClearStaleExe(in));

    in.completed = false;
    REQUIRE(GameInstall::derive(in) == GameInstall::Downloading);
    REQUIRE(GameInstall::shouldClearStaleExe(in));
}

TEST_CASE("derive: completed without exe is ReadyToInstall", "[gameinstall]") {
    DeriveIn in;
    in.completed = true;
    REQUIRE(GameInstall::derive(in) == GameInstall::ReadyToInstall);
    REQUIRE_FALSE(GameInstall::shouldClearStaleExe(in));
}

TEST_CASE("derive: incomplete download is Downloading", "[gameinstall]") {
    DeriveIn in;
    REQUIRE(GameInstall::derive(in) == GameInstall::Downloading);
}

TEST_CASE("shouldClearStaleExe ignores overlay and running", "[gameinstall]") {
    DeriveIn in;
    in.hasExePath = true;
    in.exeExists = false;
    in.overlay = GameInstall::Installing;
    REQUIRE_FALSE(GameInstall::shouldClearStaleExe(in));

    in.overlay = -1;
    in.running = true;
    REQUIRE_FALSE(GameInstall::shouldClearStaleExe(in));
}

TEST_CASE("nextPendingAction: launch when Ready", "[gameinstall]") {
    PendingIn in;
    in.state = GameInstall::Ready;
    in.downloadDone = true;
    REQUIRE(GameInstall::nextPendingAction(in) == PendingAction::LaunchAndFinish);
}

TEST_CASE("nextPendingAction: finish-only when already Playing", "[gameinstall]") {
    PendingIn in;
    in.state = GameInstall::Playing;
    REQUIRE(GameInstall::nextPendingAction(in) == PendingAction::FinishOnly);
}

TEST_CASE("nextPendingAction: setup / fail terminal", "[gameinstall]") {
    PendingIn in;
    in.state = GameInstall::NeedsSetup;
    REQUIRE(GameInstall::nextPendingAction(in) == PendingAction::FailNeedSetup);
    in.state = GameInstall::Failed;
    REQUIRE(GameInstall::nextPendingAction(in) == PendingAction::FailGeneric);
}

TEST_CASE("nextPendingAction: kick install once download completes", "[gameinstall]") {
    PendingIn in;
    in.state = GameInstall::ReadyToInstall;
    in.downloadDone = true;
    in.installAlreadyKicked = false;
    REQUIRE(GameInstall::nextPendingAction(in) == PendingAction::KickInstall);

    in.installAlreadyKicked = true;
    REQUIRE(GameInstall::nextPendingAction(in) == PendingAction::EmitDownloadProgress);
}

TEST_CASE("nextPendingAction: do not re-kick while extracting/installing", "[gameinstall]") {
    PendingIn in;
    in.downloadDone = true;
    in.installAlreadyKicked = false;
    in.state = GameInstall::Extracting;
    REQUIRE(GameInstall::nextPendingAction(in) == PendingAction::EmitInstallProgress);
    in.state = GameInstall::Installing;
    REQUIRE(GameInstall::nextPendingAction(in) == PendingAction::EmitInstallProgress);
}

TEST_CASE("nextPendingAction: download progress while seeding in", "[gameinstall]") {
    PendingIn in;
    in.state = GameInstall::Downloading;
    in.downloadDone = false;
    REQUIRE(GameInstall::nextPendingAction(in) == PendingAction::EmitDownloadProgress);
}

TEST_CASE("nextPendingAction: missing torrent waits then times out", "[gameinstall]") {
    PendingIn in;
    in.torrentMissing = true;
    in.ageSec = 10;
    REQUIRE(GameInstall::nextPendingAction(in) == PendingAction::Wait);
    in.ageSec = 181;
    REQUIRE(GameInstall::nextPendingAction(in) == PendingAction::FailTimeout);
}

TEST_CASE("nextPendingAction: overall timeout only after download completes", "[gameinstall]") {
    PendingIn in;
    in.state = GameInstall::Downloading;
    in.downloadDone = false;
    in.ageSec = 1801;
    // Still downloading — do not kill the Get & Install chain on a slow swarm.
    REQUIRE(GameInstall::nextPendingAction(in) == PendingAction::EmitDownloadProgress);

    in.downloadDone = true;
    in.state = GameInstall::ReadyToInstall;
    in.installAlreadyKicked = true;
    REQUIRE(GameInstall::nextPendingAction(in) == PendingAction::FailTimeout);
}
