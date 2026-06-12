// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "subtitlesearch.h"
#include "nameparser.h"

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

namespace {

constexpr int kTimeoutMs = 12000;

// OpenSubtitles wants lowercase ISO codes; Gestdown (Addic7ed) wants names.
QString osLang(const QString &code)
{
    if (code == QLatin1String("pt")) return QStringLiteral("pt-br");
    if (code == QLatin1String("zh")) return QStringLiteral("zh-cn");
    return code;
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

QString SubtitleSearch::openSubtitlesKey()
{
    QString key = QSettings("BATorrent", "BATorrent").value("osApiKey").toString().trimmed();
#ifdef BAT_OS_KEY
    if (key.isEmpty()) key = QStringLiteral(BAT_OS_KEY);
#endif
    return key;
}

void SubtitleSearch::search(const QString &videoName, const QStringList &languages)
{
    m_results.clear();
    emit resultsChanged();

    const ParsedName pn = NameParser::parse(videoName);
    const QString title = pn.cleanTitle.isEmpty() ? videoName : pn.cleanTitle;

    m_pending = 0;
    if (!openSubtitlesKey().isEmpty()) {
        ++m_pending;
        searchOpenSubtitles(title, pn.season, pn.episode, languages);
    }
    if (pn.season >= 0 && pn.episode >= 0) {
        ++m_pending;
        searchGestdown(title, pn.season, pn.episode, languages);
    }
    if (m_pending == 0) {
        // movie + no OpenSubtitles key: nothing can answer
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

        auto *remaining = new int(langs.size());
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
                if (--(*remaining) <= 0) { delete remaining; providerDone(); }
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
    if (r.provider == QLatin1String("Gestdown"))
        downloadGestdown(r, savePath);
    else
        downloadOpenSubtitles(r, savePath);
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
