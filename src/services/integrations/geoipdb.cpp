// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/integrations/geoipdb.h"

#include <QStringList>
#include <algorithm>

bool GeoIpDb::parseIpv4(const QString &s, uint32_t &out)
{
    uint32_t acc = 0;
    int octets = 0;
    int val = -1;   // -1 = no digit seen yet in this octet
    for (int i = 0; i <= s.size(); ++i) {
        const QChar ch = (i < s.size()) ? s.at(i) : QChar('.');   // sentinel to flush the last octet
        if (ch == QChar('.')) {
            if (val < 0 || val > 255 || octets >= 4) return false;
            acc = (acc << 8) | static_cast<uint32_t>(val);
            ++octets;
            val = -1;
        } else if (ch >= QChar('0') && ch <= QChar('9')) {
            val = (val < 0 ? 0 : val) * 10 + (ch.unicode() - u'0');
            if (val > 255) return false;
        } else {
            return false;
        }
    }
    if (octets != 4) return false;
    out = acc;
    return true;
}

int GeoIpDb::loadCsv(const QString &csv)
{
    m_ranges.clear();
    const QStringList lines = csv.split(QChar('\n'), Qt::SkipEmptyParts);
    m_ranges.reserve(lines.size());

    auto unquote = [](QString v) {
        v = v.trimmed();
        if (v.size() >= 2 && v.startsWith(QChar('"')) && v.endsWith(QChar('"')))
            v = v.mid(1, v.size() - 2);
        return v.trimmed();
    };

    for (const QString &line : lines) {
        // db-ip Lite: startIp,endIp,CC — fields may be quoted. IPv6 rows carry
        // ':' in the address and are skipped (IPv4 biasing only for now).
        const int c1 = line.indexOf(QChar(','));
        if (c1 < 0) continue;
        const int c2 = line.indexOf(QChar(','), c1 + 1);
        if (c2 < 0) continue;

        const QString s0 = unquote(line.left(c1));
        const QString s1 = unquote(line.mid(c1 + 1, c2 - c1 - 1));
        const QString sc = unquote(line.mid(c2 + 1));
        if (s0.contains(QChar(':')) || sc.size() != 2) continue;

        uint32_t a = 0, b = 0;
        if (!parseIpv4(s0, a) || !parseIpv4(s1, b)) continue;
        if (b < a) std::swap(a, b);

        Range r;
        r.start = a;
        r.end = b;
        r.cc[0] = sc.at(0).toUpper().toLatin1();
        r.cc[1] = sc.at(1).toUpper().toLatin1();
        m_ranges.push_back(r);
    }

    std::sort(m_ranges.begin(), m_ranges.end(),
              [](const Range &x, const Range &y) { return x.start < y.start; });
    return static_cast<int>(m_ranges.size());
}

const GeoIpDb::Range *GeoIpDb::find(uint32_t ip) const
{
    if (m_ranges.empty()) return nullptr;
    // Largest range whose start <= ip, then confirm ip is within its end.
    auto it = std::upper_bound(m_ranges.begin(), m_ranges.end(), ip,
                               [](uint32_t v, const Range &r) { return v < r.start; });
    if (it == m_ranges.begin()) return nullptr;
    --it;
    return ip <= it->end ? &*it : nullptr;
}

bool GeoIpDb::lookup(uint32_t ipv4, char out[2]) const
{
    const Range *r = find(ipv4);
    if (!r) return false;
    out[0] = r->cc[0];
    out[1] = r->cc[1];
    return true;
}

bool GeoIpDb::inCountry(uint32_t ipv4, char c0, char c1) const
{
    const Range *r = find(ipv4);
    return r && r->cc[0] == c0 && r->cc[1] == c1;
}

QString GeoIpDb::country(const QString &ipv4) const
{
    uint32_t ip = 0;
    if (!parseIpv4(ipv4, ip)) return {};
    const Range *r = find(ip);
    return r ? QString::fromLatin1(r->cc, 2) : QString{};
}
