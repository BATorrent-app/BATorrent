// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/discordrpcbridge.h"

#include "services/integrations/discordrpc.h"
#include "services/platform/translator.h"
#include "services/platform/utils.h"
#include "torrent/iengine.h"
#include "torrent/types.h"

#include <QDateTime>
#include <QSettings>

DiscordRpcBridge::DiscordRpcBridge(IEngine *session, QObject *parent)
    : QObject(parent), m_session(session)
{
    m_rpc = new DiscordRPC(this);
    m_sessionStart = QDateTime::currentSecsSinceEpoch();
    if (QSettings().value("discordRichPresence", true).toBool())
        m_rpc->setClientId(QStringLiteral("1508208411282640956"));

    connect(&m_timer, &QTimer::timeout, this, &DiscordRpcBridge::refresh);
    m_timer.start(15000);
    refresh();
}

void DiscordRpcBridge::refresh()
{
    if (!m_rpc || m_rpc->clientId().isEmpty()) return;
    if (!QSettings().value("discordEnabled", true).toBool()) {
        if (!m_lastActivityKey.isEmpty()) { m_rpc->clearActivity(); m_lastActivityKey.clear(); }
        return;
    }
    int seeding = 0, featured = -1, featuredRate = 0;
    for (int i = 0; i < m_session->torrentCount(); ++i) {
        TorrentInfo info = m_session->torrentAt(i);
        if (info.paused || info.completed) continue;
        if (info.progress >= 1.0f) { ++seeding; continue; }
        if (info.downloadRate > featuredRate) {
            featuredRate = info.downloadRate;
            featured = i;
        }
    }
    QString details, state;
    if (featured >= 0) {
        TorrentInfo info = m_session->torrentAt(featured);
        details = info.name.left(64);
        state = QStringLiteral("%1% · ↓ %2")
            .arg(static_cast<int>(info.progress * 100))
            .arg(formatSpeed(info.downloadRate));
    } else if (seeding > 0) {
        details = tr_("discord_seeding").arg(seeding);
        state = tr_("discord_seeding_state");
    } else {
        details = tr_("discord_idle");
    }
    const QString key = details + QLatin1Char('\x1f') + state;
    if (key == m_lastActivityKey) return;
    m_lastActivityKey = key;
    m_rpc->setActivity(details, state, m_sessionStart);
}
