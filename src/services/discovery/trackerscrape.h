// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
#include <QStringList>

// Seeders and leechers for a set of infohashes, asked of the UDP trackers in
// their magnets (BEP 15 scrape): the same swarm a download would join, so a
// catalog row shows what the user will get rather than an indexer's guess.
namespace TrackerScrape {

struct Counts {
    int seeders = 0;
    int leechers = 0;
};

// "host:port" for each udp:// tracker in a magnet, in order, without repeats.
QStringList udpTrackers(const QString &magnet);

QByteArray connectRequest(quint32 transaction);
bool parseConnect(const QByteArray &reply, quint32 transaction, quint64 *connectionId);
QByteArray scrapeRequest(quint64 connectionId, quint32 transaction, const QList<QByteArray> &hashes);
// One entry per hash asked, in the same order; empty when the reply is not ours.
QList<Counts> parseScrape(const QByteArray &reply, quint32 transaction, int hashCount);

} // namespace TrackerScrape

class TrackerScraper : public QObject
{
    Q_OBJECT
public:
    explicit TrackerScraper(QObject *parent = nullptr);

    // hash (40 hex) → its magnet's udp trackers. Each tracker is asked once for
    // all its hashes; results arrive per tracker, then finished() once.
    void scrape(const QHash<QString, QStringList> &trackersByHash);

signals:
    void counts(const QHash<QString, QPair<int, int>> &byHash);   // hash → (seeds, leech)
    void finished();

private:
    void askTracker(const QString &tracker, const QStringList &hashes);
    void doneOne();
    int m_pending = 0;
};
