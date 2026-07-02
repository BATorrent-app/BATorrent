// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "torrent/ipblocklist.h"

#include <QStringList>
#include <boost/asio/ip/address.hpp>

namespace bat {

libtorrent::ip_filter parseP2pBlocklist(const QString &text, int *rulesAdded)
{
    namespace lt = libtorrent;
    lt::ip_filter filter;
    int count = 0;

    const QStringList lines = text.split(QChar('\n'));
    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        // P2P format: "description:startIP-endIP". Only treat the last colon as
        // the description separator when what follows looks like an IP (has a dot
        // after it) — otherwise the whole line is the range.
        const int colonIdx = line.lastIndexOf(':');
        QString range;
        if (colonIdx >= 0 && line.indexOf('.', colonIdx) > 0)
            range = line.mid(colonIdx + 1).trimmed();
        else
            range = line;

        const int dashIdx = range.indexOf('-');
        if (dashIdx < 0)
            continue;

        const QString startIp = range.left(dashIdx).trimmed();
        const QString endIp = range.mid(dashIdx + 1).trimmed();

        try {
            boost::system::error_code ec1, ec2;
            auto addr1 = boost::asio::ip::make_address(startIp.toStdString(), ec1);
            auto addr2 = boost::asio::ip::make_address(endIp.toStdString(), ec2);
            if (ec1 || ec2)
                continue;
            // add_rule asserts on a mixed-family or reversed (start > end) range;
            // this is untrusted file input, so reject those instead of crashing.
            if (addr1.is_v4() != addr2.is_v4() || addr2 < addr1)
                continue;
            filter.add_rule(addr1, addr2, lt::ip_filter::blocked);
            ++count;
        } catch (...) { /* skip an unparseable blocklist range */ }
    }

    if (rulesAdded)
        *rulesAdded = count;
    return filter;
}

}
