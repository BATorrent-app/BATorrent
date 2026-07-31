// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/qmlnotificationbridge.h"

#include "services/platform/soundplayer.h"
#include "services/platform/translator.h"
#include "torrent/iengine.h"

#include <QApplication>
#include <QSettings>

static void maybeBeep()
{
    if (QSettings().value(QStringLiteral("notifSound"), true).toBool())
        QApplication::beep();
}

// QApplication::beep() on Windows plays the system's generic alert sound —
// a user reported it's indistinguishable from an error/warning beep, which
// reads as "download failed" right after a successful one. Kill-switch and
// suspicious-file warnings stay on maybeBeep() (an alert IS what those are).
static void maybeChime()
{
    if (QSettings().value(QStringLiteral("notifSound"), true).toBool())
        SoundPlayer::playCompletionChime();
}

void QmlNotificationBridge::onTorrentAdded(const QString &name)
{
    emit notify(tr_("notif_torrent_added"), name, 0);
}

void QmlNotificationBridge::onTorrentFinished(const QString &name, const QString &infoHash)
{
    maybeChime();
    // movies are surfaced by QmlSessionBridge::movieReady as an actionable
    // "Play now" toast — don't double up with the generic one.
    if (m_session) {
        const int row = m_session->torrentIndexByInfoHash(infoHash);
        if (row >= 0 && m_session->torrentHasVideo(row)) return;
    }
    emit notify(tr_("dlg_download_complete"), name, 3);
}

void QmlNotificationBridge::onTorrentError(const QString &message)
{
    emit notify(tr_("dlg_error"), message, 2);
}

void QmlNotificationBridge::onKillSwitchTriggered()
{
    emit notify(tr_("killswitch_title"), tr_("killswitch_triggered"), 1);
    maybeBeep();
}

void QmlNotificationBridge::onRssAutoDownloaded(const QString &feedName, const QString &itemTitle)
{
    emit notify(feedName, itemTitle, 0);
}

void QmlNotificationBridge::onSuspiciousFilesDetected(const QString &name, const QStringList &files)
{
    emit notify(tr_("warn_suspicious_title"),
                tr_("warn_suspicious_body").arg(name, files.join(QStringLiteral(", "))), 1);
    maybeBeep();
}
