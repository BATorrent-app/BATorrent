// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "nameparser.h"

#include <QRegularExpression>

ParsedName NameParser::parse(const QString &rawName)
{
    ParsedName result;
    QString work = rawName;

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
        work.remove(m.capturedStart(), m.capturedLength());
    } else {
        m = altSeRe.match(work);
        if (m.hasMatch()) {
            result.season = m.captured(1).toInt();
            result.episode = m.captured(2).toInt();
            result.contentType = ContentType::Series;
            work.remove(m.capturedStart(), m.capturedLength());
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
            result.contentType = ContentType::Game;
            result.repackerTag = repacker;
            work.remove(m.capturedStart(1), m.capturedLength(1));
            break;
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
        QStringLiteral("REMUX"), QStringLiteral("PROPER"), QStringLiteral("REPACK"),
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
            work.remove(m.capturedStart(1), m.capturedLength(1));
        }
    }

    if (mediaTagFound && result.contentType == ContentType::Unknown)
        result.contentType = ContentType::Movie;

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
    QStringList words = work.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (QString &w : words) {
        if (w.isEmpty()) continue;
        if (romanRe.match(w).hasMatch()) {
            w = w.toUpper();          // II, VII, IV, X …
        } else {
            w = w.toLower();
            w[0] = w[0].toUpper();
        }
    }
    result.cleanTitle = words.join(QLatin1Char(' '));

    return result;
}
