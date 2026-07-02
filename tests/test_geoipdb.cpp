#include <catch2/catch_test_macros.hpp>
#include "services/integrations/geoipdb.h"

TEST_CASE("parseIpv4 accepts well-formed dotted quads") {
    uint32_t v = 0;
    REQUIRE(GeoIpDb::parseIpv4("0.0.0.0", v));      REQUIRE(v == 0u);
    REQUIRE(GeoIpDb::parseIpv4("1.2.3.4", v));      REQUIRE(v == 0x01020304u);
    REQUIRE(GeoIpDb::parseIpv4("255.255.255.255", v)); REQUIRE(v == 0xFFFFFFFFu);
    REQUIRE(GeoIpDb::parseIpv4("8.8.8.8", v));      REQUIRE(v == 0x08080808u);
}

TEST_CASE("parseIpv4 rejects malformed input") {
    uint32_t v = 0;
    REQUIRE_FALSE(GeoIpDb::parseIpv4("1.2.3", v));        // too few octets
    REQUIRE_FALSE(GeoIpDb::parseIpv4("1.2.3.4.5", v));    // too many
    REQUIRE_FALSE(GeoIpDb::parseIpv4("1.2.3.256", v));    // octet > 255
    REQUIRE_FALSE(GeoIpDb::parseIpv4("1.2.3.x", v));      // non-numeric
    REQUIRE_FALSE(GeoIpDb::parseIpv4("", v));             // empty
    REQUIRE_FALSE(GeoIpDb::parseIpv4("1..3.4", v));       // empty octet
}

static const char *kCsv =
    "\"1.0.0.0\",\"1.0.0.255\",\"US\"\n"
    "\"1.0.1.0\",\"1.0.3.255\",\"CN\"\n"
    "\"2.0.0.0\",\"2.255.255.255\",\"BR\"\n"
    "\"2001:200::\",\"2001:200:ffff:ffff:ffff:ffff:ffff:ffff\",\"JP\"\n"   // IPv6 → skipped
    "garbage line with no commas\n"
    "\"10.0.0.0\",\"10.255.255.255\",\"DE\"\n";

TEST_CASE("loadCsv parses IPv4 rows and skips IPv6/garbage") {
    GeoIpDb db;
    const int n = db.loadCsv(QString::fromLatin1(kCsv));
    REQUIRE(n == 4);              // 3 clean + DE; IPv6 + garbage dropped
    REQUIRE(db.ready());
    REQUIRE(db.size() == 4);
}

TEST_CASE("country() resolves IPs to the containing range") {
    GeoIpDb db;
    db.loadCsv(QString::fromLatin1(kCsv));
    REQUIRE(db.country("1.0.0.5")   == "US");
    REQUIRE(db.country("1.0.0.255") == "US");   // inclusive end
    REQUIRE(db.country("1.0.2.10")  == "CN");
    REQUIRE(db.country("2.100.5.9") == "BR");
    REQUIRE(db.country("10.0.0.1")  == "DE");
}

TEST_CASE("country() returns empty outside any range or on bad input") {
    GeoIpDb db;
    db.loadCsv(QString::fromLatin1(kCsv));
    REQUIRE(db.country("1.0.4.0").isEmpty());    // gap between CN and BR
    REQUIRE(db.country("9.9.9.9").isEmpty());    // below DE range
    REQUIRE(db.country("200.1.1.1").isEmpty());  // above every range
    REQUIRE(db.country("not-an-ip").isEmpty());
}

TEST_CASE("empty db never matches") {
    GeoIpDb db;
    REQUIRE_FALSE(db.ready());
    REQUIRE(db.country("8.8.8.8").isEmpty());
}

TEST_CASE("lookup() and inCountry() mirror country() without allocating") {
    GeoIpDb db;
    db.loadCsv(QString::fromLatin1(kCsv));

    auto ip = [](const char *s) { uint32_t v = 0; GeoIpDb::parseIpv4(s, v); return v; };

    char cc[2] = {0, 0};
    REQUIRE(db.lookup(ip("1.0.0.5"), cc));
    REQUIRE((cc[0] == 'U' && cc[1] == 'S'));
    REQUIRE(db.lookup(ip("2.100.5.9"), cc));
    REQUIRE((cc[0] == 'B' && cc[1] == 'R'));

    REQUIRE_FALSE(db.lookup(ip("1.0.4.0"), cc));   // gap
    REQUIRE_FALSE(db.lookup(ip("200.1.1.1"), cc)); // above every range

    REQUIRE(db.inCountry(ip("1.0.2.10"), 'C', 'N'));
    REQUIRE_FALSE(db.inCountry(ip("1.0.2.10"), 'U', 'S')); // right IP, wrong CC
    REQUIRE_FALSE(db.inCountry(ip("9.9.9.9"), 'D', 'E'));  // outside every range

    GeoIpDb empty;
    REQUIRE_FALSE(empty.lookup(ip("8.8.8.8"), cc));
    REQUIRE_FALSE(empty.inCountry(ip("8.8.8.8"), 'U', 'S'));
}
