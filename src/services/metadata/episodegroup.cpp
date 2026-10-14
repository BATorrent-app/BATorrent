// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/metadata/episodegroup.h"
#include "services/metadata/nameparser.h"

#include <QRegularExpression>

namespace {

// Letters and digits only, accents dropped: "Bleach: Sennen-Kessen" and
// "bleach sennen kessen" fold the same.
QString fold(const QString &s)
{
    const QString d = s.normalized(QString::NormalizationForm_D).toLower();
    QString out;
    for (const QChar &c : d)
        if (c.isLetterOrNumber() && c.category() != QChar::Mark_NonSpacing)
            out.append(c);
    return out;
}

// An anime release numbered from 1: "[Group] Title - NN", no SxxExx anywhere.
// Returns the season named inside the title ("Title S2", "Title 2nd Season"),
// 1 when it names none, or -1 when the title is the show's name plus more
// ("Bleach - Sennen Kessen-hen" for "Bleach", "Naruto Shippuden" for
// "Naruto"): a sequel or arc with its own numbering, not season 1. Returns 0
// when the name is not of that shape.
int absoluteSeason(const QString &releaseName, const QStringList &workTitles)
{
    static const QRegularExpression explicitRe(
        QStringLiteral(R"(S\d{1,2}[ ._]?E\d{1,3}|\b\d{1,2}x\d{2,3}\b)"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression animeRe(
        QStringLiteral(R"(^\s*\[[^\]]+\]\s*(.+?)\s+-\s+(\d{1,4})(?=\s|\[|\(|\.|$))"));
    if (explicitRe.match(releaseName).hasMatch()) return 0;
    const QRegularExpressionMatch am = animeRe.match(releaseName);
    if (!am.hasMatch()) return 0;

    QString title = am.captured(1);
    int season = 1;
    static const QRegularExpression seasonRes[] = {
        QRegularExpression(QStringLiteral(R"(\bS(\d{1,2})\b)"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral(R"(\b(?:Season|Saison)[ ._]?(\d{1,2})\b)"),
                           QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral(R"(\b(\d{1,2})(?:st|nd|rd|th)[ ._]Season\b)"),
                           QRegularExpression::CaseInsensitiveOption),
    };
    for (const auto &re : seasonRes) {
        const QRegularExpressionMatch m = re.match(title);
        if (!m.hasMatch()) continue;
        season = m.captured(1).toInt();
        title.remove(m.capturedStart(), m.capturedLength());
        break;
    }
    static const QRegularExpression bracketRe(QStringLiteral(R"(\([^)]*\)|\[[^\]]*\])"));
    title.remove(bracketRe);
    const QString t = fold(title);
    for (const QString &w : workTitles) {
        const QString f = fold(w);
        if (f.isEmpty() || !t.startsWith(f) || t.size() == f.size()) continue;
        // a year or a number after the name is not another show
        const QString rest = t.mid(f.size());
        for (const QChar &c : rest)
            if (c.isLetter()) return -1;
    }
    return season;
}

} // namespace

EpisodeTag EpisodeGroup::classify(const QString &releaseName, const QStringList &workTitles)
{
    EpisodeTag tag;

    // A run of episodes is the season, not its first episode: "S01E01-E08",
    // "S01E01-08", "S01E01 to E08". Read as SxxExx it sat a whole 27 GB
    // season on episode 1.
    static const QRegularExpression epRangeRe(
        QStringLiteral("\\bS(\\d{1,2})[ ._]?E(\\d{1,3})[ ._]?(?:-|~|to)[ ._]?(?:S\\d{1,2}[ ._]?)?E?(\\d{1,3})\\b"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch range = epRangeRe.match(releaseName);
    if (range.hasMatch() && range.captured(3).toInt() > range.captured(2).toInt()) {
        tag.season = range.captured(1).toInt();
        tag.pack = true;
        return tag;
    }

    // "[Erai-raws] Bleach - 001 ~ 366": an anime batch, not episode 1 of it.
    // Numbered from 1 like its episodes, so it sits on season 1 with them.
    static const QRegularExpression animeRangeRe(
        QStringLiteral(R"(^\s*\[[^\]]+\]\s*.+?\s+-\s+(\d{1,4})\s*(?:~|-|to)\s*(\d{1,4})(?=\s|\[|\(|$))"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch ar = animeRangeRe.match(releaseName);
    if (ar.hasMatch() && ar.captured(2).toInt() > ar.captured(1).toInt()) {
        // the batch's first episode answers whose numbering it is
        const int season = absoluteSeason(releaseName.left(ar.capturedStart(2)).trimmed()
                                              .chopped(1).trimmed(), workTitles);
        if (season < 0) return tag;   // a sequel's or arc's batch
        tag.season = season > 0 ? season : 1;
        tag.pack = true;
        return tag;
    }

    const ParsedName p = NameParser::parse(releaseName);
    if (p.episode > 0) {
        // anime absolute numbering has no on-screen season: NameParser pins it
        // to 1, which is only right when the title is the show's own name
        const int absolute = absoluteSeason(releaseName, workTitles);
        if (absolute < 0) return tag;
        tag.season = absolute > 0 ? absolute : (p.season > 0 ? p.season : 1);
        tag.episode = p.episode;
        return tag;
    }

    // "S01 E05" / "S01.E05": separated forms NameParser's strict SxxExx misses
    static const QRegularExpression spacedSeRe(
        QStringLiteral("\\bS(\\d{1,2})[ ._-]+E(\\d{1,3})\\b"),
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch m = spacedSeRe.match(releaseName);
    if (m.hasMatch()) {
        tag.season = m.captured(1).toInt();
        tag.episode = m.captured(2).toInt();
        return tag;
    }

    // multi-season range → whole-series pack: "S01-S05", "Season 1-3"
    static const QRegularExpression rangeRe(
        QStringLiteral("\\bS(?:easons?)?[ ._]?(\\d{1,2})[ ._]?[-~][ ._]?S?(?:easons?[ ._]?)?(\\d{1,2})\\b"),
        QRegularExpression::CaseInsensitiveOption);
    if (rangeRe.match(releaseName).hasMatch()) {
        tag.pack = true;
        return tag;
    }

    // season-only marker → season pack. Checked in priority order: a single
    // alternation would let "Round 6 Temporada 2" resolve to 6 ("6 Temporada"
    // ordinal form) because the leftmost match wins across alternatives.
    static const QRegularExpression wordSeasonRe(
        QStringLiteral("\\b(?:Season|Temporada|Saison|Staffel)[ ._]?(\\d{1,2})\\b"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression ordinalSeasonRe(
        QStringLiteral("\\b(\\d{1,2})\\s*[ªº]?\\s*Temporada\\b"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression bareSRe(
        QStringLiteral("\\bS(\\d{1,2})\\b"),
        QRegularExpression::CaseInsensitiveOption);
    for (const auto *re : { &wordSeasonRe, &ordinalSeasonRe, &bareSRe }) {
        m = re->match(releaseName);
        if (m.hasMatch()) {
            tag.season = m.captured(1).toInt();
            tag.pack = true;
            return tag;
        }
    }

    // complete/batch keywords → whole-series pack
    static const QRegularExpression completeRe(
        QStringLiteral("\\b(?:complete|completa|completo|batch|full[ ._]?series|all[ ._]?seasons|"
                       "todas[ ._]as[ ._]temporadas|cole[cç][aã]o|collection|int[eé]grale)\\b"),
        QRegularExpression::CaseInsensitiveOption);
    if (completeRe.match(releaseName).hasMatch()) {
        tag.pack = true;
        return tag;
    }

    return tag;
}

EpisodeFile EpisodeGroup::pickFile(const QStringList &paths, const QList<qint64> &sizes,
                                   int fileIdx, const QString &fileName, int season, int episode)
{
    static const QStringList videoExts = { QStringLiteral(".mp4"), QStringLiteral(".mkv"),
        QStringLiteral(".avi"), QStringLiteral(".mov"), QStringLiteral(".wmv"), QStringLiteral(".flv"),
        QStringLiteral(".webm"), QStringLiteral(".m4v"), QStringLiteral(".ts") };
    const auto leaf = [](QString p) {
        if (p.endsWith(QStringLiteral(".!bt"))) p.chop(4);
        p.replace(QLatin1Char('\\'), QLatin1Char('/'));
        return p.section(QLatin1Char('/'), -1);
    };
    const auto isVideo = [&](int i) {
        const QString l = leaf(paths.at(i));
        for (const QString &ext : videoExts)
            if (l.endsWith(ext, Qt::CaseInsensitive)) return true;
        return false;
    };
    const auto sizeAt = [&](int i) { return i < sizes.size() ? sizes.at(i) : 0; };

    if (!fileName.isEmpty()) {
        const QString want = leaf(fileName);
        for (int i = 0; i < paths.size(); ++i)
            if (leaf(paths.at(i)).compare(want, Qt::CaseInsensitive) == 0) return { i, QStringLiteral("filename") };
    }
    if (fileIdx >= 0 && fileIdx < paths.size() && isVideo(fileIdx))
        return { fileIdx, QStringLiteral("fileIdx") };

    int best = -1;
    if (episode > 0) {
        for (int i = 0; i < paths.size(); ++i) {
            if (!isVideo(i)) continue;
            const EpisodeTag t = classify(leaf(paths.at(i)));
            if (t.pack || t.episode != episode) continue;
            if (season >= 0 && t.season >= 0 && t.season != season) continue;
            if (best < 0 || sizeAt(i) > sizeAt(best)) best = i;
        }
        if (best >= 0) return { best, QStringLiteral("episode") };
    }
    for (int i = 0; i < paths.size(); ++i)
        if (isVideo(i) && (best < 0 || sizeAt(i) > sizeAt(best))) best = i;
    return { best, best >= 0 ? QStringLiteral("largest") : QString() };
}
