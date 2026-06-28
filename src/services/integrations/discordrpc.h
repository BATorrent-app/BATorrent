// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_DISCORDRPC_H
#define BATORRENT_DISCORDRPC_H

#include <QObject>
#include <QString>
#include <QTimer>

class QLocalSocket;

// Discord Rich Presence over local IPC.
//
// Protocol: connects to Discord client's local socket (named pipe on
// Windows, unix socket on macOS / Linux), sends a v1 handshake with the
// client_id, then sends SET_ACTIVITY frames to populate the user's
// profile activity ("Downloading X · 67%").
//
// Requires the user to provide a Discord application ID (free, 2-minute
// setup at https://discord.com/developers/applications). With an empty
// client ID this class is a silent no-op — no socket connection, no logs.
//
// Re-connects automatically with backoff if Discord wasn't running at
// startup or restarts mid-session.
class DiscordRPC : public QObject
{
    Q_OBJECT
public:
    explicit DiscordRPC(QObject *parent = nullptr);

    void setClientId(const QString &id);
    QString clientId() const { return m_clientId; }

    // Update the presence. Empty `details` triggers a CLEAR_ACTIVITY frame.
    // `startEpoch` is unix seconds the activity started — Discord uses it
    // to render the elapsed timer.
    void setActivity(const QString &details, const QString &state, qint64 startEpoch);
    void clearActivity();

private slots:
    void tryConnect();
    void onConnected();
    void onDisconnected();
    void onReadyRead();

private:
    void sendFrame(int op, const QByteArray &json);
    QString socketPath() const;

    QLocalSocket *m_socket;
    QTimer *m_retryTimer;
    QString m_clientId;
    bool m_handshaken = false;
    // Cached presence — resent after a reconnect so the activity survives a
    // Discord restart.
    QString m_lastDetails;
    QString m_lastState;
    qint64  m_lastStart = 0;
    bool    m_hasCachedActivity = false;
};

#endif
