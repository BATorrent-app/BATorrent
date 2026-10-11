// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "ipc/mediachannel.h"
#include <QLocalServer>
#include <QLocalSocket>

#ifdef Q_OS_WIN
#  include <QWinEventNotifier>
#  include <windows.h>
#  include <sddl.h>
#endif

MediaChannel::MediaChannel(QObject *parent) : QObject(parent) {}

#ifndef Q_OS_WIN

MediaChannel::~MediaChannel() = default;

bool MediaChannel::listen(const QString &name, const QString &)
{
    m_server = new QLocalServer(this);
    m_server->setSocketOptions(QLocalServer::UserAccessOption);
    QLocalServer::removeServer(name);
    if (!m_server->listen(name)) return false;
    connect(m_server, &QLocalServer::newConnection, this, [this] {
        QLocalSocket *sock = m_server->nextPendingConnection();
        if (!sock) return;
        m_server->close();   // one child per channel
        emit connected(sock);
    });
    return true;
}

#else

namespace {

QString currentUserSid()
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return {};
    DWORD len = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &len);
    QByteArray buf(int(len), '\0');
    QString out;
    if (GetTokenInformation(token, TokenUser, buf.data(), len, &len)) {
        LPWSTR s = nullptr;
        if (ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER *>(buf.data())->User.Sid, &s)) {
            out = QString::fromWCharArray(s);
            LocalFree(s);
        }
    }
    CloseHandle(token);
    return out;
}

bool validSid(const QString &sid)
{
    if (!sid.startsWith(QLatin1String("S-1-"))) return false;
    for (QChar c : sid) if (!(c.isDigit() || c == QLatin1Char('-') || c == QLatin1Char('S'))) return false;
    return true;
}

}

MediaChannel::~MediaChannel()
{
    if (m_overlapped) {
        auto *ov = static_cast<OVERLAPPED *>(m_overlapped);
        if (m_pipe) CancelIoEx(m_pipe, ov);
        CloseHandle(ov->hEvent);
        delete ov;
    }
    if (m_pipe) CloseHandle(m_pipe);
}

bool MediaChannel::listen(const QString &name, const QString &peerSid)
{
    const QString user = currentUserSid();
    if (!validSid(user) || (!peerSid.isEmpty() && !validSid(peerSid))) return false;
    QString sddl = QStringLiteral("D:P(A;;GA;;;SY)(A;;GA;;;%1)").arg(user);
    if (!peerSid.isEmpty()) sddl += QStringLiteral("(A;;GRGW;;;%1)").arg(peerSid);
    sddl += QStringLiteral("S:(ML;;NW;;;LW)");   // AppContainers run at low integrity

    PSECURITY_DESCRIPTOR sd = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            reinterpret_cast<LPCWSTR>(sddl.utf16()), SDDL_REVISION_1, &sd, nullptr))
        return false;
    SECURITY_ATTRIBUTES sa{ sizeof sa, sd, FALSE };
    const QString path = QStringLiteral("\\\\.\\pipe\\") + name;
    m_pipe = CreateNamedPipeW(reinterpret_cast<LPCWSTR>(path.utf16()),
                              PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
                              PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
                              1, 64 * 1024, 64 * 1024, 0, &sa);
    LocalFree(sd);
    if (m_pipe == INVALID_HANDLE_VALUE) { m_pipe = nullptr; return false; }

    auto *ov = new OVERLAPPED{};
    ov->hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    m_overlapped = ov;
    if (ConnectNamedPipe(m_pipe, ov)) { adopt(); return true; }
    const DWORD err = GetLastError();
    if (err == ERROR_PIPE_CONNECTED) { adopt(); return true; }
    if (err != ERROR_IO_PENDING) return false;
    m_notifier = new QWinEventNotifier(ov->hEvent, this);
    connect(m_notifier, &QWinEventNotifier::activated, this, [this] {
        m_notifier->setEnabled(false);
        DWORD n = 0;
        if (GetOverlappedResult(m_pipe, static_cast<OVERLAPPED *>(m_overlapped), &n, FALSE)) adopt();
    });
    return true;
}

void MediaChannel::adopt()
{
    auto *sock = new QLocalSocket(this);
    if (!sock->setSocketDescriptor(qintptr(m_pipe), QLocalSocket::ConnectedState)) {
        delete sock;
        return;
    }
    m_pipe = nullptr;   // the socket owns it now
    emit connected(sock);
}

#endif
