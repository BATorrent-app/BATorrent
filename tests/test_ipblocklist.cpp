#include <catch2/catch_test_macros.hpp>
#include "torrent/ipblocklist.h"

#include <boost/asio/ip/address.hpp>

namespace lt = libtorrent;

static bool blocks(const lt::ip_filter &f, const char *ip) {
    return f.access(boost::asio::ip::make_address(ip)) & lt::ip_filter::blocked;
}

TEST_CASE("blocklist: empty and comment-only input adds no rules") {
    int n = -1;
    bat::parseP2pBlocklist("", &n);
    REQUIRE(n == 0);
    bat::parseP2pBlocklist("# a comment\n\n   \n# another", &n);
    REQUIRE(n == 0);
}

TEST_CASE("blocklist: P2P description:range line blocks the range") {
    int n = 0;
    const lt::ip_filter f = bat::parseP2pBlocklist("Some Org:1.2.3.4-1.2.3.10", &n);
    REQUIRE(n == 1);
    REQUIRE(blocks(f, "1.2.3.4"));
    REQUIRE(blocks(f, "1.2.3.7"));
    REQUIRE(blocks(f, "1.2.3.10"));
    REQUIRE_FALSE(blocks(f, "1.2.3.11"));
    REQUIRE_FALSE(blocks(f, "9.9.9.9"));
}

TEST_CASE("blocklist: a bare range with no description also parses") {
    int n = 0;
    const lt::ip_filter f = bat::parseP2pBlocklist("10.0.0.0-10.0.0.255", &n);
    REQUIRE(n == 1);
    REQUIRE(blocks(f, "10.0.0.128"));
    REQUIRE_FALSE(blocks(f, "10.0.1.0"));
}

TEST_CASE("blocklist: malformed lines are skipped, valid ones kept") {
    int n = -1;
    const lt::ip_filter f = bat::parseP2pBlocklist(
        "garbage\n"
        "desc:not.an.ip-also.bad\n"
        "missing dash 1.2.3.4\n"
        "Good:8.8.8.0-8.8.8.255\n", &n);
    REQUIRE(n == 1);
    REQUIRE(blocks(f, "8.8.8.8"));
}

TEST_CASE("blocklist: reversed and mixed-family ranges are rejected (no crash)") {
    int n = -1;
    // reversed (start > end) — would assert inside libtorrent's add_rule
    bat::parseP2pBlocklist("Bad:1.2.3.10-1.2.3.4", &n);
    REQUIRE(n == 0);
    // mixed IPv4/IPv6
    bat::parseP2pBlocklist("Bad:1.2.3.4-2001:db8::1", &n);
    REQUIRE(n == 0);
}

TEST_CASE("blocklist: a bare IPv6 range parses") {
    // NOTE: the "description:" prefix heuristic is IPv4-centric (it keys off a dot
    // after the colon), so a *bare* IPv6 range is the supported form — matching the
    // original behavior this was extracted from.
    int n = 0;
    const lt::ip_filter f = bat::parseP2pBlocklist("2001:db8::-2001:db8::ffff", &n);
    REQUIRE(n == 1);
    REQUIRE(blocks(f, "2001:db8::5"));
    REQUIRE_FALSE(blocks(f, "2001:db9::1"));
}
