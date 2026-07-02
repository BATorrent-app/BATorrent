// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef GEOIPDB_H
#define GEOIPDB_H

#include <QString>
#include <cstdint>
#include <vector>

// Local, offline IP→country lookup over a db-ip.com Lite (CC-BY) IPv4 range
// table. Loaded once into a start-sorted vector; lookup is a binary search —
// no network, unlike GeoIpResolver which hits ipinfo.io just for peer flags.
// This is the data foundation for locality-based peer biasing: classifying
// every peer the tracker/DHT returns as same-country or not, at swarm scale.
class GeoIpDb
{
public:
    // Load ranges from db-ip Lite CSV text. Each line is startIp,endIp,CC
    // (optionally quoted). IPv6 rows and malformed lines are skipped, not fatal.
    // Returns the number of IPv4 ranges parsed. Replaces any previous data.
    int loadCsv(const QString &csv);

    bool ready() const { return !m_ranges.empty(); }
    int size() const { return static_cast<int>(m_ranges.size()); }

    // 2-letter uppercase country code for a dotted IPv4 string, or "" if the
    // input is malformed or falls outside every known range.
    QString country(const QString &ipv4) const;

    // Allocation-free lookup for a host-order IPv4 int. Writes the 2-char
    // country code into out[2] and returns true, or returns false if the IP is
    // outside every range. The peer-ranking hot path (compare_peer) uses these.
    bool lookup(uint32_t ipv4, char out[2]) const;

    // True iff the host-order IPv4 resolves to country code c0c1. No allocation
    // — called once per peer per connect-candidate ranking.
    bool inCountry(uint32_t ipv4, char c0, char c1) const;

    // Parse "1.2.3.4" → host-order uint32. Returns false on malformed input
    // (wrong octet count, non-numeric, or an octet > 255).
    static bool parseIpv4(const QString &s, uint32_t &out);

private:
    struct Range {
        uint32_t start;
        uint32_t end;
        char cc[2];
    };
    // Range covering ip (start <= ip <= end), or nullptr if none.
    const Range *find(uint32_t ip) const;
    std::vector<Range> m_ranges;   // sorted by start, non-overlapping after load
};

#endif // GEOIPDB_H
