#include <catch2/catch_test_macros.hpp>
#include "torrent/sessionresume.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

using SessionResume::RemovalDisposition;

TEST_CASE("removalDisposition: deleteFiles × permanent × targets",
          "[sessionremove]")
{
    const QStringList targets{QStringLiteral("/dl/a"), QStringLiteral("/dl/a.!bt")};
    const QStringList empty;

    REQUIRE(SessionResume::removalDisposition(false, false, targets)
            == RemovalDisposition::Keep);
    REQUIRE(SessionResume::removalDisposition(false, true, targets)
            == RemovalDisposition::Keep);
    REQUIRE(SessionResume::removalDisposition(true, false, empty)
            == RemovalDisposition::Keep);
    REQUIRE(SessionResume::removalDisposition(true, true, empty)
            == RemovalDisposition::Keep);

    REQUIRE(SessionResume::removalDisposition(true, false, targets)
            == RemovalDisposition::Trash);
    REQUIRE(SessionResume::removalDisposition(true, true, targets)
            == RemovalDisposition::PermanentDelete);
}

TEST_CASE("removalDisposition: missing disk files still schedule when listed",
          "[sessionremove]")
{
    // Targets are built from torrent mapping, not from a pre-flight exists()
    // check — absent paths are skipped later by existingRemovalTargets.
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    const QString present = tmp.filePath(QStringLiteral("present.bin"));
    const QString missing = tmp.filePath(QStringLiteral("gone.bin"));
    {
        QFile f(present);
        REQUIRE(f.open(QIODevice::WriteOnly));
        f.write("x");
    }
    REQUIRE_FALSE(QFileInfo::exists(missing));

    const QStringList targets{present, missing,
                              present + QLatin1String(".!bt"),
                              tmp.filePath(QStringLiteral(".deadbeef.parts"))};
    REQUIRE(SessionResume::removalDisposition(true, false, targets)
            == RemovalDisposition::Trash);
    REQUIRE(SessionResume::removalDisposition(true, true, targets)
            == RemovalDisposition::PermanentDelete);

    const auto existing = SessionResume::existingRemovalTargets(targets);
    REQUIRE(existing == QStringList{present});
    REQUIRE(SessionResume::existingRemovalTargets(
                {missing, missing + QLatin1String(".!bt")})
                .isEmpty());
}

TEST_CASE("trashTargetsForRemoval on temp savePath: both variants + parts",
          "[sessionremove]")
{
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    const QString save = tmp.path();
    const auto tops = SessionResume::topLevelTrashNames({
        QStringLiteral("Show/S01/ep.mkv.!bt"),
        QStringLiteral("orphan.bin"),
    });
    const auto targets = SessionResume::trashTargetsForRemoval(
        save, tops, QStringLiteral("abcd1234"));

    REQUIRE(targets.contains(QDir(save).filePath(QStringLiteral("Show"))));
    REQUIRE(targets.contains(QDir(save).filePath(QStringLiteral("Show.!bt"))));
    REQUIRE(targets.contains(QDir(save).filePath(QStringLiteral("orphan.bin"))));
    REQUIRE(targets.contains(QDir(save).filePath(QStringLiteral("orphan.bin.!bt"))));
    REQUIRE(targets.contains(QDir(save).filePath(QStringLiteral(".abcd1234.parts"))));

    // None of the mapped targets need to exist for scheduling.
    REQUIRE(SessionResume::existingRemovalTargets(targets).isEmpty());
    REQUIRE(SessionResume::removalDisposition(true, false, targets)
            == RemovalDisposition::Trash);
}
