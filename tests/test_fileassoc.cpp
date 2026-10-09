// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include <catch2/catch_test_macros.hpp>

#include "services/platform/fileassociation.h"

using FileAssociation::driftedKinds;

namespace {

const QString kExe = QStringLiteral("C:\\Program Files\\BATorrent\\BATorrent.exe");

QString openCmd(const QString &exe)
{
    return QStringLiteral("\"%1\" \"%2\"").arg(exe, QStringLiteral("%1"));
}

QMap<QString, bool> allWanted()
{
    return { { "torrent", true }, { "magnet", true }, { "bittorrent", true } };
}

} // namespace

TEST_CASE("nothing drifts while the registry still points at us", "[fileassoc]")
{
    const QMap<QString, QString> registered {
        { "torrent",    openCmd(kExe) },
        { "magnet",     openCmd(kExe) },
        { "bittorrent", openCmd(kExe) },
    };
    REQUIRE(driftedKinds(allWanted(), registered, kExe).isEmpty());
}

TEST_CASE("an uninstall takes the keys with it and every kind drifts", "[fileassoc]")
{
    // Reinstalling leaves the Setup checkboxes unticked, so nothing rewrites
    // them: the stored preference is the only record that they were ever on.
    const QMap<QString, QString> registered {
        { "torrent", {} }, { "magnet", {} }, { "bittorrent", {} },
    };
    REQUIRE(driftedKinds(allWanted(), registered, kExe)
            == QStringList{ "bittorrent", "magnet", "torrent" });
}

TEST_CASE("moving the install directory drifts every kind", "[fileassoc]")
{
    const QString old = QStringLiteral("D:\\Apps\\BATorrent\\BATorrent.exe");
    const QMap<QString, QString> registered {
        { "torrent",    openCmd(old) },
        { "magnet",     openCmd(old) },
        { "bittorrent", openCmd(old) },
    };
    REQUIRE(driftedKinds(allWanted(), registered, kExe).size() == 3);
}

TEST_CASE("another application taking a kind over drifts only that one", "[fileassoc]")
{
    const QMap<QString, QString> registered {
        { "torrent",    openCmd(kExe) },
        { "magnet",     openCmd(QStringLiteral("C:\\qBittorrent\\qbittorrent.exe")) },
        { "bittorrent", openCmd(kExe) },
    };
    REQUIRE(driftedKinds(allWanted(), registered, kExe) == QStringList{ "magnet" });
}

TEST_CASE("a kind the user turned off is never taken back", "[fileassoc]")
{
    // Whatever holds it now is somebody else's association, and reclaiming it
    // behind the user's back is how two torrent clients fight over a machine.
    const QMap<QString, bool> wanted {
        { "torrent", true }, { "magnet", false }, { "bittorrent", false },
    };
    const QMap<QString, QString> registered {
        { "torrent",    openCmd(kExe) },
        { "magnet",     openCmd(QStringLiteral("C:\\qBittorrent\\qbittorrent.exe")) },
        { "bittorrent", {} },
    };
    REQUIRE(driftedKinds(wanted, registered, kExe).isEmpty());
}

TEST_CASE("the registry's casing is not ours to depend on", "[fileassoc]")
{
    const QMap<QString, bool> wanted { { "torrent", true } };
    const QMap<QString, QString> registered {
        { "torrent", openCmd(QStringLiteral("c:\\program files\\batorrent\\BATORRENT.EXE")) },
    };
    REQUIRE(driftedKinds(wanted, registered, kExe).isEmpty());
}

TEST_CASE("an unknown exe path claims no drift it cannot fix", "[fileassoc]")
{
    REQUIRE(driftedKinds(allWanted(), {}, QString()).isEmpty());
}
