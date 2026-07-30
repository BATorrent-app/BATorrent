// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include <catch2/catch_test_macros.hpp>

#include "services/platform/contentlanguage.h"

#include <QCoreApplication>
#include <QSettings>

namespace {
// QSettings needs an org/app to write to; give the tests their own scope so a
// run never touches the real BATorrent preferences.
struct SettingsScope {
    SettingsScope()
    {
        QCoreApplication::setOrganizationName(QStringLiteral("BATorrentTests"));
        QCoreApplication::setApplicationName(QStringLiteral("contentlanguage"));
        ContentLanguage::followApp();
    }
    ~SettingsScope() { ContentLanguage::followApp(); }
};
}

TEST_CASE("with no choice stored, content follows the app language", "[contentlanguage]")
{
    SettingsScope scope;
    REQUIRE(ContentLanguage::followsApp());

    Translator::instance().setLanguage(Translator::Portuguese);
    REQUIRE(ContentLanguage::current() == Translator::Portuguese);
    REQUIRE(ContentLanguage::releaseTag() == "PT");

    Translator::instance().setLanguage(Translator::English);
    REQUIRE(ContentLanguage::current() == Translator::English);
    REQUIRE(ContentLanguage::releaseTag() == "EN");
}

// The whole point of the module: English chrome, Portuguese content.
TEST_CASE("an explicit choice outranks the app language", "[contentlanguage]")
{
    SettingsScope scope;
    Translator::instance().setLanguage(Translator::English);
    ContentLanguage::set(Translator::Portuguese);

    REQUIRE_FALSE(ContentLanguage::followsApp());
    REQUIRE(ContentLanguage::current() == Translator::Portuguese);
    REQUIRE(ContentLanguage::releaseTag() == "PT");
    REQUIRE(ContentLanguage::tmdb() == "pt-BR");
    REQUIRE(ContentLanguage::subtitleCode() == "pt");
    REQUIRE(ContentLanguage::region() == "BR");

    ContentLanguage::followApp();
    REQUIRE(ContentLanguage::current() == Translator::English);
}

// Turkish is the case that used to read one past the end of an eight-entry table
// and silently degrade to English on the dub/sub axis.
TEST_CASE("every supported language has a code, Turkish included", "[contentlanguage]")
{
    SettingsScope scope;
    const Translator::Language all[] = {
        Translator::English, Translator::Portuguese, Translator::Chinese,
        Translator::Japanese, Translator::Russian, Translator::Spanish,
        Translator::German, Translator::Ukrainian, Translator::Turkish
    };
    for (Translator::Language l : all) {
        REQUIRE_FALSE(ContentLanguage::releaseTag(l).isEmpty());
        REQUIRE_FALSE(ContentLanguage::tmdb(l).isEmpty());
        REQUIRE_FALSE(ContentLanguage::subtitleCode(l).isEmpty());
        REQUIRE(ContentLanguage::tmdb(l).contains('-'));
        REQUIRE(ContentLanguage::subtitleCode(l).size() == 2);
    }

    REQUIRE(ContentLanguage::releaseTag(Translator::Turkish) == "TR");
    REQUIRE(ContentLanguage::tmdb(Translator::Turkish) == "tr-TR");
    REQUIRE(ContentLanguage::subtitleCode(Translator::Turkish) == "tr");

    SECTION("only English maps to the English codes") {
        for (Translator::Language l : all) {
            if (l == Translator::English) continue;
            REQUIRE(ContentLanguage::releaseTag(l) != "EN");
            REQUIRE(ContentLanguage::tmdb(l) != "en-US");
        }
    }
}

TEST_CASE("a corrupt stored value falls back instead of crashing", "[contentlanguage]")
{
    SettingsScope scope;
    Translator::instance().setLanguage(Translator::German);

    QSettings().setValue(QStringLiteral("contentLanguage"), 999);
    REQUIRE(ContentLanguage::current() == Translator::German);

    QSettings().setValue(QStringLiteral("contentLanguage"), -3);
    REQUIRE(ContentLanguage::current() == Translator::German);

    QSettings().setValue(QStringLiteral("contentLanguage"), QStringLiteral("nonsense"));
    REQUIRE(ContentLanguage::current() == Translator::German);
}
