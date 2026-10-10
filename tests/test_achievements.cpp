#include <catch2/catch_test_macros.hpp>
#include "services/games/achievements.h"
#include "services/games/achievementservice.h"
#include "services/games/steamstore.h"
#include "services/platform/contentlanguage.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include <QVariantMap>

namespace {

const QByteArray kSchema = QByteArrayLiteral(R"({"game":{"gameName":"X","availableGameStats":{"achievements":[
  {"name":"NIGHT_SHIFT","defaultvalue":0,"displayName":"Night Shift","hidden":0,
   "description":"Survive your first night","icon":"https://cdn/a.jpg","icongray":"https://cdn/a_g.jpg"},
  {"name":"REQUIEM","displayName":"Requiem","hidden":1,"icon":"https://cdn/r.jpg","icongray":"https://cdn/r_g.jpg"},
  {"name":"","displayName":"no id"},
  {"name":"PAPER_TRAIL","displayName":"","description":"Every case file"}
]}}})");

}

TEST_CASE("the schema keeps every named achievement and its art", "[achievements]") {
    const auto list = Achievements::parseSchema(kSchema);
    REQUIRE(list.size() == 3);
    REQUIRE(list[0].apiName == QLatin1String("NIGHT_SHIFT"));
    REQUIRE(list[0].name == QLatin1String("Night Shift"));
    REQUIRE(list[0].icon == QLatin1String("https://cdn/a.jpg"));
    REQUIRE(list[0].iconGray == QLatin1String("https://cdn/a_g.jpg"));
    REQUIRE_FALSE(list[0].hidden);
    REQUIRE(list[1].hidden);
    REQUIRE(list[1].description.isEmpty());
    REQUIRE(list[2].name == QLatin1String("PAPER_TRAIL"));   // no display name: the id stands in
}

TEST_CASE("a schema that is not one is empty, not a crash", "[achievements]") {
    REQUIRE(Achievements::parseSchema(QByteArray()).isEmpty());
    REQUIRE(Achievements::parseSchema("{not json").isEmpty());
    REQUIRE(Achievements::parseSchema(R"({"game":{}})").isEmpty());
    REQUIRE(Achievements::parseSchema(R"({"game":{"availableGameStats":{"achievements":"x"}}})").isEmpty());
}

TEST_CASE("global percentages read as numbers or strings, and nonsense is dropped", "[achievements]") {
    const auto pct = Achievements::parsePercentages(R"({"achievementpercentages":{"achievements":[
      {"name":"A","percent":41.5},{"name":"B","percent":"6.2"},{"name":"C","percent":"lots"},
      {"name":"D","percent":180},{"name":"","percent":3}]}})");
    REQUIRE(pct.size() == 2);
    REQUIRE(pct.value("A") == 41.5);
    REQUIRE(pct.value("B") == 6.2);
    REQUIRE(Achievements::parsePercentages("[]").isEmpty());
}

TEST_CASE("merging puts the newest unlocks first and keeps the rest in schema order", "[achievements]") {
    auto schema = Achievements::parseSchema(kSchema);
    const QHash<QString, double> pct{ { "NIGHT_SHIFT", 41.0 }, { "PAPER_TRAIL", 12.0 } };
    const QHash<QString, qint64> unlocked{ { "PAPER_TRAIL", 2000 }, { "NIGHT_SHIFT", 1000 }, { "NOT_IN_SCHEMA", 5 } };
    const auto merged = Achievements::merge(schema, pct, unlocked);
    REQUIRE(merged.size() == 3);
    REQUIRE(merged[0].apiName == QLatin1String("PAPER_TRAIL"));
    REQUIRE(merged[0].unlockedAt == 2000);
    REQUIRE(merged[1].apiName == QLatin1String("NIGHT_SHIFT"));
    REQUIRE(merged[1].percent == 41.0);
    REQUIRE_FALSE(merged[2].unlocked);
    REQUIRE(merged[2].percent == -1);
    REQUIRE(Achievements::unlockedCount(merged) == 2);

    const QVariantMap v = Achievements::toVariant(merged).value(2).toMap();
    REQUIRE(v.value("apiName").toString() == QLatin1String("REQUIEM"));
    REQUIRE(v.value("hidden").toBool());
    REQUIRE_FALSE(v.value("unlocked").toBool());
}

TEST_CASE("a fresh cached schema answers without the network, crossed with the emulator's file", "[achievements]") {
    static int argc = 1;
    static char arg0[] = "test_achievements";
    static char *argv[] = { arg0, nullptr };
    if (!QCoreApplication::instance()) new QCoreApplication(argc, argv);

    QTemporaryDir cache, game;
    const int appId = 4242;
    {
        QDir().mkpath(cache.path());
        QFile f(QDir(cache.path()).filePath(QStringLiteral("%1_%2.json").arg(appId).arg(ContentLanguage::steam())));
        REQUIRE(f.open(QIODevice::WriteOnly));
        f.write(QJsonDocument(QJsonObject{
            { "at", double(QDateTime::currentSecsSinceEpoch()) },
            { "schema", QString::fromUtf8(kSchema) },
            { "percent", R"({"achievementpercentages":{"achievements":[{"name":"REQUIEM","percent":"4.0"}]}})" },
        }).toJson());
    }
    {
        QDir().mkpath(game.filePath("steam_settings/4242"));
        QFile f(game.filePath("steam_settings/4242/achievements.json"));
        REQUIRE(f.open(QIODevice::WriteOnly));
        f.write(R"({"REQUIEM":{"earned":true,"earned_time":1700000000}})");
    }

    AchievementService svc(cache.path());
    QString gotKey;
    QVariantList got;
    int unlocked = -1, total = -1;
    QObject::connect(&svc, &AchievementService::ready, [&](const QString &key, int, const QVariantList &list, int u, int t) {
        gotKey = key; got = list; unlocked = u; total = t;
    });
    svc.request(QStringLiteral("hash1"), appId, game.path());
    REQUIRE(gotKey == QLatin1String("hash1"));
    REQUIRE(total == 3);
    REQUIRE(unlocked == 1);
    const QVariantMap first = got.value(0).toMap();
    REQUIRE(first.value("apiName").toString() == QLatin1String("REQUIEM"));
    REQUIRE(first.value("unlockedAt").toLongLong() == 1700000000);
    REQUIRE(first.value("percent").toDouble() == 4.0);

    gotKey.clear();
    svc.request(QStringLiteral("nothing"), 0, game.path());
    // No appid and no title to search for: an empty answer, so the caller stops waiting.
    REQUIRE(gotKey == QLatin1String("nothing"));
    REQUIRE(got.isEmpty());
    REQUIRE(total == 0);
}

TEST_CASE("the store search term drops the edition tail Steam can't find", "[achievements][steamstore]") {
    REQUIRE(SteamStore::searchTerm("The Witcher 3: Wild Hunt - Complete Edition") == QLatin1String("the witcher 3 wild hunt"));
    REQUIRE(SteamStore::searchTerm("Dishonored Definitive Edition") == QLatin1String("dishonored"));
    REQUIRE(SteamStore::searchTerm("Batman: Arkham City - Game of the Year Edition") == QLatin1String("batman arkham city"));
    REQUIRE(SteamStore::searchTerm("Gold") == QLatin1String("gold"));   // a one-word title keeps its word
    REQUIRE(SteamStore::searchTerm("Kingdom Come: Deliverance II") == QLatin1String("kingdom come deliverance ii"));
}

TEST_CASE("the store match is the game, never the first hit", "[achievements][steamstore]") {
    // What the store returned for "the witcher 3 wild hunt": a DLC first, and
    // the game renamed "— Remastered".
    const QByteArray witcher = R"({"total":5,"items":[
      {"type":"app","name":"The Witcher 3: Wild Hunt — Songs of the Past","id":5006530},
      {"type":"app","name":"The Witcher 3: Wild Hunt — Remastered","id":292030},
      {"type":"app","name":"The Witcher 3: Wild Hunt — Remastered Soundtrack","id":1229320}]})";
    REQUIRE(SteamStore::bestAppId(witcher, "The Witcher 3: Wild Hunt - Complete Edition") == 292030);

    const QByteArray re = R"({"items":[
      {"type":"app","name":"Resident Evil Requiem - Deluxe Kit","id":3990820},
      {"type":"app","name":"Resident Evil Requiem","id":3764200}]})";
    REQUIRE(SteamStore::bestAppId(re, "Resident Evil Requiem") == 3764200);

    REQUIRE(SteamStore::bestAppId(R"({"items":[{"type":"app","name":"ELDEN RING NIGHTREIGN","id":2}]})", "Elden Ring") == 0);
    REQUIRE(SteamStore::bestAppId(R"({"items":[{"type":"sub","name":"Elden Ring","id":9}]})", "Elden Ring") == 0);
    REQUIRE(SteamStore::bestAppId("{broken", "Elden Ring") == 0);
    REQUIRE(SteamStore::bestAppId(re, "") == 0);
    REQUIRE(SteamStore::heroUrl(292030).endsWith("/apps/292030/library_hero.jpg"));
    REQUIRE(SteamStore::logoUrl(0).isEmpty());
}

TEST_CASE("an unlock with no time still counts, and a negative time reads as unknown", "[achievements]") {
    const auto merged = Achievements::merge(Achievements::parseSchema(kSchema), {},
                                            { { "REQUIEM", 0 }, { "NIGHT_SHIFT", -7 } });
    REQUIRE(Achievements::unlockedCount(merged) == 2);
    for (const auto &a : merged)
        if (a.unlocked) REQUIRE(a.unlockedAt == 0);
}
