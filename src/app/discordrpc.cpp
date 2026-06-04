// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "discordrpc.h"
#include "logger.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QProcessEnvironment>
#include <QUuid>

namespace {
constexpr int kOpHandshake = 0;
constexpr int kOpFrame = 1;
constexpr int kOpClose = 2;
constexpr int kOpPing = 3;
}

DiscordRPC::DiscordRPC(QObject *parent)
    : QObject(parent)
    , m_socket(new QLocalSocket(this))
    , m_retryTimer(new QTimer(this))
{
    connect(m_socket, &QLocalSocket::connected, this, &DiscordRPC::onConnected);
    connect(m_socket, &QLocalSocket::disconnected, this, &DiscordRPC::onDisconnected);
    connect(m_socket, &QLocalSocket::readyRead, this, &DiscordRPC::onReadyRead);
    connect(m_socket, &QLocalSocket::errorOccurred, this, [this](QLocalSocket::LocalSocketError) {
        // Discord not running, socket missing, etc. Schedule a retry; the
        // user shouldn't see anything fail.
        if (m_retryTimer->isActive()) return;
        m_retryTimer->start(30000); // 30s — Discord either started or didn't
    });
    connect(m_retryTimer, &QTimer::timeout, this, &DiscordRPC::tryConnect);
    m_retryTimer->setSingleShot(false);
}

void DiscordRPC::setClientId(const QString &id)
{
    if (id == m_clientId) return;
    m_clientId = id.trimmed();
    if (m_clientId.isEmpty()) {
        // Feature turned off — disconnect cleanly.
        m_retryTimer->stop();
        if (m_socket->state() != QLocalSocket::UnconnectedState)
            m_socket->disconnectFromServer();
        m_handshaken = false;
        return;
    }
    tryConnect();
}

void DiscordRPC::setActivity(const QString &details, const QString &state, qint64 startEpoch)
{
    m_lastDetails = details;
    m_lastState = state;
    m_lastStart = startEpoch;
    m_hasCachedActivity = true;
    if (!m_handshaken) return;

    QJsonObject activity;
    if (!details.isEmpty()) activity.insert("details", details);
    if (!state.isEmpty())   activity.insert("state", state);
    if (startEpoch > 0) {
        QJsonObject timestamps;
        timestamps.insert("start", startEpoch);
        activity.insert("timestamps", timestamps);
    }
    // Assets — the "logo" key must match an image uploaded to the Discord
    // Application's Rich Presence Art Assets (discord.com/developers →
    // your app → Rich Presence → Art Assets → upload logo1.png as "logo").
    QJsonObject assets;
    assets.insert("large_image", "logo");
    assets.insert("large_text", QStringLiteral("BATorrent %1").arg(
        QCoreApplication::applicationVersion()));
    activity.insert("assets", assets);
    // Buttons — up to 2 clickable links shown on the activity card. This
    // is the organic marketing channel: anyone who sees a friend's profile
    // showing BATorrent activity can click straight to the releases page.
    QJsonArray buttons;
    QJsonObject dlBtn;
    dlBtn.insert("label", "Download BATorrent");
    dlBtn.insert("url", "https://github.com/Mateuscruz19/BATorrent/releases/latest");
    buttons.append(dlBtn);
    QJsonObject ghBtn;
    ghBtn.insert("label", "View on GitHub");
    ghBtn.insert("url", "https://github.com/Mateuscruz19/BATorrent");
    buttons.append(ghBtn);
    activity.insert("buttons", buttons);
    QJsonObject args;
    args.insert("pid", static_cast<qint64>(QCoreApplication::applicationPid()));
    args.insert("activity", activity);
    QJsonObject payload;
    payload.insert("cmd", "SET_ACTIVITY");
    payload.insert("args", args);
    payload.insert("nonce", QUuid::createUuid().toString(QUuid::WithoutBraces));
    sendFrame(kOpFrame, QJsonDocument(payload).toJson(QJsonDocument::Compact));
}

void DiscordRPC::clearActivity()
{
    m_hasCachedActivity = false;
    m_lastDetails.clear();
    m_lastState.clear();
    if (!m_handshaken) return;
    QJsonObject args;
    args.insert("pid", static_cast<qint64>(QCoreApplication::applicationPid()));
    QJsonObject payload;
    payload.insert("cmd", "SET_ACTIVITY");
    payload.insert("args", args);
    payload.insert("nonce", QUuid::createUuid().toString(QUuid::WithoutBraces));
    sendFrame(kOpFrame, QJsonDocument(payload).toJson(QJsonDocument::Compact));
}

void DiscordRPC::tryConnect()
{
    if (m_clientId.isEmpty()) { m_retryTimer->stop(); return; }
    if (m_socket->state() == QLocalSocket::ConnectedState) return;
    const QString path = socketPath();
    if (path.isEmpty()) return;
    m_socket->connectToServer(path);
}

void DiscordRPC::onConnected()
{
    m_retryTimer->stop();
    // Handshake: v1 + clientId.
    QJsonObject obj;
    obj.insert("v", 1);
    obj.insert("client_id", m_clientId);
    sendFrame(kOpHandshake, QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void DiscordRPC::onDisconnected()
{
    m_handshaken = false;
    if (!m_clientId.isEmpty())
        m_retryTimer->start(30000);
}

void DiscordRPC::onReadyRead()
{
    // We mostly don't care what Discord replies with — DISPATCH READY means
    // handshake succeeded, errors come as opcode=2 (close). Drain the buffer
    // and replay the cached activity on first READY.
    while (m_socket->bytesAvailable() >= 8) {
        const auto header = m_socket->read(8);
        quint32 op, len;
        memcpy(&op, header.constData(), 4);
        memcpy(&len, header.constData() + 4, 4);
        const auto body = m_socket->read(len);
        if (op == kOpClose) {
            Logger::instance().log(Logger::Warning,
                QStringLiteral("Discord IPC closed by remote: %1").arg(QString::fromUtf8(body)));
            m_socket->disconnectFromServer();
            continue;
        }
        if (!m_handshaken) {
            m_handshaken = true;
            if (m_hasCachedActivity)
                setActivity(m_lastDetails, m_lastState, m_lastStart);
        }
    }
}

void DiscordRPC::sendFrame(int op, const QByteArray &json)
{
    if (m_socket->state() != QLocalSocket::ConnectedState) return;
    quint32 op32 = static_cast<quint32>(op);
    quint32 len32 = static_cast<quint32>(json.size());
    QByteArray header(8, 0);
    memcpy(header.data(), &op32, 4);
    memcpy(header.data() + 4, &len32, 4);
    m_socket->write(header);
    m_socket->write(json);
}

QString DiscordRPC::socketPath() const
{
#ifdef Q_OS_WIN
    // Qt's QLocalSocket on Windows expects the suffix (after \\.\pipe\).
    return QStringLiteral("discord-ipc-0");
#else
    // Discord opens up to 10 IPC slots; we try just slot 0 since that's the
    // only one mainline Discord uses. Plain Discord, Canary, PTB.
    const QStringList bases = {
        QProcessEnvironment::systemEnvironment().value("XDG_RUNTIME_DIR"),
        QProcessEnvironment::systemEnvironment().value("TMPDIR"),
        QProcessEnvironment::systemEnvironment().value("TMP"),
        QProcessEnvironment::systemEnvironment().value("TEMP"),
        QStringLiteral("/tmp"),
    };
    for (const QString &base : bases) {
        if (base.isEmpty()) continue;
        const QString candidate = QDir(base).filePath("discord-ipc-0");
        if (QFile::exists(candidate)) return candidate;
    }
    return {};
#endif
}
