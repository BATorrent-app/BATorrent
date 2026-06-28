// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef STREAMSERVER_H
#define STREAMSERVER_H

#include <QObject>

class IEngine;
class QTcpServer;

// Local HTTP server (127.0.0.1, ephemeral port) that streams a torrent file
// with byte-range support — `GET /stream/<infoHash>/<fileIndex>`. Serves bytes
// straight off disk as pieces complete (download-while-watch), prioritizing the
// requested window so seeks land fast. Feeds the embedded PlayerWindow.
class StreamServer : public QObject
{
    Q_OBJECT
public:
    explicit StreamServer(IEngine *session, QObject *parent = nullptr);
    ~StreamServer();

    bool start();              // listen on 127.0.0.1 with an ephemeral port
    void stop();
    bool isRunning() const;
    quint16 port() const { return m_port; }

private:
    void onNewConnection();

    IEngine *m_session;
    QTcpServer *m_server = nullptr;
    quint16 m_port = 0;
};

#endif
