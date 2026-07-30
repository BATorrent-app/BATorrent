// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/qmlsearchbridge_util.h"

#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/error_code.hpp>
#include <QRegularExpression>
#include <sstream>

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
    // hex info-hash SessionManager indexes — otherwise installWhenReady /
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
             hasUri };
}

} // namespace SearchBridgeUtil
