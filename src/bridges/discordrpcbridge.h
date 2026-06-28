// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef DISCORDRPCBRIDGE_H
#define DISCORDRPCBRIDGE_H

#include "bridges/bridgecommon.h"

class DiscordRpcBridge : public QObject
{
    Q_OBJECT
public:
    explicit DiscordRpcBridge(IEngine *session, QObject *parent = nullptr);

public slots:
    void refresh();

private:
    IEngine *m_session;
    DiscordRPC *m_rpc = nullptr;
    QTimer m_timer;
    qint64 m_sessionStart = 0;
    QString m_lastActivityKey;   // skip resending an identical presence each tick
};

#endif // DISCORDRPCBRIDGE_H
