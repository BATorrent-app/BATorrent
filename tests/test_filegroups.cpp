// SPDX-License-Identifier: MIT
// File grouping for the Add download window (Catch2 v3): repacks split into
// game data, language packs and extras; season packs into seasons; films
// with separate subtitles. Unknown layouts stay ungrouped.

#include <catch2/catch_test_macros.hpp>

#include "services/metadata/filegroups.h"

using namespace FileGroups;

static QList<File> files(std::initializer_list<std::pair<const char *, qint64>> list)
{
    QList<File> out;
    int i = 0;
    for (const auto &[p, s] : list) out << File{ i++, QString::fromUtf8(p), s };
    return out;
}

static const Group *find(const QList<Group> &gs, const char *kind)
{
    for (const Group &g : gs) if (g.kind == QLatin1String(kind)) return &g;
    return nullptr;
}

TEST_CASE("a FitGirl repack splits into game data, languages, extras and setup", "[filegroups]")
{
    const auto fs = files({
        { "Game [FitGirl Repack]/fg-01.bin", 4000 },
        { "Game [FitGirl Repack]/fg-02.bin", 4000 },
        { "Game [FitGirl Repack]/fg-selective-brazilian.bin", 200 },
        { "Game [FitGirl Repack]/fg-selective-french.bin", 190 },
        { "Game [FitGirl Repack]/fg-optional-soundtrack.bin", 300 },
        { "Game [FitGirl Repack]/setup.exe", 3 },
    });
    const auto gs = group(fs, "Game [FitGirl Repack]");
    REQUIRE(find(gs, "game"));
    CHECK(find(gs, "game")->size == 8000);
    REQUIRE(find(gs, "lang"));
    const Group &l = *find(gs, "lang");
    // English is in the game data: shown, nothing to download
    REQUIRE(l.chips.size() == 3);
    CHECK(l.chips.first().lang == "en");
    CHECK(l.chips.first().inBase);
    bool br = false;
    for (const Chip &c : l.chips) if (c.lang == "pt-BR") { br = true; CHECK(c.size == 200); }
    CHECK(br);
    REQUIRE(find(gs, "extras"));
    CHECK(find(gs, "extras")->chips.first().label == "Soundtrack");
    REQUIRE(find(gs, "setup"));
}

TEST_CASE("a DODI repack groups its .doi language files", "[filegroups]")
{
    const auto fs = files({
        { "Spider-Man 2 [DODI]/data1.doi", 30000 },
        { "Spider-Man 2 [DODI]/Brazilian.doi", 1840 },
        { "Spider-Man 2 [DODI]/German.doi", 1880 },
        { "Spider-Man 2 [DODI]/Setup.exe", 2 },
    });
    const auto gs = group(fs, "Spider-Man 2 [DODI Repack]");
    REQUIRE(find(gs, "lang"));
    CHECK(find(gs, "lang")->files.size() == 2);
    REQUIRE(find(gs, "game"));
    CHECK(find(gs, "game")->files.size() == 1);   // data1.doi is not a language
}

TEST_CASE("a season pack becomes one group per season with episode chips", "[filegroups]")
{
    const auto fs = files({
        { "Show/Show.S01E01.1080p.mkv", 800 },
        { "Show/Show.S01E02.1080p.mkv", 800 },
        { "Show/Show.S02E01.1080p.mkv", 800 },
        { "Show/Show.S02E02.1080p.mkv", 800 },
    });
    const auto gs = group(fs, "Show S01-S02");
    REQUIRE(gs.size() == 2);
    CHECK(gs[0].season == 1);
    CHECK(gs[0].chips.size() == 2);
    CHECK(gs[1].chips[1].label == "E02");
}

TEST_CASE("a film with separate subtitles groups them", "[filegroups]")
{
    const auto fs = files({
        { "Film.2023/Film.2023.1080p.mkv", 5000 },
        { "Film.2023/Film.2023.pt-BR.srt", 1 },
        { "Film.2023/Film.2023.en.srt", 1 },
    });
    const auto gs = group(fs, "Film 2023");
    REQUIRE(find(gs, "subs"));
    CHECK(find(gs, "subs")->chips.size() == 2);
}

TEST_CASE("an unknown layout stays ungrouped", "[filegroups]")
{
    CHECK(group(files({ { "a/readme.txt", 1 }, { "a/data.bin", 9 } }), "Something").isEmpty());
    CHECK(group(files({ { "only.iso", 9 } }), "Linux").isEmpty());
}

TEST_CASE("language tokens map to codes and their own names", "[filegroups]")
{
    CHECK(languageFromToken("brazilian").code == "pt-BR");
    CHECK(languageFromToken("Spanish_LATAM").code == "es-419");
    CHECK(languageFromToken("russian").label == QString::fromUtf8("Русский"));
    CHECK(languageFromToken("fg-01").code.isEmpty());
}
