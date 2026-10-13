#include <catch2/catch_test_macros.hpp>
#include "services/games/achievementledger.h"
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

TEST_CASE("an icon on Steam's retired community CDN moves to the live one", "[achievements]") {
    // BOMBANANA! (4656000): the schema's URL 404s, the rewritten one serves it.
    const QString hash = QStringLiteral("4656000/71559d12168d32f8dbd70e4356012bc20b45ecb8.jpg");
    const QString live = QStringLiteral("https://shared.cloudflare.steamstatic.com/community_assets/images/apps/") + hash;
    for (const QString &host : { QStringLiteral("https://steamcdn-a.akamaihd.net"),
                                 QStringLiteral("https://cdn.cloudflare.steamstatic.com"),
                                 QStringLiteral("https://cdn.akamai.steamstatic.com") })
        REQUIRE(Achievements::liveIconUrl(host + QStringLiteral("/steamcommunity/public/images/apps/") + hash) == live);
    // Not a Steam community icon: untouched.
    REQUIRE(Achievements::liveIconUrl(QStringLiteral("https://cdn/a.jpg")) == QLatin1String("https://cdn/a.jpg"));
    REQUIRE(Achievements::liveIconUrl(QString()).isEmpty());
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
    REQUIRE(SteamStore::heroUrl(292030) == QLatin1String("image://steamart/hero/292030"));
    REQUIRE(SteamStore::logoUrl(292030) == QLatin1String("image://steamart/logo/292030"));
    REQUIRE(SteamStore::logoUrl(0).isEmpty());
}

TEST_CASE("store art comes from the hashed path the store API names", "[achievements]") {
    // GetItems for BOMBANANA! (4656000), trimmed: a hero, and no logo at all.
    const QByteArray store = QByteArrayLiteral(
        "{\"response\":{\"store_items\":[{\"appid\":4656000,\"assets\":{"
        "\"asset_url_format\":\"steam/apps/4656000/${FILENAME}?t=1791542068\","
        "\"library_hero\":\"6432b9026df018c6db23dd752352a79faf880ebc/library_hero.jpg\","
        "\"header\":\"99c086faba625a8d3bc459bab444087d257ccbe1/header.jpg\"}}]}}");
    REQUIRE(SteamStore::assetUrl(store, "hero") == QLatin1String(
        "https://shared.cloudflare.steamstatic.com/store_item_assets/steam/apps/4656000/"
        "6432b9026df018c6db23dd752352a79faf880ebc/library_hero.jpg?t=1791542068"));
    REQUIRE(SteamStore::assetUrl(store, "logo").isEmpty());   // no logo: the title in text instead
    REQUIRE(SteamStore::assetUrl("{broken", "hero").isEmpty());
    REQUIRE(SteamStore::assetUrl("{\"response\":{}}", "hero").isEmpty());
    REQUIRE(SteamStore::storeItemsUrl(4656000).contains("IStoreBrowseService/GetItems"));
    REQUIRE(SteamStore::legacyArtUrl(292030, "hero").endsWith("/apps/292030/library_hero.jpg"));
    REQUIRE(SteamStore::legacyArtUrl(292030, "logo").endsWith("/apps/292030/logo.png"));
}

TEST_CASE("progress rides along with the achievement it belongs to", "[achievements]") {
    const auto merged = Achievements::merge(Achievements::parseSchema(kSchema), {}, {},
                                            { { "PAPER_TRAIL", { 3, 10 } }, { "UNKNOWN", { 1, 2 } } });
    for (const auto &a : merged) {
        if (a.apiName == QLatin1String("PAPER_TRAIL")) {
            REQUIRE(a.progress == 3);
            REQUIRE(a.progressMax == 10);
        } else {
            REQUIRE(a.progressMax == 0);
        }
    }
    const QVariantMap v = Achievements::toVariant(merged).value(1).toMap();
    REQUIRE(v.contains("progressMax"));
}

TEST_CASE("rarity and its label are decided once, on the achievement", "[achievements]") {
    const auto merged = Achievements::merge(Achievements::parseSchema(kSchema),
                                            { { "NIGHT_SHIFT", 41.4 }, { "PAPER_TRAIL", 0.42 }, { "REQUIEM", 10.0 } }, {});
    QHash<QString, QVariantMap> by;
    for (const QVariant &v : Achievements::toVariant(merged)) by.insert(v.toMap().value("apiName").toString(), v.toMap());
    REQUIRE(by["NIGHT_SHIFT"].value("percentLabel").toString() == QLatin1String("41"));
    REQUIRE_FALSE(by["NIGHT_SHIFT"].value("rare").toBool());
    REQUIRE(by["PAPER_TRAIL"].value("percentLabel").toString() == QLatin1String("0.4"));
    REQUIRE(by["PAPER_TRAIL"].value("rare").toBool());
    REQUIRE(by["REQUIEM"].value("rare").toBool());   // the line itself counts as rare
    const auto unknown = Achievements::toVariant(Achievements::merge(Achievements::parseSchema(kSchema), {}, {}));
    REQUIRE(unknown.value(0).toMap().value("percentLabel").toString().isEmpty());
    REQUIRE_FALSE(unknown.value(0).toMap().value("rare").toBool());
}

TEST_CASE("locked secrets sink below the locked ones that can be read", "[achievements]") {
    // REQUIEM is hidden and sits before PAPER_TRAIL in the schema.
    auto locked = Achievements::merge(Achievements::parseSchema(kSchema), {}, {});
    REQUIRE(locked[0].apiName == QLatin1String("NIGHT_SHIFT"));
    REQUIRE(locked[1].apiName == QLatin1String("PAPER_TRAIL"));
    REQUIRE(locked[2].apiName == QLatin1String("REQUIEM"));
    // Earned, a secret is just an unlock and leads like any other.
    auto earned = Achievements::merge(Achievements::parseSchema(kSchema), {}, { { "REQUIEM", 50 } });
    REQUIRE(earned[0].apiName == QLatin1String("REQUIEM"));
}

TEST_CASE("an unlock with no time still counts, and a negative time reads as unknown", "[achievements]") {
    const auto merged = Achievements::merge(Achievements::parseSchema(kSchema), {},
                                            { { "REQUIEM", 0 }, { "NIGHT_SHIFT", -7 } });
    REQUIRE(Achievements::unlockedCount(merged) == 2);
    for (const auto &a : merged)
        if (a.unlocked) REQUIRE(a.unlockedAt == 0);
}

// ============================================================================
//  AchievementLedger: unlocks outlive the emulator's files
// ============================================================================
TEST_CASE("ledger: an unlock seen once survives the emulator's file going away", "[achievements][ledger]") {
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString path = dir.filePath("achievements.json");
    {
        AchievementLedger ledger(path);
        const auto now = ledger.merged(4656000, "BOMBANANA!", {{"BOOM", 1790643287}, {"FirstJob", 1790643381}});
        REQUIRE(now.size() == 2);
    }
    // A new run, the game deleted: the files have nothing, the ledger still does.
    AchievementLedger again(path);
    const auto kept = again.merged(4656000, "BOMBANANA!", {});
    REQUIRE(kept.size() == 2);
    REQUIRE(kept.value("BOOM") == 1790643287);
    // A game it never saw stays empty, and no entry is written for it.
    REQUIRE(again.merged(292030, "The Witcher 3", {}).isEmpty());
}

TEST_CASE("ledger: the earliest time wins, and a known time beats an unknown one", "[achievements][ledger]") {
    AchievementLedger::Games mine, theirs;
    mine[1].unlocks = {{"A", 200}, {"B", 0}};
    theirs[1].unlocks = {{"A", 100}, {"B", 300}, {"C", 50}};
    theirs[1].title = "Game";
    REQUIRE(AchievementLedger::fold(mine, theirs) == 1);   // only C is new
    REQUIRE(mine[1].unlocks.value("A") == 100);
    REQUIRE(mine[1].unlocks.value("B") == 300);             // 0 was "unknown"
    REQUIRE(mine[1].unlocks.value("C") == 50);
    REQUIRE(mine[1].title == "Game");
}

TEST_CASE("ledger: export and import round-trip, and import never removes", "[achievements][ledger]") {
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    AchievementLedger before(dir.filePath("old-pc.json"));
    before.merged(4656000, "BOMBANANA!", {{"BOOM", 10}, {"Basket", 20}});
    before.merged(1159690, "Other", {{"X", 30}});
    const QString backup = dir.filePath("batorrent-achievements.json");
    REQUIRE(before.exportTo(backup));

    // A fresh install that already unlocked something new before the restore.
    AchievementLedger after(dir.filePath("new-pc.json"));
    after.merged(4656000, "BOMBANANA!", {{"Endless", 40}});
    REQUIRE(after.importFrom(backup) == 3);
    const auto bomb = after.merged(4656000, "BOMBANANA!", {});
    REQUIRE(bomb.size() == 3);                 // Endless stayed, BOOM and Basket came in
    REQUIRE(after.merged(1159690, "Other", {}).value("X") == 30);
    REQUIRE(after.importFrom(backup) == 0);    // a second import adds nothing
}

TEST_CASE("ledger: a file that is not a backup is refused, not merged", "[achievements][ledger]") {
    AchievementLedger::Games out;
    REQUIRE_FALSE(AchievementLedger::fromJson("{broken", &out));
    REQUIRE_FALSE(AchievementLedger::fromJson("{\"settings\":{}}", &out));
    REQUIRE(AchievementLedger::fromJson(AchievementLedger::toJson({}), &out));
    QTemporaryDir dir;
    QFile junk(dir.filePath("junk.json"));
    REQUIRE(junk.open(QIODevice::WriteOnly));
    junk.write("{\"settings\":{\"a\":1}}");
    junk.close();
    AchievementLedger ledger(dir.filePath("achievements.json"));
    REQUIRE(ledger.importFrom(junk.fileName()) == -1);
}

TEST_CASE("a game IGDB lacks gets its page from the Steam store answer", "[achievements]") {
    // GetItems for SEXNATURAL (3920160), trimmed: name, blurb, release, capsules.
    const QByteArray store = QByteArrayLiteral(
        "{\"response\":{\"store_items\":[{\"appid\":3920160,\"name\":\"SEXNATURAL: MILFs Must Be Filled\","
        "\"basic_info\":{\"short_description\":\"Exorcism has never been this hot.\"},"
        "\"release\":{\"steam_release_date\":1788616851},\"assets\":{"
        "\"asset_url_format\":\"steam/apps/3920160/${FILENAME}?t=1788783754\","
        "\"library_capsule\":\"fc68/library_capsule.jpg\","
        "\"library_capsule_2x\":\"fc68/library_capsule_2x.jpg\"}}]}}");
    const SteamStore::StoreDetails d = SteamStore::details(store);
    REQUIRE(d.name == QLatin1String("SEXNATURAL: MILFs Must Be Filled"));
    REQUIRE(d.description == QLatin1String("Exorcism has never been this hot."));
    REQUIRE(d.year == 2026);
    REQUIRE(d.posterUrl.endsWith(QLatin1String("/apps/3920160/fc68/library_capsule_2x.jpg?t=1788783754")));
    REQUIRE(SteamStore::details("{broken").name.isEmpty());
    REQUIRE(SteamStore::details(QString::fromUtf8(
                "{\"response\":{\"store_items\":[{\"name\":\"SEXNATURAL: MILFs Must Be Filled \xF0\x9F\x98\x88\xF0\x9F\x92\xA6\"}]}}")
                .toUtf8()).name == QLatin1String("SEXNATURAL: MILFs Must Be Filled"));
    // The folder name lost the subtitle; an exact name still wins over it.
    REQUIRE(SteamStore::bestAppId(R"({"items":[{"type":"app","name":"SEXNATURAL: MILFs Must Be Filled","id":3920160}]})",
                                  "Sexnatural") == 3920160);
    REQUIRE(SteamStore::bestAppId(R"({"items":[{"type":"app","name":"Portal: Prelude","id":1},
                                               {"type":"app","name":"Portal","id":400}]})", "Portal") == 400);
    REQUIRE(SteamStore::storeItemsUrl(1).contains(QLatin1String("include_basic_info")));
}
