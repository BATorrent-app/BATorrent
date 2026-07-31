#include <catch2/catch_test_macros.hpp>
#include "torrent/sessionresume.h"

#include <QDir>
#include <string>
#include <vector>
#include <utility>

using SessionResume::CorruptResumeAction;
using SessionResume::DiskFilePresence;
using SessionResume::reconcileIncompleteSuffix;

TEST_CASE("resume file naming", "[sessionresume]") {
    REQUIRE(SessionResume::resumeFileName(QStringLiteral("abc"))
            == QStringLiteral("abc.resume"));
    REQUIRE(SessionResume::corruptSidecarName(QStringLiteral("abc.resume"))
            == QStringLiteral("abc.resume.corrupt"));
    REQUIRE(SessionResume::partsSidecarName(QStringLiteral("deadbeef"))
            == QStringLiteral(".deadbeef.parts"));
}

TEST_CASE("removed history dir is sibling of resume/", "[sessionresume]") {
    const QString removed = SessionResume::removedHistoryDir(
        QStringLiteral("/tmp/BATorrent/BATorrent/resume"));
    REQUIRE(QDir::cleanPath(removed).endsWith(QStringLiteral("removed")));
    REQUIRE_FALSE(removed.contains(QStringLiteral("resume/removed")));
}

TEST_CASE("legacy resume dir is parent(appData)/resume", "[sessionresume]") {
    REQUIRE(SessionResume::legacyResumeDir(QStringLiteral("/data/BATorrent/BATorrent"))
            == QStringLiteral("/data/BATorrent/resume"));
}

TEST_CASE("shouldCopyLegacyResumes decisions", "[sessionresume]") {
    const QString legacy = QStringLiteral("/data/BATorrent/resume");
    const QString neu = QStringLiteral("/data/BATorrent/BATorrent/resume");

    REQUIRE_FALSE(SessionResume::shouldCopyLegacyResumes(true, legacy, neu, true));
    REQUIRE_FALSE(SessionResume::shouldCopyLegacyResumes(false, legacy, neu, false));
    REQUIRE_FALSE(SessionResume::shouldCopyLegacyResumes(
        false, neu, neu, true)); // same path after clean
    REQUIRE(SessionResume::shouldCopyLegacyResumes(false, legacy, neu, true));
    REQUIRE_FALSE(SessionResume::shouldCopyLegacyResumes(
        false, QString(), neu, true));
}

TEST_CASE("corrupt resume: quarantine without ti, recover with ti", "[sessionresume]") {
    REQUIRE(SessionResume::corruptResumeAction(false)
            == CorruptResumeAction::Quarantine);
    REQUIRE(SessionResume::corruptResumeAction(true)
            == CorruptResumeAction::RecoverRecheck);
}

TEST_CASE("reconcileIncompleteSuffix prefers full-size plain", "[sessionresume]") {
    DiskFilePresence disk{true, 100, true, 100};
    const auto pick = reconcileIncompleteSuffix("Movie/a.mkv.!bt", 100, true, disk);
    REQUIRE(pick.chosen == "Movie/a.mkv");
    REQUIRE(pick.wantedFullSize);
}

TEST_CASE("reconcileIncompleteSuffix falls back to full-size .!bt", "[sessionresume]") {
    DiskFilePresence disk{false, 0, true, 42};
    const auto pick = reconcileIncompleteSuffix("a.mkv", 42, true, disk);
    REQUIRE(pick.chosen == "a.mkv.!bt");
    REQUIRE(pick.wantedFullSize);
}

TEST_CASE("reconcileIncompleteSuffix keeps mapping when neither full size", "[sessionresume]") {
    DiskFilePresence disk{true, 10, true, 11};
    const auto pick = reconcileIncompleteSuffix("a.mkv.!bt", 100, true, disk);
    REQUIRE(pick.chosen == "a.mkv.!bt");
    REQUIRE_FALSE(pick.wantedFullSize);
}

TEST_CASE("reconcileIncompleteSuffix: unwanted miss does not spoil completeness",
          "[sessionresume]") {
    DiskFilePresence disk{false, 0, false, 0};
    const auto pick = reconcileIncompleteSuffix("Sample/x.jpg.!bt", 50, false, disk);
    REQUIRE(pick.chosen == "Sample/x.jpg.!bt");
    REQUIRE(pick.wantedFullSize);
}

TEST_CASE("reconcileIncompleteSuffix: wrong-size plain does not win over full .!bt",
          "[sessionresume]") {
    // Plain exists but wrong size → prefer full-size .!bt (current loadResumeData order).
    DiskFilePresence disk{true, 99, true, 100};
    const auto pick = reconcileIncompleteSuffix("a.mkv.!bt", 100, true, disk);
    REQUIRE(pick.chosen == "a.mkv.!bt");
    REQUIRE(pick.wantedFullSize);
}

TEST_CASE("incomplete suffix strip/append is idempotent", "[sessionresume]") {
    REQUIRE(SessionResume::withIncompleteSuffix("a.mkv") == "a.mkv.!bt");
    REQUIRE(SessionResume::withIncompleteSuffix("a.mkv.!bt") == "a.mkv.!bt");
    std::string p = "a.mkv.!bt";
    REQUIRE(SessionResume::stripIncompleteSuffix(p));
    REQUIRE(p == "a.mkv");
    REQUIRE_FALSE(SessionResume::stripIncompleteSuffix(p));
}

TEST_CASE("topLevelTrashNames: windows backslash + multi-file folder", "[sessionresume]") {
    const auto tops = SessionResume::topLevelTrashNames({
        QStringLiteral("Show\\S01\\ep.mkv"),
        QStringLiteral("Show/S01/ep2.mkv"),
        QStringLiteral("lone.mkv"),
        QStringLiteral(""),
    });
    REQUIRE(tops == QStringList({QStringLiteral("Show"), QStringLiteral("lone.mkv")}));
}

TEST_CASE("trashTargetsForRemoval: both suffix variants + parts sidecar",
          "[sessionresume]") {
    const auto targets = SessionResume::trashTargetsForRemoval(
        QStringLiteral("/dl"),
        {QStringLiteral("Movie.!bt"), QStringLiteral("extra")},
        QStringLiteral("abcd"));
    REQUIRE(targets.contains(QStringLiteral("/dl/Movie")));
    REQUIRE(targets.contains(QStringLiteral("/dl/Movie.!bt")));
    REQUIRE(targets.contains(QStringLiteral("/dl/extra")));
    REQUIRE(targets.contains(QStringLiteral("/dl/extra.!bt")));
    REQUIRE(targets.contains(QStringLiteral("/dl/.abcd.parts")));
}

TEST_CASE("shouldScheduleFileRemoval matrix", "[sessionresume]") {
    REQUIRE_FALSE(SessionResume::shouldScheduleFileRemoval(false, {QStringLiteral("x")}));
    REQUIRE_FALSE(SessionResume::shouldScheduleFileRemoval(true, {}));
    REQUIRE(SessionResume::shouldScheduleFileRemoval(true, {QStringLiteral("x")}));
}

TEST_CASE("shouldEmitTorrentFinished mute matrix", "[sessionresume]") {
    REQUIRE_FALSE(SessionResume::shouldEmitTorrentFinished(false, false));
    REQUIRE_FALSE(SessionResume::shouldEmitTorrentFinished(false, true));
    REQUIRE_FALSE(SessionResume::shouldEmitTorrentFinished(true, true));
    REQUIRE(SessionResume::shouldEmitTorrentFinished(true, false));
}

TEST_CASE("downloadedPayloadThisSession gates finish side effects",
          "[sessionresume]") {
    REQUIRE_FALSE(SessionResume::downloadedPayloadThisSession(0));
    REQUIRE_FALSE(SessionResume::downloadedPayloadThisSession(-1));
    REQUIRE(SessionResume::downloadedPayloadThisSession(1));
    REQUIRE(SessionResume::downloadedPayloadThisSession(4096));
}

TEST_CASE("finishMoveDestination: intended wins over auto-move",
          "[sessionresume]") {
    REQUIRE(SessionResume::finishMoveDestination(
                QStringLiteral("/final"), true, QStringLiteral("/auto"))
            == QStringLiteral("/final"));
    REQUIRE(SessionResume::finishMoveDestination(
                QString(), true, QStringLiteral("/auto"))
            == QStringLiteral("/auto"));
    REQUIRE(SessionResume::finishMoveDestination(
                QString(), false, QStringLiteral("/auto")).isEmpty());
    REQUIRE(SessionResume::finishMoveDestination(
                QString(), true, QString()).isEmpty());
    REQUIRE(SessionResume::finishMoveDestination(
                QString(), false, QString()).isEmpty());
}

TEST_CASE("finish .!bt strip: only suffixed paths rename", "[sessionresume]") {
    std::string plain = "a.mkv";
    REQUIRE_FALSE(SessionResume::stripIncompleteSuffix(plain));
    REQUIRE(plain == "a.mkv");

    std::string done = "a.mkv.!bt";
    REQUIRE(SessionResume::stripIncompleteSuffix(done));
    REQUIRE(done == "a.mkv");
}

TEST_CASE("removedHistoryHashesToPrune keeps newest 50", "[sessionresume]") {
    std::vector<std::pair<qint64, QString>> sorted;
    for (int i = 0; i < 53; ++i)
        sorted.emplace_back(i, QStringLiteral("h%1").arg(i));
    const auto prune = SessionResume::removedHistoryHashesToPrune(sorted, 50);
    REQUIRE(prune.size() == 3);
    REQUIRE(prune.at(0) == QStringLiteral("h0"));
    REQUIRE(prune.at(2) == QStringLiteral("h2"));
    REQUIRE(SessionResume::removedHistoryHashesToPrune(sorted, 53).isEmpty());
    REQUIRE(SessionResume::removedHistoryHashesToPrune({}, 50).isEmpty());
}
