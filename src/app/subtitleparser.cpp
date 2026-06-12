// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "subtitleparser.h"

#include <QFile>
#include <QRegularExpression>
#include <QStringConverter>
#include <QStringDecoder>

#include <algorithm>

namespace {

// "01:02:03,456" (SRT) or "01:02:03.456" / "02:03.456" (VTT) → ms
qint64 parseTimestamp(const QString &ts)
{
    static const QRegularExpression re(
        QStringLiteral("^(?:(\\d{1,2}):)?(\\d{1,2}):(\\d{1,2})[.,](\\d{1,3})$"));
    const auto m = re.match(ts.trimmed());
    if (!m.hasMatch()) return -1;
    const qint64 h = m.captured(1).isEmpty() ? 0 : m.captured(1).toLongLong();
    const qint64 mi = m.captured(2).toLongLong();
    const qint64 s = m.captured(3).toLongLong();
    const qint64 ms = m.captured(4).leftJustified(3, QLatin1Char('0')).toLongLong();
    return ((h * 60 + mi) * 60 + s) * 1000 + ms;
}

// keep <i>/<b>/<u> (+ closers) for Text.StyledText; drop ASS overrides
// ("{\an8}") and any other angle tag (<font>, <c.yellow>, <v Name>, ...)
QString cleanText(QString t)
{
    static const QRegularExpression assTag(QStringLiteral("\\{\\\\[^}]*\\}"));
    static const QRegularExpression otherTag(
        QStringLiteral("</?(?!/?[ibu]>)[a-zA-Z][^>]*>"));
    t.replace(assTag, QString());
    t.replace(otherTag, QString());
    return t.trimmed();
}

QList<SubtitleCue> parseBlocks(const QStringList &lines, bool vtt)
{
    QList<SubtitleCue> cues;
    static const QRegularExpression arrow(QStringLiteral("\\s-->\\s"));
    int i = 0;
    const int n = lines.size();
    while (i < n) {
        // skip blank lines and (VTT) comment/metadata blocks
        if (lines[i].trimmed().isEmpty()) { ++i; continue; }
        if (vtt) {
            const QString head = lines[i].trimmed();
            if (head.startsWith(QLatin1String("WEBVTT")) || head.startsWith(QLatin1String("NOTE"))
                || head.startsWith(QLatin1String("STYLE")) || head.startsWith(QLatin1String("REGION"))) {
                while (i < n && !lines[i].trimmed().isEmpty()) ++i;
                continue;
            }
        }

        // optional cue identifier line (SRT index / VTT cue id)
        QString timing = lines[i];
        if (!timing.contains(QLatin1String("-->")) && i + 1 < n && lines[i + 1].contains(QLatin1String("-->")))
            timing = lines[++i];
        if (!timing.contains(QLatin1String("-->"))) { ++i; continue; }   // garbage block

        const QStringList parts = timing.split(arrow);
        if (parts.size() < 2) { ++i; continue; }
        // VTT puts cue settings ("align:start position:10%") after the end time
        const QString endPart = parts[1].trimmed().section(QLatin1Char(' '), 0, 0);
        const qint64 start = parseTimestamp(parts[0]);
        const qint64 end = parseTimestamp(endPart);

        QStringList textLines;
        ++i;
        while (i < n && !lines[i].trimmed().isEmpty())
            textLines << lines[i++];

        if (start < 0 || end < 0 || end <= start) continue;   // malformed: skip cue
        // clean per line, then join — cleanText would strip the <br/> separators
        QStringList cleaned;
        for (const QString &tl : textLines) {
            const QString c = cleanText(tl);
            if (!c.isEmpty()) cleaned << c;
        }
        if (!cleaned.isEmpty())
            cues.append({start, end, cleaned.join(QStringLiteral("<br/>"))});
    }
    std::sort(cues.begin(), cues.end(),
              [](const SubtitleCue &a, const SubtitleCue &b) { return a.startMs < b.startMs; });
    return cues;
}

} // namespace

namespace SubtitleParser {

QList<SubtitleCue> parseSrt(const QString &content)
{
    return parseBlocks(content.split(QRegularExpression(QStringLiteral("\r?\n"))), false);
}

QList<SubtitleCue> parseVtt(const QString &content)
{
    return parseBlocks(content.split(QRegularExpression(QStringLiteral("\r?\n"))), true);
}

QList<SubtitleCue> parseFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    QByteArray raw = f.readAll();

    QStringDecoder utf8(QStringConverter::Utf8, QStringConverter::Flag::Stateless);
    QString content = utf8.decode(raw);
    if (utf8.hasError())
        content = QStringDecoder(QStringConverter::Latin1).decode(raw);
    if (!content.isEmpty() && content.front() == QChar(0xFEFF))
        content.remove(0, 1);

    const bool vtt = path.endsWith(QLatin1String(".vtt"), Qt::CaseInsensitive)
                     || content.startsWith(QLatin1String("WEBVTT"));
    return vtt ? parseVtt(content) : parseSrt(content);
}

}
