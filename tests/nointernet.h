// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// For suites with no case meant to reach the internet, whose code under test
// still fires requests on the side (GeoIP lookups for VPN endpoints, torrent
// provider searches). Left alone, those resolve and handshake with real hosts
// and are often still in flight when the case ends; on Windows a process that
// exits with a stalled lookup can sit there until ctest's timeout.

#ifndef BAT_TESTS_NOINTERNET_H
#define BAT_TESTS_NOINTERNET_H

#include <QNetworkProxy>

namespace nointernet {

// A proxy on a closed loopback port: every QNetworkAccessManager request fails
// locally at connect, with no DNS lookup or TLS for the real host. Qt never
// proxies loopback, so local test servers are unaffected. Call after the app
// object exists.
inline void install()
{
    QNetworkProxy::setApplicationProxy(
        QNetworkProxy(QNetworkProxy::HttpProxy, QStringLiteral("127.0.0.1"), 1));
}

} // namespace nointernet

#endif
