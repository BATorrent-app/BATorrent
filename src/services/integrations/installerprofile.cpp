// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/integrations/installerprofile.h"

#include <QFile>
#include <QFileInfo>

#include <cstring>

namespace {

bool contains(const QByteArray &hay, const char *needle, int len)
{
    return hay.indexOf(QByteArray::fromRawData(needle, len)) >= 0;
}

} // namespace

namespace InstallerProfile {

Engine detectEngineFromBytes(const QByteArray &bytes)
{
    if (bytes.size() < 8)
        return Engine::Unknown;

    // MSI / any OLE2 compound file: D0 CF 11 E0 A1 B1 1A E1 at offset 0.
    static const char ole[] = { '\xD0', '\xCF', '\x11', '\xE0', '\xA1', '\xB1', '\x1A', '\xE1' };
    if (bytes.size() >= 8 && memcmp(bytes.constData(), ole, 8) == 0)
        return Engine::Msi;

    // Inno Setup: 4-byte magic "Inno" at offset 0x30 in the loader header.
    if (bytes.size() >= 0x34 && memcmp(bytes.constData() + 0x30, "Inno", 4) == 0)
        return Engine::InnoSetup;
    // Fallback marker present in the data section even when the loader is repacked.
    if (contains(bytes, "Inno Setup Setup Data", 21))
        return Engine::InnoSetup;

    // NSIS: the FirstHeader anchor DEADBEEF + "NullsoftInst", appended after the PE.
    static const char nsis[] = { '\xEF', '\xBE', '\xAD', '\xDE',
                                 'N', 'u', 'l', 'l', 's', 'o', 'f', 't', 'I', 'n', 's', 't' };
    if (contains(bytes, nsis, int(sizeof(nsis))))
        return Engine::Nsis;
    if (contains(bytes, "Nullsoft.NSIS.exehead", 21))
        return Engine::Nsis;

    // InstallShield launcher markers.
    if (contains(bytes, "ISSetupStream", 13) || contains(bytes, "_ISMSIDEX", 9)
        || contains(bytes, "InstallShield", 13))
        return Engine::InstallShield;

    // Self-extracting archives: a PE (MZ) with an archive appended.
    const bool isPe = bytes.size() >= 2 && bytes.at(0) == 'M' && bytes.at(1) == 'Z';
    if (isPe) {
        if (contains(bytes, "Rar!\x1A\x07", 6))
            return Engine::WinRarSfx;
        static const char sevenz[] = { '7', 'z', '\xBC', '\xAF', '\x27', '\x1C' };
        if (contains(bytes, sevenz, int(sizeof(sevenz))))
            return Engine::SevenZipSfx;
    }

    return Engine::Unknown;
}

Engine detectEngine(const QString &exePath)
{
    const QString lower = exePath.toLower();
    if (lower.endsWith(QStringLiteral(".msi")))
        return Engine::Msi;

    QFile f(exePath);
    if (!f.open(QIODevice::ReadOnly))
        return Engine::Unknown;

    const qint64 size = f.size();
    // NSIS/SFX markers are appended after the PE, so scan a head chunk (Inno/MSI/IS
    // markers) and a tail chunk (appended archives) rather than the whole file.
    constexpr qint64 kHead = 256 * 1024;
    constexpr qint64 kTail = 1024 * 1024;

    QByteArray head = f.read(qMin(size, kHead));
    Engine e = detectEngineFromBytes(head);
    if (e != Engine::Unknown)
        return e;

    if (size > kHead) {
        f.seek(qMax<qint64>(0, size - kTail));
        QByteArray tail = f.read(kTail);
        // Keep the MZ prefix in scope so SFX detection (which requires a PE) holds.
        if (!head.isEmpty())
            tail.prepend(head.left(2));
        e = detectEngineFromBytes(tail);
    }
    return e;
}

QString engineName(Engine e)
{
    switch (e) {
    case Engine::InnoSetup:     return QStringLiteral("Inno Setup");
    case Engine::Nsis:          return QStringLiteral("NSIS");
    case Engine::InstallShield: return QStringLiteral("InstallShield");
    case Engine::Msi:           return QStringLiteral("MSI");
    case Engine::WinRarSfx:     return QStringLiteral("WinRAR SFX");
    case Engine::SevenZipSfx:   return QStringLiteral("7-Zip SFX");
    case Engine::Unknown:       break;
    }
    return QStringLiteral("Unknown");
}

SilentInvocation silentInvocation(Engine e, const QString &installerPath, const QString &targetDir)
{
    SilentInvocation s;
    s.engine = e;
    const bool haveDir = !targetDir.isEmpty();

    switch (e) {
    case Engine::InnoSetup:
        s.supported = true;
        s.args = { QStringLiteral("/VERYSILENT"), QStringLiteral("/SUPPRESSMSGBOXES"),
                   QStringLiteral("/NORESTART"), QStringLiteral("/SP-") };
        if (haveDir)
            s.args << QStringLiteral("/DIR=%1").arg(targetDir);
        break;
    case Engine::Nsis:
        s.supported = true;
        s.args = { QStringLiteral("/S") };
        // /D= must be last and unquoted — carried in rawTail so the runner appends it
        // raw, bypassing QProcess argument quoting.
        if (haveDir)
            s.rawTail = QStringLiteral("/D=%1").arg(targetDir);
        break;
    case Engine::InstallShield:
        s.supported = true;
        s.args = { QStringLiteral("/s"), QStringLiteral("/v\"/qn\"") };
        break;
    case Engine::Msi:
        s.supported = true;
        s.program = QStringLiteral("msiexec");
        s.args = { QStringLiteral("/i"), installerPath, QStringLiteral("/qn"),
                   QStringLiteral("/norestart") };
        if (haveDir)
            s.args << QStringLiteral("INSTALLDIR=%1").arg(targetDir);
        break;
    case Engine::WinRarSfx:
        s.supported = true;
        s.args = { QStringLiteral("-s1") };
        if (haveDir)
            s.args << QStringLiteral("-d%1").arg(targetDir);
        break;
    case Engine::SevenZipSfx:
        // 7-Zip SFX silent behaviour is baked into the archive config, not a switch;
        // safer to extract it (handled upstream) than to run it blind.
        s.supported = false;
        break;
    case Engine::Unknown:
        s.supported = false;
        break;
    }
    return s;
}

Tier tierForEngine(Engine e)
{
    switch (e) {
    case Engine::InnoSetup:
    case Engine::Nsis:
    case Engine::InstallShield:
    case Engine::Msi:
    case Engine::WinRarSfx:
        return Tier::SilentB;
    case Engine::SevenZipSfx:
    case Engine::Unknown:
        return Tier::GuidedC;
    }
    return Tier::GuidedC;
}

bool isLikelyRepack(const QString &installerPath, const QStringList &siblingFiles)
{
    const QString name = QFileInfo(installerPath).fileName().toLower();
    if (name.contains(QStringLiteral("fitgirl")) || name.contains(QStringLiteral("dodi")))
        return true;

    // FreeArc payload alongside the installer is the repack tell: setup-*.bin / *.arc.
    for (const QString &f : siblingFiles) {
        const QString lf = QFileInfo(f).fileName().toLower();
        if (lf.endsWith(QStringLiteral(".arc")))
            return true;
        if (lf.startsWith(QStringLiteral("setup-")) && lf.endsWith(QStringLiteral(".bin")))
            return true;
    }
    return false;
}

QString crackDir(const QStringList &subdirNames)
{
    static const QStringList groups = {
        QStringLiteral("crack"), QStringLiteral("codex"), QStringLiteral("plaza"),
        QStringLiteral("rune"), QStringLiteral("reloaded"), QStringLiteral("skidrow"),
        QStringLiteral("flt"), QStringLiteral("tenoke"), QStringLiteral("razor1911"),
        QStringLiteral("hoodlum"), QStringLiteral("empress"), QStringLiteral("goldberg"),
        QStringLiteral("steamemu"), QStringLiteral("3dm") };
    for (const QString &d : subdirNames) {
        const QString ld = d.toLower();
        for (const QString &g : groups)
            if (ld == g) return d;
    }
    return {};
}

} // namespace InstallerProfile
