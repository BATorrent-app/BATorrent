// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/subtitles/asstext.h"

#include <QRegularExpression>

namespace AssText {

namespace {

// legacy \a counts 1-3 bottom, 5-7 top, 9-11 middle
int legacyAlign(int a)
{
    if (a >= 1 && a <= 3) return a;
    if (a >= 5 && a <= 7) return a + 2;
    if (a >= 9 && a <= 11) return a - 5;
    return 2;
}

}

Line toHtml(const QString &raw)
{
    Line line;
    QString s = raw;
    // "ReadOrder,Layer,Style,Name,MarginL,MarginR,MarginV,Effect,Text" from FFmpeg
    static const QRegularExpression fields(QStringLiteral("^\\d+,-?\\d+,[^,]*,[^,]*,\\d+,\\d+,\\d+,[^,]*,"));
    s.remove(fields);
    // a tag and its argument: \an8, \i1, \fs40, \pos(1,2), \p0
    static const QRegularExpression tagRe(QStringLiteral("\\\\([a-zA-Z]+)(\\([^)]*\\)|[^\\\\]*)"));

    QString out;
    bool italic = false, bold = false, under = false;
    int drawing = 0;
    auto setStyle = [&](bool &flag, bool on, const char *tag) {
        if (flag == on) return;
        flag = on;
        out += on ? QStringLiteral("<%1>").arg(QLatin1String(tag)) : QStringLiteral("</%1>").arg(QLatin1String(tag));
    };

    for (int i = 0; i < s.size(); ++i) {
        const QChar c = s.at(i);
        if (c == QLatin1Char('{')) {
            const int end = s.indexOf(QLatin1Char('}'), i);
            if (end < 0) break;
            const QString block = s.mid(i + 1, end - i - 1);
            auto it = tagRe.globalMatch(block);
            while (it.hasNext()) {
                const auto m = it.next();
                const QString name = m.captured(1).toLower();
                const QString arg = m.captured(2).trimmed();
                if (name == QLatin1String("an")) {
                    const int a = arg.toInt();
                    if (a >= 1 && a <= 9) line.align = a;
                } else if (name == QLatin1String("a")) {
                    line.align = legacyAlign(arg.toInt());
                } else if (name == QLatin1String("i")) {
                    setStyle(italic, arg != QLatin1String("0"), "i");
                } else if (name == QLatin1String("b")) {
                    // \b1, \b0, or a weight (\b700)
                    const int w = arg.isEmpty() ? 1 : arg.toInt();
                    setStyle(bold, w == 1 || w >= 600, "b");
                } else if (name == QLatin1String("u")) {
                    setStyle(under, arg != QLatin1String("0"), "u");
                } else if (name == QLatin1String("p")) {
                    drawing = arg.toInt();
                } else if (name == QLatin1String("r")) {
                    setStyle(italic, false, "i");
                    setStyle(bold, false, "b");
                    setStyle(under, false, "u");
                }
            }
            i = end;
            continue;
        }
        if (drawing > 0) continue;   // vector drawing commands, not text
        if (c == QLatin1Char('\\') && i + 1 < s.size()) {
            const QChar n = s.at(i + 1);
            if (n == QLatin1Char('N') || n == QLatin1Char('n')) { out += QStringLiteral("<br>"); ++i; continue; }
            if (n == QLatin1Char('h')) { out += QStringLiteral("&nbsp;"); ++i; continue; }
        }
        if (c == QLatin1Char('\n')) { out += QStringLiteral("<br>"); continue; }
        if (c == QLatin1Char('\r')) continue;
        if (c == QLatin1Char('&')) out += QStringLiteral("&amp;");
        else if (c == QLatin1Char('<')) out += QStringLiteral("&lt;");
        else if (c == QLatin1Char('>')) out += QStringLiteral("&gt;");
        else out += c;
    }
    setStyle(italic, false, "i");
    setStyle(bold, false, "b");
    setStyle(under, false, "u");
    // nothing but markup left (a drawing, an empty line): show nothing
    static const QRegularExpression onlyMarkup(QStringLiteral("^(\\s|<[^>]*>|&nbsp;)*$"));
    while (out.endsWith(QLatin1String("<br>"))) out.chop(4);
    line.html = onlyMarkup.match(out).hasMatch() ? QString() : out.trimmed();
    return line;
}

} // namespace AssText
