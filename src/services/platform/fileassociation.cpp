// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/platform/fileassociation.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QSettings>

namespace FileAssociation {

QStringList driftedKinds(const QMap<QString, bool> &wanted,
                         const QMap<QString, QString> &registered,
                         const QString &exePath)
{
    QStringList out;
    if (exePath.isEmpty()) return out;
    for (auto it = wanted.cbegin(); it != wanted.cend(); ++it) {
        if (!it.value()) continue;
        // Case-insensitively: the registry hands paths back in whatever case
        // was written, and Windows does not care which that was.
        if (!registered.value(it.key()).contains(exePath, Qt::CaseInsensitive))
            out.append(it.key());
    }
    out.sort();
    return out;
}

#ifdef Q_OS_WIN
// One association slice, .torrent, magnet: or bittorrent:, written or
// removed independently. The key layout is the exact set the one-shot
// "set as default" button proved out in the wild; the "Default Programs"
// Capabilities plumbing is shared and needed for Windows' per-protocol
// picker and for browsers to offer us as a handler at all.
bool apply(const QString &kind, bool on)
{
    const QString nativeExe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    const QString cmd = "\"" + nativeExe + "\" \"%1\"";
    QSettings reg("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);
    QSettings caps("HKEY_CURRENT_USER\\Software\\BATorrent\\Capabilities", QSettings::NativeFormat);

    auto proto = [&](const QString &progId, const QString &friendly,
                     const QString &scheme, const QString &schemeFriendly) {
        if (on) {
            reg.setValue(progId + "/.", friendly);
            reg.setValue(progId + "/URL Protocol", "");
            reg.setValue(progId + "/shell/open/command/.", cmd);
            reg.setValue(progId + "/DefaultIcon/.", nativeExe + ",0");
            reg.setValue(scheme + "/.", schemeFriendly);
            reg.setValue(scheme + "/URL Protocol", "");
            reg.setValue(scheme + "/shell/open/command/.", cmd);
            reg.setValue(scheme + "/DefaultIcon/.", nativeExe + ",0");
            caps.setValue("UrlAssociations/" + scheme, progId);
        } else {
            reg.remove(progId);
            if (reg.value(scheme + "/shell/open/command/.").toString().contains(nativeExe))
                reg.remove(scheme);
            caps.remove("UrlAssociations/" + scheme);
        }
    };

    if (kind == QLatin1String("torrent")) {
        if (on) {
            reg.setValue(".torrent/.", "BATorrent.Torrent");
            reg.setValue("BATorrent.Torrent/.", "BATorrent Torrent File");
            reg.setValue("BATorrent.Torrent/shell/open/command/.", cmd);
            reg.setValue("BATorrent.Torrent/DefaultIcon/.", nativeExe + ",0");
            caps.setValue("FileAssociations/.torrent", "BATorrent.Torrent");
        } else {
            // Case-insensitive: the installer writes this value too, and the
            // two spellings used to differ, so an exact compare left the
            // association behind when the user switched it off.
            if (reg.value(".torrent/.").toString()
                    .compare(QLatin1String("BATorrent.Torrent"), Qt::CaseInsensitive) == 0)
                reg.remove(".torrent");
            reg.remove("BATorrent.Torrent");
            caps.remove("FileAssociations/.torrent");
        }
    } else if (kind == QLatin1String("magnet")) {
        proto("BATorrent.Url.Magnet", "BATorrent Magnet Link", "magnet", "URL:Magnet Protocol");
    } else if (kind == QLatin1String("bittorrent")) {
        proto("BATorrent.Url.BitTorrent", "BATorrent BitTorrent Link", "bittorrent", "URL:BitTorrent Protocol");
    } else {
        return false;
    }

    caps.setValue("ApplicationName", "BATorrent");
    caps.setValue("ApplicationDescription", "Lightweight, open-source BitTorrent client");
    caps.setValue("ApplicationIcon", nativeExe + ",0");
    reg.sync(); caps.sync();
    QSettings registered("HKEY_CURRENT_USER\\Software\\RegisteredApplications", QSettings::NativeFormat);
    registered.setValue("BATorrent", "Software\\BATorrent\\Capabilities");
    registered.sync();
    return reg.status() == QSettings::NoError && caps.status() == QSettings::NoError
        && registered.status() == QSettings::NoError;
}

void reconcile()
{
    QSettings prefs;
    const QMap<QString, bool> wanted {
        { QStringLiteral("torrent"),    prefs.value(QStringLiteral("assocTorrent")).toBool() },
        { QStringLiteral("magnet"),     prefs.value(QStringLiteral("assocMagnet")).toBool() },
        { QStringLiteral("bittorrent"), prefs.value(QStringLiteral("assocBittorrent")).toBool() },
    };

    QSettings reg("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);
    const QString progIdCmd = QStringLiteral("BATorrent.Torrent/shell/open/command/.");
    const QMap<QString, QString> registered {
        { QStringLiteral("torrent"),
          reg.value(".torrent/.").toString().compare(QLatin1String("BATorrent.Torrent"),
                                                     Qt::CaseInsensitive) == 0
              ? reg.value(progIdCmd).toString() : QString() },
        { QStringLiteral("magnet"),     reg.value("magnet/shell/open/command/.").toString() },
        { QStringLiteral("bittorrent"), reg.value("bittorrent/shell/open/command/.").toString() },
    };

    const QString exe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    for (const QString &kind : driftedKinds(wanted, registered, exe))
        apply(kind, true);
}

bool setAsDefaultApp()
{
    const bool ok = apply(QStringLiteral("torrent"), true)
                 && apply(QStringLiteral("magnet"), true)
                 && apply(QStringLiteral("bittorrent"), true);
    if (ok) {
        QSettings s;
        s.setValue("assocTorrent", true);
        s.setValue("assocMagnet", true);
        s.setValue("assocBittorrent", true);
    }
    return ok;
}

#elif defined(Q_OS_LINUX) || defined(Q_OS_MACOS)

bool apply(const QString &, bool)
{
    // Per-type toggles are Windows-only; rows are hidden elsewhere.
    return false;
}

// Nothing to reconcile: the desktop entry and the bundle's Info.plist travel
// with the install, and neither xdg-mime nor Launch Services loses the user's
// choice when the app is updated in place.
void reconcile() {}

bool setAsDefaultApp()
{
    // A missing helper (xdg-mime / duti) leaves exitCode() at its default 0,
    // which would look like success: gate on the process actually finishing.
    auto run = [](const QString &exe, const QStringList &args) {
        QProcess p;
        p.start(exe, args);
        return p.waitForFinished(3000)
            && p.exitStatus() == QProcess::NormalExit
            && p.exitCode() == 0;
    };
  #if defined(Q_OS_LINUX)
    return run("xdg-mime", {"default", "batorrent.desktop", "application/x-bittorrent"})
        && run("xdg-mime", {"default", "batorrent.desktop", "x-scheme-handler/magnet"});
  #else
    return run("duti", {"-s", "com.batorrent.app", ".torrent", "all"});
  #endif
}

#else

bool apply(const QString &, bool) { return false; }
bool setAsDefaultApp() { return false; }
void reconcile() {}

#endif

} // namespace FileAssociation
