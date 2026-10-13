// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/integrations/installerprofile.h"

#include <QDir>
#include <QDirIterator>
#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>

#include <cstring>

#if defined(Q_OS_WIN)
#include <windows.h>
#endif

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
        // /D= must be last and unquoted: carried in rawTail so the runner appends it
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
    // The installer's own name, or the folder it came in ("Game [FitGirl Repack]").
    const QFileInfo fi(installerPath);
    for (const QString &n : { fi.fileName().toLower(), fi.dir().dirName().toLower() }) {
        if (n.contains(QStringLiteral("fitgirl")) || n.contains(QStringLiteral("dodi")))
            return true;
    }

    // FreeArc payload alongside the installer is the repack tell: fg-01.bin,
    // setup-1.bin, *.arc; FitGirl also ships a script to check the .bin files.
    for (const QString &f : siblingFiles) {
        const QString lf = QFileInfo(f).fileName().toLower();
        if (lf.endsWith(QStringLiteral(".arc")))
            return true;
        if ((lf.startsWith(QStringLiteral("setup-")) || lf.startsWith(QStringLiteral("fg-")))
            && lf.endsWith(QStringLiteral(".bin")))
            return true;
        if (lf.startsWith(QStringLiteral("verify bin files")))
            return true;
    }
    return false;
}

QList<UninstallEntry> uninstallEntries()
{
    QList<UninstallEntry> out;
#if defined(Q_OS_WIN)
    const wchar_t *path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
    const struct { HKEY root; REGSAM view; } hives[] = {
        { HKEY_LOCAL_MACHINE, KEY_WOW64_64KEY },
        { HKEY_LOCAL_MACHINE, KEY_WOW64_32KEY },
        { HKEY_CURRENT_USER, 0 },
    };
    auto readString = [](HKEY key, const wchar_t *name) {
        wchar_t buf[1024];
        DWORD size = sizeof(buf);
        DWORD type = 0;
        if (RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<BYTE *>(buf), &size) != ERROR_SUCCESS
            || (type != REG_SZ && type != REG_EXPAND_SZ))
            return QString();
        return QString::fromWCharArray(buf, int(size / sizeof(wchar_t))).section(QChar(0), 0, 0).trimmed();
    };
    for (const auto &h : hives) {
        HKEY list = nullptr;
        if (RegOpenKeyExW(h.root, path, 0, KEY_READ | h.view, &list) != ERROR_SUCCESS)
            continue;
        for (DWORD i = 0;; ++i) {
            wchar_t sub[256];
            DWORD subLen = 256;
            FILETIME written = {};
            const LONG r = RegEnumKeyExW(list, i, sub, &subLen, nullptr, nullptr, nullptr, &written);
            if (r == ERROR_NO_MORE_ITEMS) break;
            if (r != ERROR_SUCCESS) continue;
            HKEY app = nullptr;
            if (RegOpenKeyExW(list, sub, 0, KEY_READ | h.view, &app) != ERROR_SUCCESS)
                continue;
            UninstallEntry e;
            e.displayName = readString(app, L"DisplayName");
            e.installLocation = readString(app, L"InstallLocation");
            if (e.installLocation.isEmpty())   // Inno keeps it here as well
                e.installLocation = readString(app, L"Inno Setup: App Path");
            // "C:/Game/game.exe,0": the icon index goes.
            e.displayIcon = readString(app, L"DisplayIcon").remove(QLatin1Char('"'));
            const int comma = e.displayIcon.lastIndexOf(QLatin1Char(','));
            bool isIndex = false;
            if (comma > 0) e.displayIcon.mid(comma + 1).trimmed().toInt(&isIndex);
            if (isIndex) e.displayIcon.truncate(comma);
            RegCloseKey(app);
            // FILETIME counts 100 ns from 1601; the Unix epoch is 11644473600 s later.
            const qint64 ticks = (qint64(written.dwHighDateTime) << 32) | written.dwLowDateTime;
            e.writtenMs = ticks / 10000 - 11644473600000LL;
            e.installLocation.remove(QLatin1Char('"'));
            if (!e.installLocation.isEmpty())
                out << e;
        }
        RegCloseKey(list);
    }
#endif
    return out;
}

// Letters and digits only, lower case: "007 First Light, v1.0" → "007firstlightv10".
static QString squash(const QString &s)
{
    QString out;
    for (const QChar c : s.toLower())
        if (c.isLetterOrNumber()) out += c;
    return out;
}

QString installDirFor(const QList<UninstallEntry> &entries, qint64 sinceMs, const QString &title)
{
    return installEntryFor(entries, sinceMs, title).installLocation;
}

UninstallEntry installEntryFor(const QList<UninstallEntry> &entries, qint64 sinceMs, const QString &title)
{
    // A short title ("Ori") would match far too much.
    const QString want = squash(title);
    auto named = [&want](const UninstallEntry &e) {
        if (want.size() < 4) return false;
        const QString have = squash(e.displayName);
        return have == want || (have.startsWith(want) && have.size() - want.size() <= 12);
    };
    QList<const UninstallEntry *> fresh;
    if (sinceMs > 0)
        for (const UninstallEntry &e : entries)
            if (e.writtenMs >= sinceMs) fresh << &e;
    for (const UninstallEntry *e : fresh)
        if (named(*e)) return *e;
    for (const UninstallEntry &e : entries)
        if (named(e)) return e;
    // The repack named the game its own way. Trust a new entry only when it is
    // the only one: browsers and launchers rewrite theirs while they update.
    return fresh.size() == 1 ? *fresh.first() : UninstallEntry{};
}

QStringList shortcutTargets()
{
    QStringList out;
#if defined(Q_OS_WIN)
    const QStringList roots = {
        QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation),
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        qEnvironmentVariable("ProgramData") + QStringLiteral("/Microsoft/Windows/Start Menu/Programs"),
        qEnvironmentVariable("PUBLIC") + QStringLiteral("/Desktop"),
    };
    for (const QString &root : roots) {
        if (root.isEmpty()) continue;
        QDirIterator it(root, { QStringLiteral("*.lnk") }, QDir::Files, QDirIterator::Subdirectories);
        int seen = 0;
        while (it.hasNext() && seen++ < 2000) {
            const QString target = QFileInfo(it.next()).symLinkTarget();   // Qt resolves .lnk on Windows
            if (!target.isEmpty()) out << target;
        }
    }
#endif
    return out;
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
