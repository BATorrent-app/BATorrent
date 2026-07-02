// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/integrations/geoipprovider.h"
#include "services/platform/logger.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QDate>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include <zlib.h>

namespace {
constexpr qint64 kMaxUncompressed = 128 * 1024 * 1024; // hard cap vs a hostile body
constexpr qint64 kMaxDownload = 32 * 1024 * 1024;      // gz is ~4 MB; cap the reply
constexpr int kMaxMonthsBack = 2;                      // this month → two months back
}

GeoIpProvider::GeoIpProvider(QObject *parent)
    : QObject(parent), m_db(std::make_shared<GeoIpDb>())
{
}

QByteArray GeoIpProvider::gunzip(const QByteArray &gz)
{
    if (gz.isEmpty()) return {};
    z_stream s{};
    // 16 + MAX_WBITS tells zlib to expect a gzip header (not raw/zlib deflate).
    if (inflateInit2(&s, 16 + MAX_WBITS) != Z_OK) return {};
    s.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(gz.constData()));
    s.avail_in = static_cast<uInt>(gz.size());

    QByteArray out;
    out.resize(qMin<qint64>(kMaxUncompressed, qMax(gz.size() * 8, 1 << 20)));
    int ret;
    do {
        if (s.total_out >= static_cast<uLong>(out.size())) {
            if (static_cast<qint64>(out.size()) * 2 > kMaxUncompressed) {
                inflateEnd(&s);
                return {};
            }
            out.resize(out.size() * 2);
        }
        s.next_out = reinterpret_cast<Bytef *>(out.data() + s.total_out);
        s.avail_out = static_cast<uInt>(out.size() - s.total_out);
        ret = inflate(&s, Z_NO_FLUSH);
    } while (ret == Z_OK);
    const uLong total = s.total_out;
    inflateEnd(&s);
    if (ret != Z_STREAM_END) return {};
    out.resize(static_cast<int>(total));
    return out;
}

void GeoIpProvider::start(const QString &cacheDir)
{
    m_cacheDir = cacheDir;
    QDir().mkpath(cacheDir);
    const QString cache = QDir(cacheDir).filePath(QStringLiteral("dbip-country-lite.csv.gz"));

    // The DB is populated exactly once — before the peer-ranking classifier is
    // installed — and then never mutated, so libtorrent's compare_peer thread
    // can read it lock-free. That means: if we have any usable cache, use it and
    // do NOT also download (a second loadCsv would race the reader). Monthly geo
    // drift is immaterial for locality biasing, so stale-but-present is fine.
    const QFileInfo fi(cache);
    if (fi.exists() && fi.size() > 0) {
        loadFromGzFile(cache);
        if (ready()) return;
    }
    downloadMonth(0);
}

void GeoIpProvider::loadFromGzFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return;
    if (f.size() > kMaxDownload) return;   // corrupt/oversized cache — ignore
    loadFromGzBytes(f.readAll());
}

bool GeoIpProvider::loadFromGzBytes(const QByteArray &gz)
{
    const QByteArray csv = gunzip(gz);
    if (csv.isEmpty()) return false;
    const int n = m_db->loadCsv(QString::fromLatin1(csv));
    if (n <= 0) return false;
    Logger::instance().log(Logger::Info,
        QStringLiteral("GeoIP: loaded %1 IPv4 ranges").arg(n));
    emit loaded();
    return true;
}

void GeoIpProvider::downloadMonth(int monthsBack)
{
    if (monthsBack > kMaxMonthsBack) {
        Logger::instance().log(Logger::Warning,
            QStringLiteral("GeoIP: no db-ip Lite dataset found for recent months"));
        return;
    }
    const QDate d = QDate::currentDate().addMonths(-monthsBack);
    const QString url = QStringLiteral(
        "https://download.db-ip.com/free/dbip-country-lite-%1-%2.csv.gz")
        .arg(d.year(), 4, 10, QChar('0'))
        .arg(d.month(), 2, 10, QChar('0'));

    QNetworkRequest req{QUrl(url)};
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setTransferTimeout(300000);   // whole-transfer cap: the db is ~MBs, allow slow links
    QNetworkReply *reply = m_nam.get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply, monthsBack]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            // 404 for a not-yet-published month is expected → walk back.
            downloadMonth(monthsBack + 1);
            return;
        }
        const QByteArray gz = reply->readAll();
        if (gz.isEmpty() || gz.size() > kMaxDownload) {
            downloadMonth(monthsBack + 1);
            return;
        }
        if (!loadFromGzBytes(gz)) {
            downloadMonth(monthsBack + 1);
            return;
        }
        // Cache atomically so a half-written file never poisons the next launch.
        QSaveFile out(QDir(m_cacheDir).filePath(QStringLiteral("dbip-country-lite.csv.gz")));
        if (out.open(QIODevice::WriteOnly)) {
            out.write(gz);
            out.commit();
        }
    });
}
