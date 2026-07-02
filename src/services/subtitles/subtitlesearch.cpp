// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/subtitles/subtitlesearch.h"
#include "services/metadata/nameparser.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrl>
#include <QUrlQuery>
#include <memory>

#include <zlib.h>

namespace {

constexpr int kTimeoutMs = 12000;

// Raw DEFLATE inflate (no zlib header — that's how ZIP stores method-8 data).
QByteArray inflateRaw(const char *src, int n, int hint)
{
    z_stream s{};
    if (inflateInit2(&s, -MAX_WBITS) != Z_OK) return {};
    s.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(src));
    s.avail_in = static_cast<uInt>(n);
    QByteArray out;
    // a garbage central-directory size mustn't drive a huge allocation
    const int start = (hint > 0 && hint < 64 * 1024 * 1024) ? hint : qMax(n * 4, 16384);
    out.resize(start);
    int ret;
    do {
        if (s.total_out >= static_cast<uLong>(out.size())) out.resize(out.size() * 2);
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

// Pull the first .srt out of a (small, non-zip64) ZIP via its central directory.
QByteArray unzipFirstSrt(const QByteArray &zip)
{
    const char *d = zip.constData();
    const int n = zip.size();
    if (n < 22) return {};
    auto u16 = [&](int o) { return quint16(quint8(d[o]) | (quint8(d[o + 1]) << 8)); };
    auto u32 = [&](int o) {
        return quint32(quint8(d[o]) | (quint8(d[o + 1]) << 8)
                       | (quint8(d[o + 2]) << 16) | (quint32(quint8(d[o + 3])) << 24));
    };
    int eocd = -1;
    for (int i = n - 22; i >= 0 && i >= n - 22 - 65536; --i)
        if (u32(i) == 0x06054b50) { eocd = i; break; }
    if (eocd < 0) return {};
    const int count = int(u16(eocd + 10));
    // 64-bit offsets so a corrupt header (e.g. an HTML error body served as a
    // .zip) can't overflow to a negative index and read out of bounds.
    qint64 p = u32(eocd + 16);
    for (int e = 0; e < count; ++e) {
        if (p < 0 || p + 46 > n || u32(int(p)) != 0x02014b50) break;
        const quint16 method = u16(p + 10);
        const quint32 compSize = u32(p + 20);
        const quint32 uncompSize = u32(p + 24);
        const int fnLen = int(u16(p + 28));
        const int efLen = int(u16(p + 30));
        const int cmLen = int(u16(p + 32));
        const qint64 lho = u32(p + 42);
        if (p + 46 + fnLen > n) break;
        const QString name = QString::fromUtf8(d + int(p) + 46, fnLen);
        p += 46 + fnLen + efLen + cmLen;
        if (!name.endsWith(QLatin1String(".srt"), Qt::CaseInsensitive)) continue;
        if (lho < 0 || lho + 30 > n) return {};
        const qint64 dataOff = lho + 30 + u16(int(lho) + 26) + u16(int(lho) + 28);
        if (dataOff < 0 || dataOff + qint64(compSize) > n) return {};
        if (method == 0) return QByteArray(d + dataOff, int(compSize));   // stored
        if (method == 8) return inflateRaw(d + int(dataOff), int(compSize), int(uncompSize));
        return {};
    }
    return {};
}

// OpenSubtitles wants lowercase ISO codes; Gestdown (Addic7ed) wants names.
QString osLang(const QString &code)
{
    if (code == QLatin1String("pt")) return QStringLiteral("pt-br");
    if (code == QLatin1String("zh")) return QStringLiteral("zh-cn");
    return code;
}
// SubDL wants uppercase codes; Brazilian Portuguese is PT-BR.
QString subdlLang(const QString &code)
{
    if (code == QLatin1String("pt")) return QStringLiteral("PT-BR");
    return code.toUpper();
}
QString gestdownLang(const QString &code)
{
    if (code == QLatin1String("pt")) return QStringLiteral("Portuguese (Brazilian)");
    if (code == QLatin1String("en")) return QStringLiteral("English");
    if (code == QLatin1String("es")) return QStringLiteral("Spanish");
    if (code == QLatin1String("de")) return QStringLiteral("German");
    if (code == QLatin1String("ru")) return QStringLiteral("Russian");
    if (code == QLatin1String("uk")) return QStringLiteral("Ukrainian");
    if (code == QLatin1String("ja")) return QStringLiteral("Japanese");
    if (code == QLatin1String("zh")) return QStringLiteral("Chinese (Simplified)");
    return QStringLiteral("English");
}

void prepare(QNetworkRequest &req)
{
    req.setTransferTimeout(kTimeoutMs);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QStringLiteral("BATorrent v%1").arg(QCoreApplication::applicationVersion()));
}

} // namespace

SubtitleSearch::SubtitleSearch(QObject *parent)
    : QObject(parent)
{
}

QString SubtitleSearch::subdlKey()
{
    QString key = QSettings("BATorrent", "BATorrent").value("subdlApiKey").toString().trimmed();
#ifdef BAT_SUBDL_KEY
    if (key.isEmpty()) key = QStringLiteral(BAT_SUBDL_KEY);
#endif
    return key;
}

QString SubtitleSearch::openSubtitlesKey()
{
    QString key = QSettings("BATorrent", "BATorrent").value("osApiKey").toString().trimmed();
#ifdef BAT_OS_KEY
    if (key.isEmpty()) key = QStringLiteral(BAT_OS_KEY);
#endif
    return key;
}

void SubtitleSearch::search(const QString &videoName, const QStringList &languages, int tmdbId)
{
    m_results.clear();
    emit resultsChanged();

    const ParsedName pn = NameParser::parse(videoName);
    const QString title = pn.cleanTitle.isEmpty() ? videoName : pn.cleanTitle;

    m_pending = 0;
    if (!subdlKey().isEmpty()) {
        ++m_pending;
        searchSubDL(title, tmdbId, pn.season, pn.episode, languages);
    }
    if (!openSubtitlesKey().isEmpty()) {
        ++m_pending;
        searchOpenSubtitles(title, pn.season, pn.episode, languages);
    }
    if (pn.season >= 0 && pn.episode >= 0) {
        ++m_pending;
        searchGestdown(title, pn.season, pn.episode, languages);
    }
    if (m_pending == 0) {
        // movie + no keyed provider: nothing can answer
        emit errorOccurred(QStringLiteral("no_sources"));
        emit searchFinished();
    }
}

void SubtitleSearch::providerDone()
{
    if (--m_pending <= 0) {
        m_pending = 0;
        emit searchFinished();
    }
}

void SubtitleSearch::searchSubDL(const QString &title, int tmdbId, int season, int episode,
                                 const QStringList &langs)
{
    QStringList codes;
    for (const QString &l : langs) codes << subdlLang(l);

    QUrl url(QStringLiteral("https://api.subdl.com/api/v1/subtitles"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("api_key"), subdlKey());
    // tmdb_id pins the exact title — "Michael" otherwise matches a dozen films.
    if (tmdbId > 0) q.addQueryItem(QStringLiteral("tmdb_id"), QString::number(tmdbId));
    else q.addQueryItem(QStringLiteral("film_name"), title);
    q.addQueryItem(QStringLiteral("languages"), codes.join(QLatin1Char(',')));
    q.addQueryItem(QStringLiteral("type"), (season >= 0 && episode >= 0)
                       ? QStringLiteral("tv") : QStringLiteral("movie"));
    if (season >= 0) q.addQueryItem(QStringLiteral("season_number"), QString::number(season));
    if (episode >= 0) q.addQueryItem(QStringLiteral("episode_number"), QString::number(episode));
    q.addQueryItem(QStringLiteral("subs_per_page"), QStringLiteral("30"));
    q.addQueryItem(QStringLiteral("unpack"), QStringLiteral("1"));
    url.setQuery(q);

    QNetworkRequest req(url);
    prepare(req);
    QNetworkReply *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            const auto doc = QJsonDocument::fromJson(reply->readAll());
            const QJsonArray subs = doc.object().value(QStringLiteral("subtitles")).toArray();
            for (const auto &sv : subs) {
                const QJsonObject so = sv.toObject();
                // unpack=1 *sometimes* splits a pack into per-file entries whose
                // url serves the raw .srt directly. When it doesn't (common), the
                // only handle is the parent .zip, which downloadSubDL unzips.
                const QJsonArray files = so.value(QStringLiteral("unpack_files")).toArray();
                bool added = false;
                for (const auto &fv : files) {
                    const QJsonObject o = fv.toObject();
                    if (o.value(QStringLiteral("format")).toString().compare(
                            QLatin1String("srt"), Qt::CaseInsensitive) != 0)
                        continue;
                    const QString ref = o.value(QStringLiteral("url")).toString();
                    if (ref.isEmpty()) continue;
                    SubtitleResult r;
                    r.provider = QStringLiteral("SubDL");
                    r.language = o.value(QStringLiteral("language")).toString().toLower();
                    r.name = o.value(QStringLiteral("release_name")).toString();
                    if (r.name.isEmpty()) r.name = o.value(QStringLiteral("name")).toString();
                    if (o.value(QStringLiteral("hi")).toBool(false)) r.name += QStringLiteral(" [HI]");
                    r.downloadRef = ref;
                    m_results << r;
                    added = true;
                }
                if (added) continue;
                const QString zipRef = so.value(QStringLiteral("url")).toString();
                if (zipRef.isEmpty()) continue;
                SubtitleResult r;
                r.provider = QStringLiteral("SubDL");
                r.language = so.value(QStringLiteral("language")).toString().toLower();
                r.name = so.value(QStringLiteral("release_name")).toString();
                if (r.name.isEmpty()) r.name = so.value(QStringLiteral("name")).toString();
                if (so.value(QStringLiteral("hi")).toBool(false)) r.name += QStringLiteral(" [HI]");
                r.downloadRef = zipRef;
                m_results << r;
            }
            emit resultsChanged();
        }
        providerDone();
    });
}

void SubtitleSearch::searchOpenSubtitles(const QString &title, int season, int episode,
                                         const QStringList &langs)
{
    QStringList codes;
    for (const QString &l : langs) codes << osLang(l);

    QUrl url(QStringLiteral("https://api.opensubtitles.com/api/v1/subtitles"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("query"), title);
    q.addQueryItem(QStringLiteral("languages"), codes.join(QLatin1Char(',')));
    if (season >= 0) q.addQueryItem(QStringLiteral("season_number"), QString::number(season));
    if (episode >= 0) q.addQueryItem(QStringLiteral("episode_number"), QString::number(episode));
    url.setQuery(q);

    QNetworkRequest req(url);
    prepare(req);
    req.setRawHeader("Api-Key", openSubtitlesKey().toUtf8());

    QNetworkReply *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            const auto doc = QJsonDocument::fromJson(reply->readAll());
            const QJsonArray data = doc.object().value(QStringLiteral("data")).toArray();
            for (const auto &v : data) {
                const QJsonObject attrs = v.toObject().value(QStringLiteral("attributes")).toObject();
                const QJsonArray files = attrs.value(QStringLiteral("files")).toArray();
                if (files.isEmpty()) continue;
                SubtitleResult r;
                r.provider = QStringLiteral("OpenSubtitles");
                r.language = attrs.value(QStringLiteral("language")).toString();
                r.name = attrs.value(QStringLiteral("release")).toString();
                if (r.name.isEmpty())
                    r.name = files.first().toObject().value(QStringLiteral("file_name")).toString();
                r.downloadRef = QString::number(
                    files.first().toObject().value(QStringLiteral("file_id")).toDouble(), 'f', 0);
                m_results << r;
            }
            emit resultsChanged();
        }
        providerDone();
    });
}

void SubtitleSearch::searchGestdown(const QString &title, int season, int episode,
                                    const QStringList &langs)
{
    // two-step: resolve the Addic7ed show id, then one get per language
    const QUrl searchUrl(QStringLiteral("https://api.gestdown.info/shows/search/")
                         + QString::fromUtf8(QUrl::toPercentEncoding(title)));
    QNetworkRequest req(searchUrl);
    prepare(req);
    QNetworkReply *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, season, episode, langs]() {
        reply->deleteLater();
        QString showId;
        if (reply->error() == QNetworkReply::NoError) {
            const auto doc = QJsonDocument::fromJson(reply->readAll());
            const QJsonArray shows = doc.object().value(QStringLiteral("shows")).toArray();
            if (!shows.isEmpty())
                showId = shows.first().toObject().value(QStringLiteral("id")).toString();
        }
        if (showId.isEmpty() || langs.isEmpty()) { providerDone(); return; }

        auto remaining = std::make_shared<int>(langs.size());
        for (const QString &l : langs) {
            const QUrl url(QStringLiteral("https://api.gestdown.info/subtitles/get/%1/%2/%3/%4")
                               .arg(showId)
                               .arg(season)
                               .arg(episode)
                               .arg(QString::fromUtf8(QUrl::toPercentEncoding(gestdownLang(l)))));
            QNetworkRequest sreq(url);
            prepare(sreq);
            QNetworkReply *sreply = m_nam.get(sreq);
            connect(sreply, &QNetworkReply::finished, this, [this, sreply, remaining, l]() {
                sreply->deleteLater();
                if (sreply->error() == QNetworkReply::NoError) {
                    const auto doc = QJsonDocument::fromJson(sreply->readAll());
                    const QJsonArray subs = doc.object().value(QStringLiteral("matchingSubtitles")).toArray();
                    for (const auto &v : subs) {
                        const QJsonObject o = v.toObject();
                        if (!o.value(QStringLiteral("completed")).toBool(true)) continue;
                        SubtitleResult r;
                        r.provider = QStringLiteral("Gestdown");
                        r.language = l;
                        r.name = o.value(QStringLiteral("version")).toString();
                        if (o.value(QStringLiteral("hearingImpaired")).toBool(false))
                            r.name += QStringLiteral(" [HI]");
                        r.downloadRef = o.value(QStringLiteral("downloadUri")).toString();
                        if (r.downloadRef.isEmpty()) continue;
                        m_results << r;
                    }
                    emit resultsChanged();
                }
                if (--(*remaining) <= 0) providerDone();
            });
        }
    });
}

void SubtitleSearch::download(int index, const QString &targetDir, const QString &videoBaseName)
{
    if (index < 0 || index >= m_results.size()) return;
    const SubtitleResult r = m_results[index];
    const QString savePath = QDir(targetDir).filePath(
        videoBaseName + QLatin1Char('.') + r.language + QStringLiteral(".srt"));
    if (r.provider == QLatin1String("SubDL"))
        downloadSubDL(r, savePath);
    else if (r.provider == QLatin1String("Gestdown"))
        downloadGestdown(r, savePath);
    else
        downloadOpenSubtitles(r, savePath);
}

void SubtitleSearch::downloadSubDL(const SubtitleResult &r, const QString &savePath)
{
    const QUrl url = r.downloadRef.startsWith(QLatin1String("http"))
                         ? QUrl(r.downloadRef)
                         : QUrl(QStringLiteral("https://dl.subdl.com") + r.downloadRef);
    const bool isZip = url.path().endsWith(QLatin1String(".zip"), Qt::CaseInsensitive);
    QNetworkRequest req(url);
    prepare(req);
    QNetworkReply *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, savePath, isZip]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            return;
        }
        QByteArray data = reply->readAll();
        if (isZip) {
            data = unzipFirstSrt(data);
            if (data.isEmpty()) { emit errorOccurred(QStringLiteral("unzip_failed")); return; }
        }
        QFile f(savePath);
        if (!f.open(QIODevice::WriteOnly)) { emit errorOccurred(savePath); return; }
        f.write(data);
        f.close();
        emit downloadFinished(savePath);
    });
}

void SubtitleSearch::downloadGestdown(const SubtitleResult &r, const QString &savePath)
{
    QUrl url = r.downloadRef.startsWith(QLatin1String("http"))
                   ? QUrl(r.downloadRef)
                   : QUrl(QStringLiteral("https://api.gestdown.info") + r.downloadRef);
    QNetworkRequest req(url);
    prepare(req);
    QNetworkReply *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, savePath]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            return;
        }
        QFile f(savePath);
        if (!f.open(QIODevice::WriteOnly)) { emit errorOccurred(savePath); return; }
        f.write(reply->readAll());
        f.close();
        emit downloadFinished(savePath);
    });
}

void SubtitleSearch::downloadOpenSubtitles(const SubtitleResult &r, const QString &savePath)
{
    QNetworkRequest req(QUrl(QStringLiteral("https://api.opensubtitles.com/api/v1/download")));
    prepare(req);
    req.setRawHeader("Api-Key", openSubtitlesKey().toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QJsonObject body;
    body[QStringLiteral("file_id")] = r.downloadRef.toLongLong();
    QNetworkReply *reply = m_nam.post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, savePath]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            return;
        }
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        const QString link = doc.object().value(QStringLiteral("link")).toString();
        if (link.isEmpty()) {
            // quota exhausted or API error — surface the message OpenSubtitles sent
            emit errorOccurred(doc.object().value(QStringLiteral("message")).toString());
            return;
        }
        QNetworkRequest fileReq{QUrl(link)};
        prepare(fileReq);
        QNetworkReply *fileReply = m_nam.get(fileReq);
        connect(fileReply, &QNetworkReply::finished, this, [this, fileReply, savePath]() {
            fileReply->deleteLater();
            if (fileReply->error() != QNetworkReply::NoError) {
                emit errorOccurred(fileReply->errorString());
                return;
            }
            QFile f(savePath);
            if (!f.open(QIODevice::WriteOnly)) { emit errorOccurred(savePath); return; }
            f.write(fileReply->readAll());
            f.close();
            emit downloadFinished(savePath);
        });
    });
}
