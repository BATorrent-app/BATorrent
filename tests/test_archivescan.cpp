// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// ArchiveScan beyond discovery: the passwords a release group's archives take,
// where a lone archive unpacks, and the extractor's progress. Discovery itself
// (formats, multi-part) lives in test_unit's [archive] cases.
#include <catch2/catch_test_macros.hpp>

#include "services/security/archiveextractor.h"
#include "services/security/archivescan.h"

TEST_CASE("an OnlineFix release gets its password first", "[archivescan]")
{
    // The real names: single rar, multi-part, and spellings of the group.
    for (const QString &n : { QStringLiteral("BOMBANANA!.v1.0.0-OFME.rar"),
                              QStringLiteral("Restaurats.v0.19.1.16-OFME.part1.rar"),
                              QStringLiteral("D:/Downloads/X/x.OFME.Online-Fix.rar"),
                              QStringLiteral("Game_OnlineFix.rar") }) {
        REQUIRE(ArchiveScan::taggedPasswords(n) == QStringList{ QStringLiteral("online-fix.me") });
        REQUIRE(ArchiveScan::knownPasswords(n).value(0) == QLatin1String("online-fix.me"));
    }
}

TEST_CASE("an untagged archive still gets the known passwords, none first", "[archivescan]")
{
    REQUIRE(ArchiveScan::taggedPasswords(QStringLiteral("Some.Game-RUNE.rar")).isEmpty());
    REQUIRE(ArchiveScan::knownPasswords(QStringLiteral("Some.Game-RUNE.rar")).contains(QStringLiteral("online-fix.me")));
    REQUIRE(ArchiveScan::taggedPasswords(QStringLiteral("softmesh.rar")).isEmpty());   // "ofme" inside a word
}

TEST_CASE("a lone archive unpacks into a folder named after it", "[archivescan]")
{
    REQUIRE(ArchiveScan::unpackFolderName(QStringLiteral("Sons.Of.The.Forest.Build.21012025-OFME.rar"))
            == QLatin1String("Sons.Of.The.Forest.Build.21012025-OFME"));
    REQUIRE(ArchiveScan::unpackFolderName(QStringLiteral("D:/Downloads/Game-OFME.part1.rar")) == QLatin1String("Game-OFME"));
    REQUIRE(ArchiveScan::unpackFolderName(QStringLiteral("pack.7z.001")) == QLatin1String("pack"));
    REQUIRE(ArchiveScan::unpackFolderName(QStringLiteral("data.tar.gz")) == QLatin1String("data"));
    REQUIRE(ArchiveScan::unpackFolderName(QStringLiteral("old.r00")) == QLatin1String("old"));
}

TEST_CASE("extractor progress is the last percentage it printed", "[archivescan]")
{
    // UnRAR rewrites its figure with backspaces; a chunk can hold several.
    REQUIRE(ArchiveScan::progressPercent("Extracting  a.dll  \b\b\b\b  5%\b\b\b\b 12%") == 12);
    REQUIRE(ArchiveScan::progressPercent(" 45% 12 - file.bin") == 45);          // 7-Zip -bsp1
    REQUIRE(ArchiveScan::progressPercent("Extracting  a.dll   OK") == -1);      // no figure in this chunk
    REQUIRE(ArchiveScan::progressPercent("ratio 250%") == -1);                   // not a progress figure
}

TEST_CASE("the half-done marker sits in the folder being unpacked", "[archivescan]")
{
    REQUIRE(ArchiveExtractor::inProgressMarker(QStringLiteral("D:/Games/X"))
            == QLatin1String("D:/Games/X/.batorrent-extracting"));
}
