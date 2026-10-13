// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/discovery/trackerscrape.h"

#include <QHostInfo>
#include <QRandomGenerator>
#include <QTimer>
#include <QUdpSocket>
#include <QUrl>
#include <QUrlQuery>
#include <QtEndian>

#include <algorithm>
#include <memory>
#include <tuple>

namespace {

constexpr quint64 kProtocolId = 0x41727101980ULL;
constexpr quint32 kActionConnect = 0;
constexpr quint32 kActionScrape = 2;
constexpr int kMaxPerScrape = 70;    // a scrape reply must fit one datagram
constexpr int kMaxTrackers = 6;      // per call; a magnet lists twenty, most dead
constexpr int kTimeoutMs = 6000;

void put32(QByteArray &b, quint32 v) { v = qToBigEndian(v); b.append(reinterpret_cast<const char *>(&v), 4); }
void put64(QByteArray &b, quint64 v) { v = qToBigEndian(v); b.append(reinterpret_cast<const char *>(&v), 8); }
quint32 get32(const QByteArray &b, int at) { return qFromBigEndian<quint32>(b.constData() + at); }

} // namespace

namespace TrackerScrape {

QStringList udpTrackers(const QString &magnet)
{
    QStringList out;
    const QUrlQuery q(QUrl(magnet).query(QUrl::FullyEncoded));
    for (const QString &tr : q.allQueryItemValues(QStringLiteral("tr"), QUrl::FullyDecoded)) {
        const QUrl u(tr);
        if (u.scheme().compare(QLatin1String("udp"), Qt::CaseInsensitive) != 0 || u.host().isEmpty() || u.port() <= 0)
            continue;
        const QString key = u.host().toLower() + QLatin1Char(':') + QString::number(u.port());
        if (!out.contains(key)) out << key;
    }
    return out;
}

QByteArray connectRequest(quint32 transaction)
{
    QByteArray b;
    put64(b, kProtocolId);
    put32(b, kActionConnect);
    put32(b, transaction);
    return b;
}

bool parseConnect(const QByteArray &reply, quint32 transaction, quint64 *connectionId)
{
    if (reply.size() < 16 || get32(reply, 0) != kActionConnect || get32(reply, 4) != transaction)
        return false;
    *connectionId = qFromBigEndian<quint64>(reply.constData() + 8);
    return true;
}

QByteArray scrapeRequest(quint64 connectionId, quint32 transaction, const QList<QByteArray> &hashes)
{
    QByteArray b;
    put64(b, connectionId);
    put32(b, kActionScrape);
    put32(b, transaction);
    for (const QByteArray &h : hashes) b.append(h);
    return b;
}

QList<Counts> parseScrape(const QByteArray &reply, quint32 transaction, int hashCount)
{
    if (reply.size() < 8 + 12 * hashCount || get32(reply, 0) != kActionScrape || get32(reply, 4) != transaction)
        return {};
    QList<Counts> out;
    for (int i = 0; i < hashCount; ++i) {
        const int at = 8 + 12 * i;   // seeders, completed, leechers
        out << Counts{ int(get32(reply, at)), int(get32(reply, at + 8)) };
    }
    return out;
}

} // namespace TrackerScrape

TrackerScraper::TrackerScraper(QObject *parent) : QObject(parent) {}

void TrackerScraper::scrape(const QHash<QString, QStringList> &trackersByHash)
{
    // The trackers most magnets share go first: one round trip covers more rows.
    QHash<QString, QStringList> hashesByTracker;
    for (auto it = trackersByHash.cbegin(); it != trackersByHash.cend(); ++it)
        for (const QString &t : it.value())
            hashesByTracker[t] << it.key();
    QStringList trackers = hashesByTracker.keys();
    std::sort(trackers.begin(), trackers.end(), [&](const QString &a, const QString &b) {
        const int na = hashesByTracker.value(a).size(), nb = hashesByTracker.value(b).size();
        return na != nb ? na > nb : a < b;
    });
    trackers = trackers.mid(0, kMaxTrackers);
    m_pending = trackers.size();
    if (m_pending == 0) {
        QTimer::singleShot(0, this, &TrackerScraper::finished);
        return;
    }
    for (const QString &t : trackers)
        askTracker(t, hashesByTracker.value(t));
}

void TrackerScraper::doneOne()
{
    if (--m_pending == 0) emit finished();
}

void TrackerScraper::askTracker(const QString &tracker, const QStringList &hashes)
{
    const QString host = tracker.section(QLatin1Char(':'), 0, -2);
    const quint16 port = quint16(tracker.section(QLatin1Char(':'), -1).toUInt());
    auto *sock = new QUdpSocket(this);
    auto *timer = new QTimer(sock);
    timer->setSingleShot(true);
    auto done = std::make_shared<bool>(false);
    auto finish = [this, sock, done]() {
        if (*done) return;
        *done = true;
        sock->deleteLater();
        doneOne();
    };
    connect(timer, &QTimer::timeout, sock, finish);
    timer->start(kTimeoutMs);

    QHostInfo::lookupHost(host, sock, [this, sock, port, hashes, finish](const QHostInfo &info) {
        QHostAddress addr;
        for (const QHostAddress &a : info.addresses())
            if (a.protocol() == QAbstractSocket::IPv4Protocol) { addr = a; break; }
        if (addr.isNull()) { finish(); return; }
        if (!sock->bind(QHostAddress::AnyIPv4, 0)) { finish(); return; }

        // Batches of hashes, each scraped in turn on the one connection id.
        QList<QList<QByteArray>> batches;
        QList<QStringList> batchKeys;
        for (int i = 0; i < hashes.size(); i += kMaxPerScrape) {
            QList<QByteArray> raw;
            const QStringList keys = hashes.mid(i, kMaxPerScrape);
            for (const QString &h : keys) raw << QByteArray::fromHex(h.toLatin1());
            batches << raw;
            batchKeys << keys;
        }
        auto state = std::make_shared<std::tuple<quint32, quint64, int>>(
            QRandomGenerator::global()->generate(), 0, -1);   // transaction, connection id, batch
        auto send = [sock, addr, port, state, batches]() {
            auto &[tx, cid, batch] = *state;
            tx = QRandomGenerator::global()->generate();
            sock->writeDatagram(batch < 0 ? TrackerScrape::connectRequest(tx)
                                          : TrackerScrape::scrapeRequest(cid, tx, batches.at(batch)),
                                addr, port);
        };
        connect(sock, &QUdpSocket::readyRead, sock, [this, sock, state, batches, batchKeys, send, finish]() {
            while (sock->hasPendingDatagrams()) {
                QByteArray reply(int(sock->pendingDatagramSize()), Qt::Uninitialized);
                sock->readDatagram(reply.data(), reply.size());
                auto &[tx, cid, batch] = *state;
                if (batch < 0) {
                    if (!TrackerScrape::parseConnect(reply, tx, &cid)) continue;
                } else {
                    const QList<TrackerScrape::Counts> got =
                        TrackerScrape::parseScrape(reply, tx, int(batches.at(batch).size()));
                    if (got.isEmpty()) continue;
                    QHash<QString, QPair<int, int>> byHash;
                    for (int i = 0; i < got.size(); ++i)
                        byHash.insert(batchKeys.at(batch).at(i), { got.at(i).seeders, got.at(i).leechers });
                    emit counts(byHash);
                }
                if (++batch >= batches.size()) { finish(); return; }
                send();
            }
        });
        send();
    });
}
