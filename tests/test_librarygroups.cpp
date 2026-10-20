// SPDX-License-Identifier: MIT
// Library series grouping (Catch2 v3).
//
// Episodes of one show added one torrent at a time fold into a single library
// row. Pins how torrents are matched to a show (TMDB id first, parsed name
// otherwise), the order an open group lists its episodes in, and the model
// the grid and list read: group rows, what they add up to, and the session
// rows an action on them reaches.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "bridges/librarygroups.h"
#include "bridges/qmllibrarymodel.h"
#include "bridges/qmlpostermodel.h"

#include <QSettings>
#include <QStandardItemModel>
#include <QStandardPaths>

using LibraryGroups::Member;

static Member ep(const char *show, int season, int episode, int tmdb = 0)
{
    Member m;
    m.nameKey = LibraryGroups::nameKey(QString::fromUtf8(show));
    m.season = season;
    m.episode = episode;
    m.tmdbId = tmdb;
    m.name = QStringLiteral("%1 S%2E%3").arg(QString::fromUtf8(show)).arg(season).arg(episode);
    return m;
}

static Member film(const char *name)
{
    Member m;
    m.name = QString::fromUtf8(name);
    return m;
}

TEST_CASE("nameKey folds punctuation and case", "[librarygroups]")
{
    CHECK(LibraryGroups::nameKey(QStringLiteral("The.Last.of.Us")) == QStringLiteral("the last of us"));
    CHECK(LibraryGroups::nameKey(QStringLiteral("The Last Of Us")) == QStringLiteral("the last of us"));
    CHECK(LibraryGroups::nameKey(QStringLiteral("  Bleach -  ")) == QStringLiteral("bleach"));
    CHECK(LibraryGroups::nameKey(QString()).isEmpty());
}

TEST_CASE("keys prefer the TMDB id and pull unresolved episodes into it", "[librarygroups]")
{
    const QStringList k = LibraryGroups::keys({
        ep("Bleach", 1, 2, 30984),
        ep("Bleach", 1, 3),          // not resolved yet: same name, same show
        ep("Bleach", 1, 4, 99999),   // the resolver says another show
        ep("Frieren", 1, 1),
        film("Dune Part Two 2024"),
    });
    CHECK(k[0] == QStringLiteral("tmdb:30984"));
    CHECK(k[1] == QStringLiteral("tmdb:30984"));
    CHECK(k[2] == QStringLiteral("tmdb:99999"));
    CHECK(k[3] == QStringLiteral("name:frieren"));
    CHECK(k[4].isEmpty());
}

TEST_CASE("a closed group is one row where its first episode was", "[librarygroups]")
{
    const auto rows = LibraryGroups::layout({
        film("Dune Part Two 2024"),
        ep("Bleach", 1, 5),
        film("Oppenheimer 2023"),
        ep("Bleach", 1, 2),
        ep("Bleach", 1, 9),
    }, {});
    REQUIRE(rows.size() == 3);
    CHECK_FALSE(rows[0].group);
    CHECK(rows[0].members == QVector<int>{0});
    CHECK(rows[1].group);
    CHECK(rows[1].groupKey == QStringLiteral("name:bleach"));
    CHECK(rows[1].members == (QVector<int>{3, 1, 4}));   // episode order, not list order
    CHECK_FALSE(rows[2].group);
    CHECK(rows[2].members == QVector<int>{2});
}

TEST_CASE("an open group lists its episodes under it in episode order", "[librarygroups]")
{
    const auto rows = LibraryGroups::layout({
        ep("Bleach", 2, 1),
        ep("Bleach", 1, 10),
        ep("Bleach", 1, -1),   // season 1 pack
        ep("Bleach", 1, 2),
    }, {QStringLiteral("name:bleach")});
    REQUIRE(rows.size() == 5);
    CHECK(rows[0].group);
    QVector<int> order;
    for (int i = 1; i < rows.size(); ++i) {
        CHECK(rows[i].child);
        CHECK(rows[i].groupKey == QStringLiteral("name:bleach"));
        order << rows[i].members.value(0);
    }
    CHECK(order == (QVector<int>{2, 3, 1, 0}));
}

TEST_CASE("a show with one torrent and films stay plain rows", "[librarygroups]")
{
    const auto rows = LibraryGroups::layout({
        ep("Bleach", 1, 1),
        film("Dune Part Two 2024"),
        film("Dune Part Two 2024"),   // same film twice is still not a series
    }, {QStringLiteral("name:bleach")});
    REQUIRE(rows.size() == 3);
    for (const auto &r : rows) {
        CHECK_FALSE(r.group);
        CHECK_FALSE(r.child);
    }
}

// The model, fed by a stand-in for QmlPosterModel through the real filter proxy.

namespace {

struct Fixture {
    QStandardItemModel source;
    QmlTorrentFilterProxy filter;

    Fixture() { filter.setSourceModel(&source); }

    void add(const QString &name, const QString &hash, const QString &showKey,
             int season, int episode, qint64 size, qreal progress,
             const QString &state = QStringLiteral("downloading"), int down = 0, int tmdb = 0)
    {
        auto *it = new QStandardItem;
        it->setData(name, QmlPosterModel::NameRole);
        it->setData(name, QmlPosterModel::MetaTitleRole);
        it->setData(hash, QmlPosterModel::InfoHashRole);
        it->setData(showKey, QmlPosterModel::ShowKeyRole);
        it->setData(showKey.isEmpty() ? QString() : showKey, QmlPosterModel::ShowTitleRole);
        it->setData(tmdb, QmlPosterModel::ShowTmdbRole);
        it->setData(season, QmlPosterModel::SeasonRole);
        it->setData(episode, QmlPosterModel::EpisodeRole);
        it->setData(size, QmlPosterModel::SizeBytesRole);
        it->setData(progress, QmlPosterModel::ProgressRole);
        it->setData(state, QmlPosterModel::StateKeyRole);
        it->setData(state, QmlPosterModel::StateStringRole);
        it->setData(down, QmlPosterModel::DownRateRole);
        it->setData(0, QmlPosterModel::UpRateRole);
        it->setData(QString(), QmlPosterModel::EtaRole);
        it->setData(QString(), QmlPosterModel::CategoryRole);
        source.appendRow(it);
    }
};

QVariant at(const QmlLibraryModel &m, int row, int role)
{
    return m.data(m.index(row), role);
}

QList<int> ints(const QVariantList &l)
{
    QList<int> out;
    for (const QVariant &v : l) out << v.toInt();
    return out;
}

} // namespace

TEST_CASE("model folds episodes into one row and opens it on request", "[librarygroups]")
{
    Fixture f;
    f.add(QStringLiteral("Dune"), QStringLiteral("h0"), {}, -1, -1, 100, 1.0, QStringLiteral("seeding"));
    f.add(QStringLiteral("Bleach S01E03"), QStringLiteral("h1"), QStringLiteral("bleach"), 1, 3, 100, 0.5);
    f.add(QStringLiteral("Bleach S01E01"), QStringLiteral("h2"), QStringLiteral("bleach"), 1, 1, 300, 1.0, QStringLiteral("seeding"));

    QmlLibraryModel m(&f.filter);
    REQUIRE(m.rowCount() == 2);
    CHECK_FALSE(m.isGroup(0));
    CHECK(m.mapToSource(0) == 0);
    REQUIRE(m.isGroup(1));
    CHECK(at(m, 1, QmlLibraryModel::GroupCountRole).toInt() == 2);
    CHECK(m.mapToSource(1) == -1);
    // An action on the group reaches every episode, first episode first.
    CHECK(ints(m.sourceRowsAt(1)) == QList<int>{2, 1});
    // A closed group answers for its episodes.
    CHECK(m.mapFromSource(1) == 1);

    // Progress weighs episodes by size; the state is the one that needs the user.
    CHECK_THAT(at(m, 1, QmlPosterModel::ProgressRole).toReal(), Catch::Matchers::WithinAbs(0.875, 1e-9));
    CHECK(at(m, 1, QmlPosterModel::SizeBytesRole).toLongLong() == 400);
    CHECK(at(m, 1, QmlPosterModel::StateKeyRole).toString() == QStringLiteral("downloading"));
    CHECK(at(m, 1, QmlPosterModel::NameRole).toString() == QStringLiteral("bleach"));

    int structural = 0;
    QObject::connect(&m, &QmlLibraryModel::structureChanged, [&] { ++structural; });
    m.toggleGroup(1);
    CHECK(structural == 1);
    REQUIRE(m.rowCount() == 4);
    CHECK(at(m, 1, QmlLibraryModel::GroupExpandedRole).toBool());
    CHECK(at(m, 2, QmlLibraryModel::GroupChildRole).toBool());
    CHECK(m.mapToSource(2) == 2);   // E01 first
    CHECK(m.mapToSource(3) == 1);   // then E03
    CHECK(m.mapFromSource(1) == 3);

    m.toggleGroup(1);
    CHECK(m.rowCount() == 2);
}

TEST_CASE("model groups the episode that arrives second and splits on removal", "[librarygroups]")
{
    Fixture f;
    f.add(QStringLiteral("Bleach S01E01"), QStringLiteral("h1"), QStringLiteral("bleach"), 1, 1, 100, 0.0);
    QmlLibraryModel m(&f.filter);
    REQUIRE(m.rowCount() == 1);
    CHECK_FALSE(m.isGroup(0));
    const QString lone = m.rowKey(0);

    f.add(QStringLiteral("Bleach S01E02"), QStringLiteral("h2"), QStringLiteral("bleach"), 1, 2, 100, 0.0);
    REQUIRE(m.rowCount() == 1);
    CHECK(m.isGroup(0));
    CHECK(m.rowForKey(lone) == -1);

    f.source.removeRow(0);
    REQUIRE(m.rowCount() == 1);
    CHECK_FALSE(m.isGroup(0));
    CHECK(m.mapToSource(0) == 0);
}

TEST_CASE("revealSource opens the group a torrent is in", "[librarygroups]")
{
    Fixture f;
    f.add(QStringLiteral("Bleach S01E01"), QStringLiteral("h1"), QStringLiteral("bleach"), 1, 1, 100, 0.0);
    f.add(QStringLiteral("Bleach S01E02"), QStringLiteral("h2"), QStringLiteral("bleach"), 1, 2, 100, 0.0);
    QmlLibraryModel m(&f.filter);
    REQUIRE(m.rowCount() == 1);
    const int row = m.revealSource(1);
    CHECK(m.rowCount() == 3);
    CHECK(row == 2);
    CHECK(m.mapToSource(row) == 1);
}

TEST_CASE("filtering down to one episode shows it as a plain row", "[librarygroups]")
{
    Fixture f;
    f.add(QStringLiteral("Bleach S01E01"), QStringLiteral("h1"), QStringLiteral("bleach"), 1, 1, 100, 1.0, QStringLiteral("seeding"));
    f.add(QStringLiteral("Bleach S01E02"), QStringLiteral("h2"), QStringLiteral("bleach"), 1, 2, 100, 0.2);
    QmlLibraryModel m(&f.filter);
    REQUIRE(m.rowCount() == 1);
    m.setFilterState(QStringLiteral("seeding"));
    REQUIRE(m.rowCount() == 1);
    CHECK_FALSE(m.isGroup(0));
    CHECK(m.mapToSource(0) == 0);
    m.setFilterState(QStringLiteral("all"));
    CHECK(m.isGroup(0));
}

TEST_CASE("open groups are remembered across restarts", "[librarygroups]")
{
    QStandardPaths::setTestModeEnabled(true);
    const QString key = QStringLiteral("test/libraryOpenGroups");
    QSettings("BATorrent", "BATorrent").remove(key);

    {
        Fixture f;
        f.add(QStringLiteral("Bleach S01E01"), QStringLiteral("h1"), QStringLiteral("bleach"), 1, 1, 100, 0.0);
        f.add(QStringLiteral("Bleach S01E02"), QStringLiteral("h2"), QStringLiteral("bleach"), 1, 2, 100, 0.0);
        QmlLibraryModel m(&f.filter, key);
        m.setGroupExpanded(0, true);
        CHECK(m.rowCount() == 3);
    }
    {
        Fixture f;
        f.add(QStringLiteral("Bleach S01E02"), QStringLiteral("h2"), QStringLiteral("bleach"), 1, 2, 100, 0.0);
        f.add(QStringLiteral("Bleach S01E01"), QStringLiteral("h1"), QStringLiteral("bleach"), 1, 1, 100, 0.0);
        QmlLibraryModel m(&f.filter, key);
        REQUIRE(m.rowCount() == 3);
        CHECK(m.isGroupExpanded(0));
        CHECK(m.mapToSource(1) == 1);   // E01, even though it was added second
    }
    QSettings("BATorrent", "BATorrent").remove(key);
}

// A tester asked for a franchise's films to sit together. TMDB says which
// collection a film belongs to; two or more films of one fold into a group,
// in release order, and a film of no collection stays a plain row.
TEST_CASE("films of one TMDB collection group, in release order", "[librarygroups][collection]")
{
    auto inCol = [](const char *name, int col, int year) {
        Member m = film(name);
        m.collectionId = col;
        m.year = year;
        return m;
    };
    const QVector<Member> members = {
        inCol("The Return of the King", 119, 2003),
        film("Fight Club"),
        inCol("The Fellowship of the Ring", 119, 2001),
        inCol("The Two Towers", 119, 2002),
        inCol("Toy Story", 10194, 1995),   // alone in its collection: no group
    };
    const QStringList k = LibraryGroups::keys(members);
    CHECK(k[0] == QStringLiteral("col:119"));
    CHECK(k[1].isEmpty());
    CHECK(LibraryGroups::isCollectionKey(k[0]));
    CHECK_FALSE(LibraryGroups::isCollectionKey(QStringLiteral("tmdb:1399")));

    const auto rows = LibraryGroups::layout(members, {QStringLiteral("col:119")});
    REQUIRE(rows.size() == 6);   // the group, its three films, Fight Club, Toy Story
    CHECK(rows[0].group);
    CHECK(rows[0].members == QVector<int>({2, 3, 0}));   // 2001, 2002, 2003
    CHECK(rows[1].child);
    CHECK(rows[1].members == QVector<int>({2}));
    CHECK(rows[4].members == QVector<int>({1}));
    CHECK_FALSE(rows[5].group);

    // an episode never groups by a collection, even if one were set
    Member e = ep("Bleach", 1, 1);
    e.collectionId = 119;
    CHECK(LibraryGroups::keys({e}).first().startsWith(QStringLiteral("name:")));
}
