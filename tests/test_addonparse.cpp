#include <catch2/catch_test_macros.hpp>
#include "services/discovery/addonparse.h"
#include "services/discovery/addoncatalog.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QSet>

TEST_CASE("isValidInfoHash accepts v1 hex, v2 hex, base32", "[addonparse]") {
    REQUIRE(AddonParse::isValidInfoHash(
        QStringLiteral("0123456789abcdef0123456789abcdef01234567")));
    REQUIRE(AddonParse::isValidInfoHash(
        QStringLiteral("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef")));
    REQUIRE(AddonParse::isValidInfoHash(
        QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZ234567")));
    REQUIRE_FALSE(AddonParse::isValidInfoHash(QStringLiteral("")));
    REQUIRE_FALSE(AddonParse::isValidInfoHash(QStringLiteral("0")));
    REQUIRE_FALSE(AddonParse::isValidInfoHash(
        QStringLiteral("0123456789abcdef0123456789abcdef01234567&ws=evil")));
    REQUIRE_FALSE(AddonParse::isValidInfoHash(QStringLiteral("not-a-hash")));
    // apibay's "nothing found" row carries a hash of zeros: no torrent has it
    REQUIRE_FALSE(AddonParse::isValidInfoHash(
        QStringLiteral("0000000000000000000000000000000000000000")));
}

TEST_CASE("normalizeAddonBaseUrl strips slash and manifest.json", "[addonparse]") {
    REQUIRE(AddonParse::normalizeAddonBaseUrl(
                QStringLiteral(" https://torrentio.strem.fun/manifest.json/ "))
            == QLatin1String("https://torrentio.strem.fun"));
    REQUIRE(AddonParse::normalizeAddonBaseUrl(
                QStringLiteral("https://v3-cinemeta.strem.io/"))
            == QLatin1String("https://v3-cinemeta.strem.io"));
    REQUIRE(AddonParse::normalizeAddonBaseUrl(QStringLiteral("   ")).isEmpty());
}

TEST_CASE("streamBaseUrl injects language only for bare torrentio", "[addonparse]") {
    REQUIRE(AddonParse::streamBaseUrl(QStringLiteral("https://torrentio.strem.fun"),
                                      QStringLiteral("portuguese"))
            == QLatin1String("https://torrentio.strem.fun/language=portuguese"));
    REQUIRE(AddonParse::streamBaseUrl(QStringLiteral("https://torrentio.strem.fun"), QString())
            == QLatin1String("https://torrentio.strem.fun"));
    REQUIRE(AddonParse::streamBaseUrl(
                QStringLiteral("https://torrentio.strem.fun/providers=nyaasi|language=japanese"),
                QStringLiteral("portuguese"))
            == QLatin1String("https://torrentio.strem.fun/providers=nyaasi|language=japanese"));
    REQUIRE(AddonParse::streamBaseUrl(QStringLiteral("https://my.custom.addon/sub"),
                                      QStringLiteral("portuguese"))
            == QLatin1String("https://my.custom.addon/sub"));
}

TEST_CASE("torrentioLanguageTag maps content languages", "[addonparse]") {
    REQUIRE(AddonParse::torrentioLanguageTag(Translator::Portuguese)
            == QLatin1String("portuguese"));
    REQUIRE(AddonParse::torrentioLanguageTag(Translator::Spanish)
            == QLatin1String("spanish"));
    REQUIRE(AddonParse::torrentioLanguageTag(Translator::English).isEmpty());
}

TEST_CASE("parseSizeValue accepts bytes and human strings", "[addonparse]") {
    REQUIRE(AddonParse::parseSizeValue(QJsonValue(1048576)) == 1048576);
    REQUIRE(AddonParse::parseSizeValue(QJsonValue(QStringLiteral("1048576"))) == 1048576);
    REQUIRE(AddonParse::parseSizeValue(QJsonValue(QStringLiteral("28.47 GB")))
            == static_cast<qint64>(28.47 * 1024.0 * 1024 * 1024));
    // TorAPI uses NBSP between number and unit
    REQUIRE(AddonParse::parseSizeValue(
                QJsonValue(QStringLiteral("1.5") + QChar(0x00A0) + QStringLiteral("MiB")))
            == static_cast<qint64>(1.5 * 1024.0 * 1024));
    REQUIRE(AddonParse::parseSizeValue(QJsonValue(QStringLiteral(""))) == 0);
    REQUIRE(AddonParse::parseSizeValue(QJsonValue(QStringLiteral("n/a"))) == 0);
}

TEST_CASE("parseProviderResponse Jackett-like Results array", "[addonparse]") {
    SearchProvider p;
    p.name = QStringLiteral("Jackett");
    p.arrayPath = QStringLiteral("Results");
    p.namePath = QStringLiteral("Title");
    p.hashPath = QStringLiteral("InfoHash");
    p.sizePath = QStringLiteral("Size");
    p.seedersPath = QStringLiteral("Seeders");
    p.leechersPath = QStringLiteral("Peers");

    const QByteArray json = QByteArrayLiteral(R"({
      "Results": [
        {
          "Title": "Movie.2020.1080p",
          "InfoHash": "0123456789abcdef0123456789abcdef01234567",
          "Size": 1500000000,
          "Seeders": 42,
          "Peers": 3
        },
        {
          "Title": "skip-empty-hash",
          "InfoHash": "0",
          "Size": 1,
          "Seeders": 1,
          "Peers": 0
        }
      ]
    })");

    const auto rows = AddonParse::parseProviderResponse(p, json);
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.at(0).name == QLatin1String("Movie.2020.1080p"));
    REQUIRE(rows.at(0).infoHash
            == QLatin1String("0123456789abcdef0123456789abcdef01234567"));
    REQUIRE(rows.at(0).size == 1500000000);
    REQUIRE(rows.at(0).seeders == 42);
    REQUIRE(rows.at(0).leechers == 3);
    REQUIRE(rows.at(0).provider == QLatin1String("Jackett"));
    REQUIRE(rows.at(0).magnet.startsWith(
        QLatin1String("magnet:?xt=urn:btih:0123456789abcdef0123456789abcdef01234567")));
    REQUIRE(rows.at(0).magnet.contains(QLatin1String("&tr=")));
}

TEST_CASE("parseProviderResponse TorAPI root array with human size", "[addonparse]") {
    SearchProvider p;
    p.name = QStringLiteral("RuTor");
    p.arrayPath = QString();
    p.namePath = QStringLiteral("Name");
    p.hashPath = QStringLiteral("Hash");
    p.sizePath = QStringLiteral("Size");
    p.seedersPath = QStringLiteral("Seeds");
    p.leechersPath = QStringLiteral("Peers");

    const QByteArray json = QByteArrayLiteral(R"([
      {
        "Name": "Film &amp; Co",
        "Hash": "ABCDEF0123456789ABCDEF0123456789ABCDEF01",
        "Size": "2.0 GB",
        "Seeds": 10,
        "Peers": 1
      }
    ])");

    const auto rows = AddonParse::parseProviderResponse(p, json);
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.at(0).name == QLatin1String("Film & Co"));
    REQUIRE(rows.at(0).size == static_cast<qint64>(2.0 * 1024 * 1024 * 1024));
    REQUIRE(rows.at(0).seeders == 10);
}

TEST_CASE("parseApibayArray rejects invalid hashes", "[addonparse]") {
    const QByteArray json = QByteArrayLiteral(R"([
      { "name": "ok", "info_hash": "0123456789abcdef0123456789abcdef01234567",
        "size": 100, "seeders": 1, "leechers": 0, "category": "201" },
      { "name": "bad", "info_hash": "short", "size": 1, "seeders": 0, "leechers": 0 }
    ])");
    const auto rows = AddonParse::parseApibayArray(json);
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.at(0).name == QLatin1String("ok"));
    REQUIRE(AddonParse::parseApibayArray(QByteArrayLiteral("{}")).isEmpty());
}

TEST_CASE("The Pirate Bay's empty answer is not a release", "[addonparse]") {
    // apibay answers a search with no hits with one made-up row. Searching a
    // film's translated title often finds nothing there, and that row showed
    // up in Find as a release called "No results returned".
    const QByteArray json = QByteArrayLiteral(R"([
      { "id": "0", "name": "No results returned",
        "info_hash": "0000000000000000000000000000000000000000",
        "leechers": "0", "seeders": "0", "num_files": "0", "size": "0",
        "username": "", "added": "0", "status": "member", "category": "0", "imdb": "" }
    ])");
    REQUIRE(AddonParse::parseApibayArray(json).isEmpty());

    SearchProvider p;     // apibay as the default provider list sets it up
    p.name = QStringLiteral("The Pirate Bay (apibay)");
    p.namePath = QStringLiteral("name");
    p.hashPath = QStringLiteral("info_hash");
    p.sizePath = QStringLiteral("size");
    p.seedersPath = QStringLiteral("seeders");
    p.leechersPath = QStringLiteral("leechers");
    REQUIRE(AddonParse::parseProviderResponse(p, json).isEmpty());
}

TEST_CASE("parseManifestJson maps types and resource objects", "[addonparse]") {
    const QByteArray json = QByteArrayLiteral(R"({
      "id": "com.example.addon",
      "name": "Example",
      "description": "desc",
      "types": ["movie", "series"],
      "resources": ["stream", { "name": "catalog", "types": ["movie"] }]
    })");
    AddonManifest m;
    REQUIRE(AddonParse::parseManifestJson(json, QStringLiteral("https://ex.example"), &m));
    REQUIRE(m.id == QLatin1String("com.example.addon"));
    REQUIRE(m.name == QLatin1String("Example"));
    REQUIRE(m.url == QLatin1String("https://ex.example"));
    REQUIRE(m.types == QStringList({QStringLiteral("movie"), QStringLiteral("series")}));
    REQUIRE(m.resources.contains(QStringLiteral("stream")));
    REQUIRE(m.resources.contains(QStringLiteral("catalog")));
    REQUIRE(m.enabled);
    REQUIRE_FALSE(AddonParse::parseManifestJson(QByteArrayLiteral("[]"),
                                                 QStringLiteral("https://x"), &m));
}

TEST_CASE("parseCatalogMetas and parseStreamResults", "[addonparse]") {
    const QByteArray catalog = QByteArrayLiteral(R"({
      "metas": [
        { "id": "tt1", "type": "movie", "name": "One", "poster": "p", "releaseInfo": "2020" },
        { "id": "", "type": "movie", "name": "skip" }
      ]
    })");
    const auto items = AddonParse::parseCatalogMetas(catalog);
    REQUIRE(items.size() == 1);
    REQUIRE(items.at(0).id == QLatin1String("tt1"));
    REQUIRE(items.at(0).year == 2020);

    const QByteArray streams = QByteArrayLiteral(R"({
      "streams": [
        {
          "infoHash": "0123456789abcdef0123456789abcdef01234567",
          "title": "1080p",
          "sources": ["tracker:udp://t.example/announce"],
          "behaviorHints": { "videoSize": 999 }
        },
        { "url": "https://http.example/file.mkv", "name": "http" },
        { "url": "magnet:?xt=urn:btih:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "name": "m" }
      ]
    })");
    const auto rows = AddonParse::parseStreamResults(streams, QStringLiteral("Torrentio"));
    REQUIRE(rows.size() == 2);
    REQUIRE(rows.at(0).addonName == QLatin1String("Torrentio"));
    REQUIRE(rows.at(0).size == 999);
    REQUIRE(rows.at(0).magnet.contains(QLatin1String("&tr=")));
    REQUIRE(rows.at(1).magnet.startsWith(QLatin1String("magnet:")));
    REQUIRE(rows.at(0).fileIdx == -1);
    REQUIRE(rows.at(0).seeders == -1);
    REQUIRE_FALSE(rows.at(0).inPack);
}

TEST_CASE("parseStreamResults keeps Torrentio's file, seeders and size", "[addonparse]") {
    // Torrentio's answer for tt0434665:1:3 (Bleach S1E3): the Erai-raws batch,
    // pointing at episode 003 inside it, and a single-episode release.
    const QByteArray streams = QByteArrayLiteral(R"({
      "streams": [
        {
          "name": "Torrentio\n1080p",
          "title": "[Erai-raws] Bleach - 001 ~ 366 [1080p][Multiple Subtitle]\n[Erai-raws] Bleach - 003 [1080p][Multiple Subtitle].mkv\n👤 561 💾 1.29 GB ⚙️ Nyaa\nMulti Subs",
          "infoHash": "0123456789abcdef0123456789abcdef01234567",
          "fileIdx": 2,
          "behaviorHints": { "bingeGroup": "torrentio|1080p", "filename": "[Erai-raws] Bleach - 003 [1080p][Multiple Subtitle].mkv" }
        },
        {
          "name": "Torrentio\n720p",
          "title": "Bleach.S01E03.720p.WEB.x264\n👤 1 💾 350 MB ⚙️ ThePirateBay",
          "infoHash": "89abcdef0123456789abcdef0123456789abcdef",
          "fileIdx": 0,
          "behaviorHints": { "videoSize": 367001600, "filename": "Bleach.S01E03.720p.WEB.x264.mkv" }
        }
      ]
    })");
    const auto rows = AddonParse::parseStreamResults(streams, QStringLiteral("Torrentio"));
    REQUIRE(rows.size() == 2);

    const StreamResult &pack = rows.at(0);
    CHECK(pack.torrentName == QLatin1String("[Erai-raws] Bleach - 001 ~ 366 [1080p][Multiple Subtitle]"));
    CHECK(pack.fileName == QLatin1String("[Erai-raws] Bleach - 003 [1080p][Multiple Subtitle].mkv"));
    CHECK(pack.fileIdx == 2);
    CHECK(pack.seeders == 561);
    CHECK(pack.source == QLatin1String("Nyaa"));
    CHECK(pack.size == qint64(1.29 * 1024 * 1024 * 1024));   // the episode's file, not the 321 GB batch
    CHECK(pack.inPack);

    const StreamResult &single = rows.at(1);
    CHECK(single.torrentName == QLatin1String("Bleach.S01E03.720p.WEB.x264"));
    CHECK(single.seeders == 1);
    CHECK(single.size == 367001600);   // behaviorHints.videoSize wins over the title's rounded size
    CHECK(single.fileIdx == 0);
    CHECK_FALSE(single.inPack);
}

TEST_CASE("readStreamTitle: a title without stats keeps its first line as the name", "[addonparse]") {
    StreamResult r;
    r.title = QStringLiteral("Some.Show.S02E01.1080p");
    AddonParse::readStreamTitle(r);
    CHECK(r.torrentName == QLatin1String("Some.Show.S02E01.1080p"));
    CHECK(r.seeders == -1);
    CHECK(r.fileName.isEmpty());
    CHECK_FALSE(r.inPack);
}

TEST_CASE("curatedCatalog seedDefault non-empty and ids unique when set", "[addoncatalog]") {
    const auto cat = AddonCatalog::curatedCatalog();
    REQUIRE_FALSE(cat.isEmpty());
    int seeded = 0;
    QSet<QString> ids;
    for (const auto &a : cat) {
        if (a.seedDefault) ++seeded;
        if (!a.id.isEmpty()) {
            REQUIRE_FALSE(ids.contains(a.id));
            ids.insert(a.id);
        }
    }
    REQUIRE(seeded > 0);
    REQUIRE(ids.contains(QStringLiteral("com.linvo.cinemeta")));
    REQUIRE(ids.contains(QStringLiteral("com.stremio.torrentio.addon")));
    // The other public torrent addons, seeded on for every language.
    for (const auto &a : cat) {
        if (a.id == QLatin1String("com.torrentsdb.addon") || a.id == QLatin1String("com.keopps.peerflix")) {
            REQUIRE(a.seedDefault);
            REQUIRE(a.lang.isEmpty());
            REQUIRE(a.resources == QStringList{ QStringLiteral("stream") });
        }
    }
    REQUIRE(ids.contains(QStringLiteral("com.torrentsdb.addon")));
    REQUIRE(ids.contains(QStringLiteral("com.keopps.peerflix")));
}

TEST_CASE("providerCatalog ids unique and non-empty", "[addoncatalog]") {
    const auto cat = AddonCatalog::providerCatalog();
    REQUIRE_FALSE(cat.isEmpty());
    QSet<QString> ids;
    for (const auto &p : cat) {
        REQUIRE_FALSE(p.provider.id.isEmpty());
        REQUIRE_FALSE(ids.contains(p.provider.id));
        ids.insert(p.provider.id);
        REQUIRE(p.provider.urlTemplate.contains(QStringLiteral("{query}")));
    }
    REQUIRE(ids.contains(QStringLiteral("jackett_local")));
    REQUIRE(ids.contains(QStringLiteral("rutor_torapi")));
}

TEST_CASE("defaultProviders seed table non-empty unique ids", "[addoncatalog]") {
    const auto defs = AddonCatalog::defaultProviders();
    REQUIRE(defs.size() >= 4);
    QSet<QString> ids;
    for (const auto &d : defs) {
        REQUIRE_FALSE(d.id.isEmpty());
        REQUIRE(d.url.contains(QStringLiteral("{query}")));
        REQUIRE_FALSE(ids.contains(d.id));
        ids.insert(d.id);
    }
    REQUIRE(ids.contains(QStringLiteral("apibay")));
}
