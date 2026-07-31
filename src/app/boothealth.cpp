// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "app/boothealth.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <cstring>

#ifdef BAT_HAVE_SENTRY
#include <sentry.h>
#endif

#include <libtorrent/version.hpp>

namespace BootHealth {

CheckResult checkCrashSentinel()
{
    QSettings st;
    int crashes = st.value("bootCrashes", 0).toInt();
    if (st.value("bootInProgress", false).toBool())
        ++crashes;
    st.setValue("bootCrashes", crashes);
    st.setValue("bootInProgress", true);
    st.sync();

    if (crashes < 2)
        return CheckResult::Continue;

    QMessageBox box;
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle("BATorrent — Recovery");
    box.setText("BATorrent didn't start properly the last couple of times.");
    box.setInformativeText("You can reset settings, get the latest version, or try starting normally.");
    QPushButton *resetBtn = box.addButton("Reset settings & restart", QMessageBox::DestructiveRole);
    QPushButton *dlBtn    = box.addButton("Download latest", QMessageBox::ActionRole);
    QPushButton *contBtn  = box.addButton("Continue anyway", QMessageBox::AcceptRole);
    box.setDefaultButton(contBtn);
    box.exec();
    if (box.clickedButton() == resetBtn) {
        const QString resumeDir =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/resume";
        QSettings().clear();
        QSettings().sync();
        QDir(resumeDir).removeRecursively();
        QProcess::startDetached(QApplication::applicationFilePath(), {});
        return CheckResult::ExitZero;
    }
    if (box.clickedButton() == dlBtn) {
        QDesktopServices::openUrl(QUrl("https://github.com/BATorrent-app/BATorrent/releases/latest"));
        return CheckResult::ExitZero;
    }
    return CheckResult::ContinueSafeMode;
}

CheckResult checkLibtorrentAbi()
{
    if (!QString::fromLatin1(lt::version()).startsWith(QStringLiteral(LIBTORRENT_VERSION))) {
        QMessageBox box;
        box.setIcon(QMessageBox::Critical);
        box.setWindowTitle("BATorrent — Recovery");
        box.setText("This installation is broken: the torrent engine on disk is from a different version.");
        box.setInformativeText("This usually happens when an update is interrupted. Please download the latest version again.");
        QPushButton *dlBtn = box.addButton("Download latest", QMessageBox::AcceptRole);
        box.addButton("Quit", QMessageBox::RejectRole);
        box.setDefaultButton(dlBtn);
        box.exec();
        if (box.clickedButton() == dlBtn)
            QDesktopServices::openUrl(QUrl("https://github.com/BATorrent-app/BATorrent/releases/latest"));
        return CheckResult::ExitOne;
    }

#ifdef BAT_LIBTORRENT_FORK
    if (qstrcmp(lt::version(), LIBTORRENT_VERSION) != 0) {
#ifdef BAT_HAVE_SENTRY
        sentry_set_tag("lt.runtime", lt::version());
#endif
        QMessageBox::critical(nullptr, QStringLiteral("BATorrent"),
            QStringLiteral("The torrent engine on this install (%1) does not match "
                           "this version of BATorrent (%2).\n\nThe installation is "
                           "likely corrupted — please reinstall BATorrent.")
                .arg(QString::fromLatin1(lt::version()),
                     QStringLiteral(LIBTORRENT_VERSION)));
        return CheckResult::ExitOne;
    }
#endif
    return CheckResult::Continue;
}

} // namespace BootHealth
