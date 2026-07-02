#include <catch2/catch_test_macros.hpp>
#include "torrent/proxycontroller.h"

// Characterization tests for the proxy settings-builder extracted from
// SessionManager. Security-relevant (leak-proof mode), so pin the exact
// libtorrent settings each config produces.

namespace lt = libtorrent;
using bat::buildProxySettings;
using sp = lt::settings_pack;

TEST_CASE("proxy type 0 (none) clears proxying and restores leak vectors") {
    const lt::settings_pack p = buildProxySettings(0, "", 0, "", "", true, false);
    REQUIRE(p.get_int(sp::proxy_type) == sp::none);
    REQUIRE(p.get_bool(sp::proxy_peer_connections) == false);
    REQUIRE(p.get_bool(sp::proxy_tracker_connections) == false);
    REQUIRE(p.get_bool(sp::proxy_hostnames) == false);
    REQUIRE(p.get_bool(sp::enable_upnp) == true);
    REQUIRE(p.get_bool(sp::enable_natpmp) == true);
    REQUIRE(p.get_bool(sp::enable_lsd) == true);
}

TEST_CASE("proxy type 0 forwards the session anonymous-mode toggle") {
    REQUIRE(buildProxySettings(0, "", 0, "", "", true, true).get_bool(sp::anonymous_mode) == true);
    REQUIRE(buildProxySettings(0, "", 0, "", "", true, false).get_bool(sp::anonymous_mode) == false);
}

TEST_CASE("SOCKS5 with credentials uses socks5_pw and routes everything") {
    const lt::settings_pack p = buildProxySettings(1, "10.64.0.1", 1080, "u", "pw", true, false);
    REQUIRE(p.get_int(sp::proxy_type) == sp::socks5_pw);
    REQUIRE(p.get_str(sp::proxy_hostname) == "10.64.0.1");
    REQUIRE(p.get_int(sp::proxy_port) == 1080);
    REQUIRE(p.get_str(sp::proxy_username) == "u");
    REQUIRE(p.get_str(sp::proxy_password) == "pw");
    REQUIRE(p.get_bool(sp::proxy_peer_connections) == true);
    REQUIRE(p.get_bool(sp::proxy_tracker_connections) == true);
    REQUIRE(p.get_bool(sp::proxy_hostnames) == true);
}

TEST_CASE("credential-less SOCKS5/HTTP drop the _pw variant") {
    REQUIRE(buildProxySettings(1, "h", 9050, "", "", true, false).get_int(sp::proxy_type) == sp::socks5);
    REQUIRE(buildProxySettings(2, "h", 8080, "", "", true, false).get_int(sp::proxy_type) == sp::http);
    REQUIRE(buildProxySettings(2, "h", 8080, "u", "pw", true, false).get_int(sp::proxy_type) == sp::http_pw);
}

TEST_CASE("leak-proof tunnel kills UPnP/NAT-PMP/LSD and forces anonymous") {
    const lt::settings_pack p = buildProxySettings(1, "h", 1080, "", "", true, false);
    REQUIRE(p.get_bool(sp::enable_upnp) == false);
    REQUIRE(p.get_bool(sp::enable_natpmp) == false);
    REQUIRE(p.get_bool(sp::enable_lsd) == false);
    REQUIRE(p.get_bool(sp::anonymous_mode) == true);
}

TEST_CASE("non-leak-proof tunnel leaves the leak vectors untouched") {
    const lt::settings_pack p = buildProxySettings(1, "h", 1080, "", "", false, false);
    // When leak-proof is off the builder must not set these at all.
    REQUIRE_FALSE(p.has_val(sp::enable_upnp));
    REQUIRE_FALSE(p.has_val(sp::enable_natpmp));
    REQUIRE_FALSE(p.has_val(sp::enable_lsd));
    REQUIRE_FALSE(p.has_val(sp::anonymous_mode));
}
