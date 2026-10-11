// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_MEDIACHANNEL_H
#define BATORRENT_MEDIACHANNEL_H

#include <QObject>
#include <QString>

class QLocalServer;
class QLocalSocket;
class QWinEventNotifier;

// The UI's listening end for one media child. On Windows the pipe's DACL
// names the child's AppContainer and carries a low-integrity label: a
// contained process can open nothing else, and QLocalServer can't say either.
class MediaChannel : public QObject
{
    Q_OBJECT
public:
    explicit MediaChannel(QObject *parent = nullptr);
    ~MediaChannel() override;

    // `peerSid`: the AppContainer allowed in (Windows); ignored elsewhere.
    bool listen(const QString &name, const QString &peerSid = {});

signals:
    void connected(QLocalSocket *socket);

private:
    QLocalServer *m_server = nullptr;
#ifdef Q_OS_WIN
    void adopt();
    void *m_pipe = nullptr;
    void *m_overlapped = nullptr;
    QWinEventNotifier *m_notifier = nullptr;
#endif
};

#endif
