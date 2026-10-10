#include <catch2/catch_test_macros.hpp>
#include "services/discovery/logotone.h"

#include <QImage>
#include <QPainter>

namespace {

// Lettering on a transparent box, the way logos ship.
QImage logo(QColor ink, QColor accent = QColor())
{
    QImage img(400, 120, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.fillRect(20, 30, 300, 60, ink);
    if (accent.isValid()) p.fillRect(320, 30, 40, 60, accent);
    return img;
}

}

TEST_CASE("a black or charcoal logo reads as dark", "[logotone]") {
    REQUIRE(LogoTone::isDark(logo(Qt::black)));
    REQUIRE(LogoTone::isDark(logo(QColor(60, 60, 60))));
    // The Witcher's case: grey lettering with a small red mark still drowns.
    REQUIRE(LogoTone::isDark(logo(QColor(55, 55, 58), QColor(200, 20, 20))));
}

TEST_CASE("white, gold and bright logos are left alone", "[logotone]") {
    REQUIRE_FALSE(LogoTone::isDark(logo(Qt::white)));
    REQUIRE_FALSE(LogoTone::isDark(logo(QColor(214, 170, 90))));   // Elden Ring's gold
    REQUIRE_FALSE(LogoTone::isDark(logo(QColor(30, 130, 70), Qt::white)));
}

TEST_CASE("transparency never counts, and nothing is not dark", "[logotone]") {
    QImage empty(200, 80, QImage::Format_ARGB32);
    empty.fill(Qt::transparent);
    REQUIRE_FALSE(LogoTone::isDark(empty));
    REQUIRE_FALSE(LogoTone::isDark(QImage()));
    // A black box that is almost fully transparent is not a dark logo's ink.
    QImage ghost(200, 80, QImage::Format_ARGB32);
    ghost.fill(QColor(0, 0, 0, 20));
    REQUIRE_FALSE(LogoTone::isDark(ghost));
}
