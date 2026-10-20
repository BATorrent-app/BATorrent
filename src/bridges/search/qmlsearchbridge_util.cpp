// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/search/qmlsearchbridge_util.h"
#include "services/metadata/releasegroup.h"

#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/error_code.hpp>
#include <QRegularExpression>
#include <sstream>
#include <algorithm>

namespace SearchBridgeUtil {

bool sameTitle(const QString &a, const QString &b)
{
    auto fold = [](const QString &s) {
        const QString d = s.normalized(QString::NormalizationForm_D).toLower();
        QString out;
        for (const QChar &c : d)
            if (c.isLetterOrNumber() && c.category() != QChar::Mark_NonSpacing)
                out.append(c);
        return out;
    };
    return fold(a) == fold(b);
}

QString btihFromMagnet(const QString &magnet)
{
    // Prefer libtorrent's parser so Base32 xt=urn:btih: tokens become the same
    // hex info-hash SessionManager indexes: otherwise installWhenReady /
    // watchWhenReady poll a hash that never matches and time out.
    lt::error_code ec;
    const lt::add_torrent_params atp = lt::parse_magnet_uri(magnet.toStdString(), ec);
    if (!ec) {
        const QString hex = QString::fromStdString(
            (std::ostringstream() << atp.info_hashes.get_best()).str());
        if (!hex.isEmpty()) return hex;
    }
    static const QRegularExpression re(QStringLiteral("xt=urn:btih:([A-Za-z0-9]+)"),
                                       QRegularExpression::CaseInsensitiveOption);
    const auto m = re.match(magnet);
    return m.hasMatch() ? m.captured(1) : QString();
}

QString resultDedupeKey(const QString &magnet, const QString &name, qlonglong size)
{
    const QString h = btihFromMagnet(magnet).toLower();
    if (!h.isEmpty()) return h;
    return name.toLower() + QLatin1Char('|') + QString::number(size);
}

GameReleasePick::Candidate gameCandFromRow(const QVariantMap &m, bool hasUri)
{
    return { m.value(QStringLiteral("fromCatalog")).toBool(),
             m.value(QStringLiteral("version")).toString(),
             m.value(QStringLiteral("uploadDate")).toString(),
             m.value(QStringLiteral("seedsN")).toInt(),
             hasUri,
             ReleaseGroup::isKnown(m.value(QStringLiteral("releaseGroup")).toString()) };
}

QVariantMap titleRowFromWork(const QVariantMap &w)
{
    QVariantMap row;
    row["name"]    = w.value(QStringLiteral("title"));
    row["title"]   = w.value(QStringLiteral("title"));
    row["originalTitle"] = w.value(QStringLiteral("originalTitle"));
    row["sub"]     = w.value(QStringLiteral("type"));
    row["sizeStr"] = w.value(QStringLiteral("year"));
    row["year"]    = w.value(QStringLiteral("year"));
    row["type"]    = w.value(QStringLiteral("type"));
    row["poster"]  = w.value(QStringLiteral("poster"));
    row["backdrop"] = w.value(QStringLiteral("backdrop"));
    row["rating"]  = w.value(QStringLiteral("rating"));
    row["overview"] = w.value(QStringLiteral("overview"));
    row["tmdbId"]  = w.value(QStringLiteral("tmdbId"));
    row["stills"]  = w.value(QStringLiteral("stills"));
    row["genres"]  = w.value(QStringLiteral("genres"));
    row["maker"]   = w.value(QStringLiteral("maker"));
    row["coverHash"] = QString();
    row["isTitle"] = true;
    return row;
}

QVariantMap filmAfter(const QVariantList &films, int tmdbId)
{
    for (int i = 0; i + 1 < films.size(); ++i)
        if (films.at(i).toMap().value(QStringLiteral("tmdbId")).toInt() == tmdbId)
            return films.at(i + 1).toMap();
    return {};
}

QString trackerQuery(const QString &title, const QString &year, const QString &type)
{
    // An apostrophe joins its word ("Philosopher's" is "Philosophers" in a
    // release name), other punctuation splits; a hyphen stays ("Spider-Man").
    QString q;
    for (const QChar &c : title) {
        if (c == QLatin1Char('\'') || c == QChar(0x2019) || c == QChar(0x2018)) continue;
        q.append(c.isLetterOrNumber() || c == QLatin1Char('-') || c.isSpace() ? c : QLatin1Char(' '));
    }
    q = q.simplified();
    // movies disambiguate well with a year; games/series search cleaner by title
    if (type == QLatin1String("movie") && !year.isEmpty()) q += QLatin1Char(' ') + year;
    return q;
}

QStringList huntTitles(const QStringList &searched, const QStringList &candidates)
{
    constexpr int kMaxTitles = 3;
    QStringList picked;
    for (const QString &c : candidates) {
        if (searched.size() + picked.size() >= kMaxTitles) break;
        const QString t = c.trimmed();
        if (t.isEmpty()) continue;
        auto same = [&t](const QString &s) { return sameTitle(s, t); };
        if (std::any_of(searched.begin(), searched.end(), same)
            || std::any_of(picked.begin(), picked.end(), same)) continue;
        picked << t;
    }
    return picked;
}

} // namespace SearchBridgeUtil
