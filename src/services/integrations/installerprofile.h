// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

// Identifies which installer engine built a setup.exe and how (or whether) it can
// be driven unattended. Detection is byte-signature based and fully cross-platform;
// actually *running* an installer is Windows-only and handled by the caller.
namespace InstallerProfile {

enum class Engine {
    Unknown,
    InnoSetup,
    Nsis,
    InstallShield,
    Msi,
    WinRarSfx,
    SevenZipSfx,
};

// How the orchestrator should treat the release once on disk.
//   PortableA : no installer; extract then run the game exe directly.
//   SilentB   : generic installer that honours documented silent switches.
//   GuidedC   : installer where interaction is the point (FitGirl/DODI repacks):
//                open it, let the user finish, then detect the produced exe.
//   IsoD      : scene ISO; mount, open the inner setup (guided), copy crack.
enum class Tier { PortableA, SilentB, GuidedC, IsoD };

struct SilentInvocation {
    bool supported = false;  // false → caller must fall back to guided
    Engine engine = Engine::Unknown;
    // Program to run. Empty means "run the installer exe itself"; non-empty means a
    // launcher (e.g. "msiexec") that takes the installer as an argument.
    QString program;
    QStringList args;
    // Appended verbatim to the command line, UNQUOTED, after everything else.
    // Needed for NSIS: `/D=` must be the last token and must not be quoted even if
    // the path has spaces. Empty for every other engine.
    QString rawTail;
};

Engine detectEngine(const QString &exePath);
Engine detectEngineFromBytes(const QByteArray &bytes);
QString engineName(Engine e);

SilentInvocation silentInvocation(Engine e, const QString &installerPath, const QString &targetDir);

Tier tierForEngine(Engine e);

// FitGirl/DODI heuristic: an installer shipped next to a FreeArc payload
// (fg-01.bin, setup-1.bin, *.arc) or in a folder named after the repacker. Silent
// mode would install only default components into a folder the user never chose,
// so these go through the guided flow even when the engine could run silent.
bool isLikelyRepack(const QString &installerPath, const QStringList &siblingFiles);

// A program as Windows lists it under Installed apps (the Uninstall keys).
struct UninstallEntry {
    QString displayName;
    QString installLocation;
    qint64 writtenMs = 0;   // when the key was last written
    // The exe Installed apps shows the icon of: Inno sets it to the game's own
    // exe more often than not. Path only, the ",0" icon index dropped.
    QString displayIcon;
};

// Every entry that names a folder. Empty off Windows.
QList<UninstallEntry> uninstallEntries();

// Where a guided installer put the game. An entry written since the installer
// started wins (a repack lets the user pick any folder); otherwise one whose
// name is the game's title, for an install done earlier or by hand. "" if none.
UninstallEntry installEntryFor(const QList<UninstallEntry> &entries, qint64 sinceMs, const QString &title);
QString installDirFor(const QList<UninstallEntry> &entries, qint64 sinceMs, const QString &title);

// What every Start menu and desktop shortcut points at, for the user and for
// all users. An installer's shortcut names the game's exe outright. Empty off Windows.
QStringList shortcutTargets();

// Scene crack convention: the cracked binaries sit in a folder named after the
// group (CODEX, PLAZA, RUNE…) or literally "Crack". Returns the matching subdir
// name to copy over the install, or "" if none is present.
QString crackDir(const QStringList &subdirNames);

} // namespace InstallerProfile
