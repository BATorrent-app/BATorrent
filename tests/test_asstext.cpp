#include <catch2/catch_test_macros.hpp>
#include "services/subtitles/asstext.h"

TEST_CASE("AssText: override codes become formatting, not text", "[subtitles]")
{
    // the line a user saw drawn raw, codes and all
    auto l = AssText::toHtml(QStringLiteral("{\\an8\\i1}Previously on Bleach{\\i0}"));
    CHECK(l.html == QStringLiteral("<i>Previously on Bleach</i>"));
    CHECK(l.align == 8);
    CHECK(l.top());

    // a whole Dialogue event as FFmpeg passes it: the nine fields go
    l = AssText::toHtml(QStringLiteral("12,0,Default,,0,0,0,,Hello\\Nthere"));
    CHECK(l.html == QStringLiteral("Hello<br>there"));
    CHECK(l.align == 2);
    CHECK_FALSE(l.top());

    // bold by weight, underline, a reset, a hard space and HTML that must stay text
    l = AssText::toHtml(QStringLiteral("{\\b700}Big{\\b0} {\\u1}<under>{\\r} 5\\h&\\h6"));
    CHECK(l.html == QStringLiteral("<b>Big</b> <u>&lt;under&gt;</u> 5&nbsp;&amp;&nbsp;6"));

    // legacy \a6 is the top middle; \pos and font tags are dropped quietly
    l = AssText::toHtml(QStringLiteral("{\\a6\\pos(320,40)\\fnArial\\fs40\\c&H00FFFF&}Sign"));
    CHECK(l.html == QStringLiteral("Sign"));
    CHECK(l.align == 8);

    // a vector drawing is not text: nothing is shown
    l = AssText::toHtml(QStringLiteral("{\\p1}m 0 0 l 100 0 100 100 0 100{\\p0}"));
    CHECK(l.html.isEmpty());

    // an open italic is closed at the end
    l = AssText::toHtml(QStringLiteral("{\\i1}never closed"));
    CHECK(l.html == QStringLiteral("<i>never closed</i>"));

    // middle of the screen
    CHECK(AssText::toHtml(QStringLiteral("{\\an5}x")).middle());
}
