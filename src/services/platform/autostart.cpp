// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/platform/autostart.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>

#ifdef Q_OS_WIN
#  include <QSettings>
#endif

namespace {

// The .app bundle, not the helper inside it — launchd and Explorer both want
// the thing a user would double-click.
QString launchTarget()
{
#ifdef Q_OS_MACOS
    QDir d(QCoreApplication::applicationDirPath());   // …/BATorrent.app/Contents/MacOS
    if (d.cdUp() && d.cdUp()) {                       // …/BATorrent.app
        const QString bundle = QDir::cleanPath(d.absolutePath());
        if (bundle.endsWith(QLatin1String(".app")))
            return bundle;
    }
#endif
    return QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
}

#ifdef Q_OS_WIN
constexpr char kRunKey[] = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr char kValueName[] = "BATorrent";
#else
QString entryPath()
{
#  ifdef Q_OS_MACOS
    return QDir::homePath() + QStringLiteral("/Library/LaunchAgents/com.batorrent.autostart.plist");
#  else
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + QStringLiteral("/autostart/batorrent.desktop");
#  endif
}
#endif

} // namespace

namespace Autostart {

bool isSupported()
{
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    return true;
#else
    return false;
#endif
}

bool launchedBySystem(const QStringList &args)
{
    return args.contains(QLatin1String(kAutostartFlag));
}

bool isEnabled()
{
#ifdef Q_OS_WIN
    QSettings run(QLatin1String(kRunKey), QSettings::NativeFormat);
    return !run.value(QLatin1String(kValueName)).toString().isEmpty();
#elif defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    return QFile::exists(entryPath());
#else
    return false;
#endif
}

bool setEnabled(bool on)
{
#ifdef Q_OS_WIN
    QSettings run(QLatin1String(kRunKey), QSettings::NativeFormat);
    if (!on) {
        run.remove(QLatin1String(kValueName));
        run.sync();
        return run.status() == QSettings::NoError;
    }
    // Quoted: Program Files has a space, and an unquoted Run value would be
    // parsed as a path plus arguments.
    run.setValue(QLatin1String(kValueName),
                 QStringLiteral("\"%1\" %2").arg(launchTarget(),
                                                 QLatin1String(kAutostartFlag)));
    run.sync();
    return run.status() == QSettings::NoError;

#elif defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    const QString path = entryPath();
    if (!on)
        return !QFile::exists(path) || QFile::remove(path);

    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "[autostart] cannot write" << path << f.errorString();
        return false;
    }
    QTextStream out(&f);
#  ifdef Q_OS_MACOS
    // open(1) rather than the inner binary: launching the bundle keeps the Dock
    // icon and the app's own single-instance handling intact.
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\""
        << " \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
        << "<plist version=\"1.0\">\n<dict>\n"
        << "  <key>Label</key><string>com.batorrent.autostart</string>\n"
        << "  <key>ProgramArguments</key>\n  <array>\n"
        << "    <string>/usr/bin/open</string>\n"
        << "    <string>-a</string>\n"
        << "    <string>" << launchTarget() << "</string>\n"
        << "    <string>--args</string>\n"
        << "    <string>" << QLatin1String(kAutostartFlag) << "</string>\n"
        << "  </array>\n"
        << "  <key>RunAtLoad</key><true/>\n"
        << "</dict>\n</plist>\n";
#  else
    out << "[Desktop Entry]\n"
        << "Type=Application\n"
        << "Name=BATorrent\n"
        << "Exec=\"" << launchTarget() << "\" " << QLatin1String(kAutostartFlag) << "\n"
        << "X-GNOME-Autostart-enabled=true\n"
        << "Terminal=false\n";
#  endif
    out.flush();
    f.close();
    return f.error() == QFile::NoError;

#else
    Q_UNUSED(on);
    return false;
#endif
}

} // namespace Autostart
