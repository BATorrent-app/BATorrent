// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "nameparser.h"
#include <QSet>
#include <algorithm>

#include <QRegularExpression>

ParsedName NameParser::parse(const QString &rawName)
{
    ParsedName result;
    QString work = rawName;

    // Token scores accumulated across the strip pipeline. The final type is the
    // winner of game vs video (Series is decided earlier by SxxExx markers).
    int gameScore = 0;
    int videoScore = 0;

    // Strip a leading release-site prefix: "www.foo.com - ", "[foo.net]",
    // "bar.org_", etc. These pollute the title so the TMDB/IGDB lookup misses
    // (e.g. "www.sitedotorrent.com - euphoria s3ep7" → "euphoria"). Requires a
    // separator after the domain so a bare name like "Doom.com" isn't eaten.
    static const QRegularExpression siteRe(
        QStringLiteral("^\\s*[\\[(]?\\s*(?:[\\w\\-]+\\.)+(?:com|net|org|tv|me|info|io|cc|to|se|club|site|xyz|top|biz|us|uk|pro|app|online|life|eu|ru|in|ws|la|pw|is|ag|tw|st|sx|cr|nu)\\b[\\s\\]\\)\\-_:]+"),
        QRegularExpression::CaseInsensitiveOption);
    work.remove(siteRe);

    // Strip file extension
    static const QRegularExpression extRe(QStringLiteral("\\.(bin|mkv|avi|mp4|mov|wmv|flv|webm|m4v|ts|iso|rar|zip|7z|torrent)$"),
        QRegularExpression::CaseInsensitiveOption);
    work.remove(extRe);

    // Strip version numbers (v1.12, v2.3.1, etc.)
    static const QRegularExpression versionRe(QStringLiteral("[.\\s\\-]v\\d+(?:\\.\\d+)*"),
        QRegularExpression::CaseInsensitiveOption);
    work.remove(versionRe);

    // Strip common game/release tags (use word boundary to avoid eating delimiters)
    static const QRegularExpression gameTagRe(QStringLiteral("[.\\s\\-](?:DLC|Update|Patch|Build\\.?\\d*|Incl|MULTi\\d*)(?=[.\\s\\-]|$)"),
        QRegularExpression::CaseInsensitiveOption);
    work.remove(gameTagRe);

    // Strip edition / release qualifiers that aren't part of the real title and
    // sink the IGDB/TMDB search (e.g. "Hades II Early Access" returns nothing,
    // "... Complete Edition" is noise). Both the "<X> Edition" forms and the
    // common standalone qualifiers.
    static const QRegularExpression editionRe(QStringLiteral(
        "\\b(?:(?:Deluxe|Complete|Definitive|Ultimate|Standard|Gold|Premium|Collector'?s|"
        "Anniversary|Enhanced|Legacy|Day[ .]One|Special|Limited|Digital|Game[ .]of[ .]the[ .]Year)"
        "[ .]Edition|Early[ .]Access|Game[ .]of[ .]the[ .]Year|GOTY|Anthology)\\b"),
        QRegularExpression::CaseInsensitiveOption);
    work.remove(editionRe);

    static const QRegularExpression seRe(
        QStringLiteral("S(\\d{1,2})E(\\d{1,3})"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression altSeRe(
        QStringLiteral("(\\d{1,2})x(\\d{2,3})"),
        QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatch m = seRe.match(work);
    if (m.hasMatch()) {
        result.season = m.captured(1).toInt();
        result.episode = m.captured(2).toInt();
        result.contentType = ContentType::Series;
        // The show title is everything *before* the SxxExx marker; the rest is
        // the episode name (e.g. "in God We trust"), which would break the
        // TMDB lookup. Drop it.
        work.truncate(m.capturedStart());
    } else {
        m = altSeRe.match(work);
        if (m.hasMatch()) {
            result.season = m.captured(1).toInt();
            result.episode = m.captured(2).toInt();
            result.contentType = ContentType::Series;
            work.remove(m.capturedStart(), m.capturedLength());
        }
    }

    // Anime fansub convention: "[Group] Title - NN (...)" — the number after the
    // " - " is the absolute episode. Gated on a leading [group] bracket so it
    // never fires on "Adele - 30" (an album) or "Movie - 2" (a sequel).
    if (result.season < 0) {
        static const QRegularExpression animeRe(
            QStringLiteral("^\\s*\\[[^\\]]+\\]\\s*(.+?)\\s+-\\s+(\\d{1,4})(?=\\s|\\[|\\(|$)"));
        const QRegularExpressionMatch am = animeRe.match(work);
        if (am.hasMatch()) {
            result.season = 1;
            result.episode = am.captured(2).toInt();
            result.contentType = ContentType::Series;
            work = am.captured(1);
        }
    }

    static const QStringList repackers = {
        QStringLiteral("FitGirl"), QStringLiteral("DODI"), QStringLiteral("CODEX"),
        QStringLiteral("PLAZA"), QStringLiteral("RUNE"), QStringLiteral("EMPRESS"),
        QStringLiteral("ElAmigos"), QStringLiteral("Xatab"), QStringLiteral("R.G.Mechanics"),
        QStringLiteral("GOG"), QStringLiteral("SKIDROW"), QStringLiteral("RELOADED"),
        QStringLiteral("TiNYiSO"), QStringLiteral("HOODLUM"), QStringLiteral("DARKSiDERS"),
        QStringLiteral("TENOKE"), QStringLiteral("SiMPLEX"), QStringLiteral("RAZOR1911"),
        QStringLiteral("CPY"), QStringLiteral("STEAMPUNKS"), QStringLiteral("3DM"),
        QStringLiteral("ALI213"), QStringLiteral("PROPHET")
    };

    for (const QString &repacker : repackers) {
        QString escaped = QRegularExpression::escape(repacker);
        QRegularExpression repackRe(
            QStringLiteral("(?:^|[.\\s\\-\\[\\]()])(%1)(?:$|[.\\s\\-\\[\\]()])").arg(escaped),
            QRegularExpression::CaseInsensitiveOption);
        m = repackRe.match(work);
        if (m.hasMatch()) {
            gameScore += 10;                 // a known repacker is a strong game signal
            result.repackerTag = repacker;
            work.remove(m.capturedStart(1), m.capturedLength(1));
            break;
        }
    }

    // Game-specific tokens that aren't repacker group names. Unambiguous enough
    // not to appear in movie/series titles (platform/DRM/crack markers).
    static const QStringList gameTokens = {
        QStringLiteral("Goldberg"), QStringLiteral("Denuvo"), QStringLiteral("CrackFix"),
        QStringLiteral("SteamRip"), QStringLiteral("Steam-Rip"), QStringLiteral("DRM-Free"),
        QStringLiteral("DRMFree"), QStringLiteral("NSP"), QStringLiteral("XCI"),
        QStringLiteral("RPCS3"), QStringLiteral("Ryujinx"), QStringLiteral("RGH"),
        QStringLiteral("JTAG"), QStringLiteral("PS3"), QStringLiteral("PSVita")
    };
    for (const QString &tok : gameTokens) {
        QString escaped = QRegularExpression::escape(tok);
        QRegularExpression tokRe(
            QStringLiteral("(?:^|[.\\s\\-\\[\\]()])(%1)(?:$|[.\\s\\-\\[\\]()])").arg(escaped),
            QRegularExpression::CaseInsensitiveOption);
        m = tokRe.match(work);
        if (m.hasMatch()) {
            gameScore += 3;
            work.remove(m.capturedStart(1), m.capturedLength(1));
        }
    }

    static const QStringList mediaTags = {
        QStringLiteral("2160p"), QStringLiteral("4K"), QStringLiteral("UHD"),
        QStringLiteral("1080p"), QStringLiteral("720p"), QStringLiteral("480p"),
        QStringLiteral("576p"),
        QStringLiteral("BluRay"), QStringLiteral("Blu-Ray"), QStringLiteral("BDRip"),
        QStringLiteral("BRRip"), QStringLiteral("WEB-DL"), QStringLiteral("WEBRip"),
        QStringLiteral("WEB"), QStringLiteral("HDRip"), QStringLiteral("DVDRip"),
        QStringLiteral("HDTV"), QStringLiteral("PDTV"), QStringLiteral("DVDScr"),
        QStringLiteral("CAM"), QStringLiteral("TS"), QStringLiteral("TELESYNC"),
        QStringLiteral("HDCAM"),
        QStringLiteral("x264"), QStringLiteral("x265"), QStringLiteral("H.264"),
        QStringLiteral("H.265"), QStringLiteral("HEVC"), QStringLiteral("AV1"),
        QStringLiteral("AVC"), QStringLiteral("XviD"), QStringLiteral("DivX"),
        QStringLiteral("VP9"), QStringLiteral("10bit"),
        QStringLiteral("AAC"), QStringLiteral("AC3"), QStringLiteral("DTS"),
        QStringLiteral("DTS-HD"), QStringLiteral("Atmos"), QStringLiteral("TrueHD"),
        QStringLiteral("FLAC"), QStringLiteral("MP3"), QStringLiteral("EAC3"),
        QStringLiteral("DD5.1"), QStringLiteral("7.1"),
        QStringLiteral("REMUX"), QStringLiteral("PROPER"),
        // "REPACK" intentionally NOT here — it's ambiguous (game repack vs movie
        // re-release). Movie repacks also carry resolution+codec, so dropping it
        // keeps movie detection while stopping "Game [Repack]" from scoring video.
        QStringLiteral("EXTENDED"), QStringLiteral("UNRATED"), QStringLiteral("DIRECTOR"),
        QStringLiteral("IMAX"), QStringLiteral("3D"), QStringLiteral("HDR"),
        QStringLiteral("HDR10"), QStringLiteral("DV"), QStringLiteral("DoVi"),
        QStringLiteral("Dual.Audio"), QStringLiteral("MULTI")
    };

    bool mediaTagFound = false;
    for (const QString &tag : mediaTags) {
        QString escaped = QRegularExpression::escape(tag);
        QRegularExpression tagRe(
            QStringLiteral("(?:^|[.\\s\\-\\[\\]()])(%1)(?:$|[.\\s\\-\\[\\]()])").arg(escaped),
            QRegularExpression::CaseInsensitiveOption);
        m = tagRe.match(work);
        if (m.hasMatch()) {
            mediaTagFound = true;
            videoScore += 1;            // resolution/codec/source/audio → video lean
            work.remove(m.capturedStart(1), m.capturedLength(1));
        }
    }

    // Audio channel layouts leak into the title ("DDP5.1" -> "Ddp5 1") because
    // codec+channel combos are too many to list. Strip "[codec]N.N" tokens
    // (DDP5.1, DD+5.1, 7.1, 2.0, TrueHD7.1…) generically.
    static const QRegularExpression audioRe(
        QStringLiteral("[.\\s\\-](?:DDPA|DDP|DD\\+|TrueHD|Atmos|EAC3|AC3|DTS(?:-?HD)?|AAC|FLAC|DD)?[ .]?\\d\\.\\d(?=[.\\s\\-\\[\\]()]|$)"),
        QRegularExpression::CaseInsensitiveOption);
    if (mediaTagFound) work.remove(audioRe);

    // Type by score (Series was already decided by SxxExx/anime markers above).
    if (result.contentType == ContentType::Series) {
        result.typeConfidence = 0.9;
    } else if (gameScore > 0 && gameScore >= videoScore) {
        result.contentType = ContentType::Game;
    } else if (videoScore > 0) {
        result.contentType = ContentType::Movie;
    }
    if (result.contentType != ContentType::Series) {
        const int top = std::max(gameScore, videoScore);
        const int low = std::min(gameScore, videoScore);
        result.typeConfidence = top == 0 ? 0.0 : double(top - low) / double(top);
    }

    static const QRegularExpression yearRe(QStringLiteral("\\b((?:19|20)(?:0|1|2)\\d)\\b"));
    QRegularExpressionMatchIterator it = yearRe.globalMatch(work);
    QRegularExpressionMatch lastYearMatch;
    while (it.hasNext())
        lastYearMatch = it.next();
    if (lastYearMatch.hasMatch()) {
        result.year = lastYearMatch.captured(1).toInt();
        work.remove(lastYearMatch.capturedStart(), lastYearMatch.capturedLength());
    }

    static const QRegularExpression bracketRe(QStringLiteral("\\[[^\\]]*\\]|\\([^)]*\\)"));
    work.remove(bracketRe);

    work.replace(QLatin1Char('.'), QLatin1Char(' '));
    work.replace(QLatin1Char('_'), QLatin1Char(' '));

    // RuTracker titles are often bilingual ("Ведьмак 3 / The Witcher 3"). TMDB/
    // IGDB are Latin-indexed, so keep the most-Latin half (split on "/"), or just
    // drop Cyrillic runs when there's no separator. Never strip to nothing.
    static const QRegularExpression cyrillicRe(QStringLiteral("[\\x{0400}-\\x{04FF}]+"));
    static const QRegularExpression latinRe(QStringLiteral("[A-Za-z]"));
    if (work.contains(cyrillicRe)) {
        if (work.contains(QLatin1Char('/'))) {
            QString best; qsizetype bestLatin = -1;
            const auto parts = work.split(QLatin1Char('/'), Qt::SkipEmptyParts);
            for (const QString &p : parts) {
                const qsizetype n = p.count(latinRe);
                if (n > bestLatin) { bestLatin = n; best = p; }
            }
            if (bestLatin > 0) work = best;
        } else if (work.contains(latinRe)) {
            QString latin = work;
            latin.remove(cyrillicRe);
            if (latin.contains(latinRe)) work = latin;
        }
    }

    static const QRegularExpression trailingGroupRe(QStringLiteral("\\s*-\\s*[A-Za-z0-9]+$"));
    work.remove(trailingGroupRe);

    static const QRegularExpression multiSpace(QStringLiteral("\\s{2,}"));
    work.replace(multiSpace, QStringLiteral(" "));
    // strip leftover separators stranded at the edges after tag removal
    // (e.g. "Hades II-" once the -DODI repacker is gone)
    static const QRegularExpression edgeJunk(QStringLiteral("^[\\s.\\-_]+|[\\s.\\-_]+$"));
    work.remove(edgeJunk);
    work = work.trimmed();

    // Roman numerals stay upper-case (Hades II, Final Fantasy VII); a naive
    // toLower+capitalize would wrongly produce "Ii"/"Vii".
    static const QRegularExpression romanRe(
        QStringLiteral("^(?=[mdclxvi]+$)m{0,3}(?:cm|cd|d?c{0,3})(?:xc|xl|l?x{0,3})(?:ix|iv|v?i{0,3})$"),
        QRegularExpression::CaseInsensitiveOption);
    // English words that are coincidentally valid roman numerals — don't shout
    // these to upper-case ("Mix" must not become "MIX").
    static const QSet<QString> romanFalsePositives = {
        QStringLiteral("mix"), QStringLiteral("di"), QStringLiteral("mi"),
        QStringLiteral("li"), QStringLiteral("xi"), QStringLiteral("mc"),
        QStringLiteral("dc"), QStringLiteral("cd"), QStringLiteral("dim"),
        QStringLiteral("mid"), QStringLiteral("did"), QStringLiteral("lid")
    };
    QStringList words = work.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (QString &w : words) {
        if (w.isEmpty()) continue;
        if (romanRe.match(w).hasMatch() && !romanFalsePositives.contains(w.toLower())) {
            w = w.toUpper();          // II, VII, IV, X …
        } else {
            w = w.toLower();
            w[0] = w[0].toUpper();
        }
    }
    result.cleanTitle = words.join(QLatin1Char(' '));

    return result;
}

ContentType NameParser::classifyByFiles(const QStringList &fileNames)
{
    // Extensions that only a game/app payload carries, vs. video containers.
    static const QSet<QString> gameExt = {
        QStringLiteral("exe"), QStringLiteral("bin"), QStringLiteral("dll"),
        QStringLiteral("iso"), QStringLiteral("msi"), QStringLiteral("cab"),
        QStringLiteral("pak"), QStringLiteral("dat"), QStringLiteral("asar"),
        QStringLiteral("uasset"), QStringLiteral("vpk"), QStringLiteral("nsp"),
        QStringLiteral("xci"), QStringLiteral("pkg")
    };
    static const QSet<QString> videoExt = {
        QStringLiteral("mkv"), QStringLiteral("mp4"), QStringLiteral("avi"),
        QStringLiteral("mov"), QStringLiteral("wmv"), QStringLiteral("m4v"),
        QStringLiteral("ts"), QStringLiteral("flv"), QStringLiteral("webm"),
        QStringLiteral("m2ts")
    };
    static const QRegularExpression epRe(
        QStringLiteral("s\\d{1,2}e\\d{1,3}|\\b\\d{1,2}x\\d{2,3}\\b"),
        QRegularExpression::CaseInsensitiveOption);

    int game = 0, video = 0, episodes = 0;
    for (const QString &f : fileNames) {
        const QString ext = f.section(QLatin1Char('.'), -1).toLower();
        if (gameExt.contains(ext)) {
            ++game;
        } else if (videoExt.contains(ext)) {
            ++video;
            if (epRe.match(f.section(QLatin1Char('/'), -1)).hasMatch()) ++episodes;
        }
    }
    if (game > video) return ContentType::Game;   // a few intro videos don't outweigh a game payload
    if (video > 0)    return episodes >= 2 ? ContentType::Series : ContentType::Movie;
    return ContentType::Unknown;
}
