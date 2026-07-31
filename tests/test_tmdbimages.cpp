#include <catch2/catch_test_macros.hpp>
#include "services/discovery/tmdbimages.h"

#include <QByteArray>

static QByteArray sampleImagesJson()
{
    return QByteArrayLiteral(R"({
      "backdrops": [
        { "file_path": "/tagged.jpg", "iso_639_1": "en" },
        { "file_path": "/clean1.jpg", "iso_639_1": null },
        { "file_path": "/clean2.jpg", "iso_639_1": null },
        { "file_path": "/tagged2.jpg", "iso_639_1": "pt" },
        { "file_path": "/clean1.jpg", "iso_639_1": null }
      ]
    })");
}

TEST_CASE("backdropUrls prefers untagged then tagged, dedupes, respects limit", "[tmdbimages]") {
    const auto urls = TmdbImages::backdropUrls(
        sampleImagesJson(), QStringLiteral("https://img"), 3);
    REQUIRE(urls.size() == 3);
    REQUIRE(urls.at(0) == QLatin1String("https://img/clean1.jpg"));
    REQUIRE(urls.at(1) == QLatin1String("https://img/clean2.jpg"));
    REQUIRE(urls.at(2) == QLatin1String("https://img/tagged.jpg"));
}

TEST_CASE("backdropUrls empty on bad input", "[tmdbimages]") {
    REQUIRE(TmdbImages::backdropUrls({}, QStringLiteral("https://img")).isEmpty());
    REQUIRE(TmdbImages::backdropUrls(sampleImagesJson(), QStringLiteral("https://img"), 0).isEmpty());
}

TEST_CASE("youtubeTrailerKey prefers Trailer over Teaser", "[tmdbimages]") {
    const QByteArray json = QByteArrayLiteral(R"({
      "results": [
        { "site": "Vimeo", "type": "Trailer", "key": "vimeo1" },
        { "site": "YouTube", "type": "Teaser", "key": "teaser1" },
        { "site": "YouTube", "type": "Trailer", "key": "trailer1" }
      ]
    })");
    REQUIRE(TmdbImages::youtubeTrailerKey(json) == QStringLiteral("trailer1"));
}

TEST_CASE("youtubeTrailerKey falls back to Teaser", "[tmdbimages]") {
    const QByteArray json = QByteArrayLiteral(R"({
      "results": [
        { "site": "YouTube", "type": "Featurette", "key": "feat" },
        { "site": "YouTube", "type": "Teaser", "key": "teaser1" }
      ]
    })");
    REQUIRE(TmdbImages::youtubeTrailerKey(json) == QStringLiteral("teaser1"));
    REQUIRE(TmdbImages::youtubeTrailerKey({}).isEmpty());
}
