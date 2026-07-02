// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef IPBLOCKLIST_H
#define IPBLOCKLIST_H

#include <QString>
#include <libtorrent/ip_filter.hpp>

// Parse a peer blocklist (untrusted user-supplied file) into a libtorrent
// ip_filter. Pure and side-effect free so it can be unit-tested and fuzzed:
// the SessionManager reads the file and applies the result to the session.
namespace bat {

// Accepts the common "P2P" text format, one range per line:
//   "some description:1.2.3.4-1.2.3.255"   or   "1.2.3.4-1.2.3.255"
// Blank lines and lines beginning with '#' are ignored; any line that doesn't
// parse to a valid IPv4/IPv6 range is skipped (never throws). If rulesAdded is
// non-null it receives the number of accepted ranges.
libtorrent::ip_filter parseP2pBlocklist(const QString &text, int *rulesAdded = nullptr);

}

#endif // IPBLOCKLIST_H
