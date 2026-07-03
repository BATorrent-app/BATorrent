// SPDX-License-Identifier: MIT
// BATorrent Debrid Selection Test Suite (Catch2 v3)
//
// Pins the pure file-selection logic both debrid providers stream through.
// The RD selected→links index mapping is the subtle part: `links` only has
// entries for selected files, so a wrong mapping silently streams the wrong
// file.

#include <catch2/catch_test_macros.hpp>

#include <QJsonArray>
#include <QJsonObject>

#include "services/integrations/debridpick.h"

static QJsonObject rdFile(const QString &path, qint64 bytes, int selected)
{
    return QJsonObject{{"path", path}, {"bytes", double(bytes)}, {"selected", selected}};
}

static QJsonObject tbFile(int id, const QString &name, qint64 size)
{
    return QJsonObject{{"id", id}, {"name", name}, {"size", double(size)}};
}

TEST_CASE("DebridPick: video extension detection", "[debrid]") {
    REQUIRE(DebridPick::looksLikeVideo("Movie.mkv"));
    REQUIRE(DebridPick::looksLikeVideo("MOVIE.MKV"));
    REQUIRE(DebridPick::looksLikeVideo("show/S01E01.mp4"));
    REQUIRE_FALSE(DebridPick::looksLikeVideo("soundtrack.mp3"));
    REQUIRE_FALSE(DebridPick::looksLikeVideo("Movie.mkv.part"));
    REQUIRE_FALSE(DebridPick::looksLikeVideo(""));
}

TEST_CASE("DebridPick: RD index counts only selected files", "[debrid]") {
    // links == [sample.mkv, Movie.mkv] — the unselected .nfo must not shift it
    QJsonArray files{
        rdFile("/info.nfo",   1000, 0),
        rdFile("/sample.mkv", 5000, 1),
        rdFile("/Movie.mkv",  900000, 1),
    };
    REQUIRE(DebridPick::bestRealDebridLinkIndex(files) == 1);
}

TEST_CASE("DebridPick: RD prefers video over a larger non-video", "[debrid]") {
    QJsonArray files{
        rdFile("/extras.iso", 4000000, 1),
        rdFile("/Movie.mkv",  2000000, 1),
    };
    REQUIRE(DebridPick::bestRealDebridLinkIndex(files) == 1);
}

TEST_CASE("DebridPick: RD falls back to largest file when no video", "[debrid]") {
    QJsonArray files{
        rdFile("/a.rar", 100, 1),
        rdFile("/b.rar", 900, 1),
        rdFile("/c.rar", 500, 1),
    };
    REQUIRE(DebridPick::bestRealDebridLinkIndex(files) == 1);
}

TEST_CASE("DebridPick: RD returns -1 when nothing selected or empty", "[debrid]") {
    REQUIRE(DebridPick::bestRealDebridLinkIndex({}) == -1);
    QJsonArray files{ rdFile("/Movie.mkv", 900, 0) };
    REQUIRE(DebridPick::bestRealDebridLinkIndex(files) == -1);
}

TEST_CASE("DebridPick: TorBox returns the file id, not a position", "[debrid]") {
    QJsonArray files{
        tbFile(5, "sample.mkv", 5000),
        tbFile(9, "Movie.mkv",  900000),
        tbFile(2, "readme.txt", 100),
    };
    const auto best = DebridPick::bestTorBoxFile(files);
    REQUIRE(best.first == 9);
    REQUIRE(best.second == "Movie.mkv");
}

TEST_CASE("DebridPick: TorBox prefers video, falls back to largest", "[debrid]") {
    QJsonArray withVideo{
        tbFile(1, "extras.iso", 4000000),
        tbFile(2, "Movie.mkv",  2000000),
    };
    REQUIRE(DebridPick::bestTorBoxFile(withVideo).first == 2);

    QJsonArray noVideo{
        tbFile(1, "a.rar", 100),
        tbFile(2, "b.rar", 900),
    };
    REQUIRE(DebridPick::bestTorBoxFile(noVideo).first == 2);
}

TEST_CASE("DebridPick: TorBox skips entries without an id", "[debrid]") {
    QJsonArray files{
        QJsonObject{{"name", "Movie.mkv"}, {"size", 900.0}},   // no id → unusable
    };
    const auto best = DebridPick::bestTorBoxFile(files);
    REQUIRE(best.first == -1);
    REQUIRE(DebridPick::bestTorBoxFile({}).first == -1);
}
