// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// Shared test helper: a minimal local HTTP/1.1 server with real Range support,
// used by the HttpDownload and HttpDownloadManager suites. Catch2WithMain gives
// no QCoreApplication, so ensureApp() lazily creates the single instance the
// network classes need for their event loop.

#ifndef BAT_TESTS_HTTPTESTSERVER_H
#define BAT_TESTS_HTTPTESTSERVER_H

#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>
#include <QUrl>
#include <QStandardPaths>
#include <utility>

namespace httptest {

inline QCoreApplication &ensureApp()
{
    static int   argc = 1;
    static char  arg0[] = "bat_httptest";
    static char *argv[] = { arg0, nullptr };
    static QCoreApplication *a = [] {
        // Keep every suite off the real user AppData (the manager persists a
        // sidecar there); tests get an isolated temp store instead.
        QStandardPaths::setTestModeEnabled(true);
        return new QCoreApplication(argc, argv);
    }();
    return *a;
}

inline QByteArray makePayload(int n)
{
    QByteArray p(n, Qt::Uninitialized);
    for (int i = 0; i < n; ++i)
        p[i] = char((i * 31 + 7) & 0xFF);
    return p;
}

// One request per connection. Honours Range for 206; `ignoreRanges` forces 200
// (single-stream fallback); `truncate` advertises the honest length but hangs up
// one byte short (integrity gate). Optional Content-Disposition for naming.
class RangeServer : public QTcpServer
{
public:
    QByteArray payload;
    bool ignoreRanges = false;
    bool truncate = false;
    QByteArray contentDisposition;

    explicit RangeServer(QByteArray body, QObject *parent = nullptr)
        : QTcpServer((ensureApp(), parent)), payload(std::move(body)) {}

    QUrl url() const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1/file.bin").arg(serverPort()));
    }

protected:
    void incomingConnection(qintptr handle) override
    {
        auto *sock = new QTcpSocket(this);
        sock->setSocketDescriptor(handle);
        connect(sock, &QTcpSocket::readyRead, this, [this, sock]() {
            sock->setProperty("buf", sock->property("buf").toByteArray() + sock->readAll());
            const QByteArray buf = sock->property("buf").toByteArray();
            const int hdrEnd = buf.indexOf("\r\n\r\n");
            if (hdrEnd < 0) return;
            respond(sock, buf.left(hdrEnd));
        });
        connect(sock, &QTcpSocket::disconnected, sock, &QObject::deleteLater);
    }

private:
    void respond(QTcpSocket *sock, const QByteArray &headers)
    {
        long long from = 0, to = payload.size() - 1;
        bool ranged = false;
        for (const QByteArray &line : headers.split('\n')) {
            const QByteArray l = line.trimmed();
            if (l.toLower().startsWith("range:")) {
                const QByteArray spec = l.mid(l.indexOf('=') + 1).trimmed();
                const int dash = spec.indexOf('-');
                if (dash >= 0) {
                    from = spec.left(dash).toLongLong();
                    const QByteArray tail = spec.mid(dash + 1).trimmed();
                    if (!tail.isEmpty()) to = tail.toLongLong();
                    ranged = true;
                }
            }
        }
        const bool isProbe = ranged && from == 0 && to == 0;

        QByteArray body, head;
        qint64 contentLen;
        if (ranged && !ignoreRanges) {
            if (to >= payload.size()) to = payload.size() - 1;
            body = payload.mid(int(from), int(to - from + 1));
            contentLen = body.size();
            head = "HTTP/1.1 206 Partial Content\r\n";
            head += "Accept-Ranges: bytes\r\n";
            head += QByteArray("Content-Range: bytes ") + QByteArray::number(from)
                    + '-' + QByteArray::number(to) + '/'
                    + QByteArray::number(payload.size()) + "\r\n";
        } else {
            body = payload;
            contentLen = body.size();
            head = "HTTP/1.1 200 OK\r\n";
        }
        if (truncate && !isProbe && body.size() > 0) body.chop(1);
        head += QByteArray("Content-Length: ") + QByteArray::number(contentLen) + "\r\n";
        if (!contentDisposition.isEmpty())
            head += "Content-Disposition: " + contentDisposition + "\r\n";
        head += "Connection: close\r\n\r\n";

        sock->write(head);
        sock->write(body);
        sock->flush();
        sock->disconnectFromHost();
    }
};

} // namespace httptest

#endif
