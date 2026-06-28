// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/integrations/realdebrid.h"

#include "services/security/secretstore.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>

namespace {
constexpr auto kBase = "https://api.real-debrid.com/rest/1.0/";
constexpr auto kSecretKey = "realDebridToken";
constexpr int kPollMs = 2500;
constexpr int kMaxPolls = 400;   // ~16 min ceiling for an uncached download
}

RealDebridClient::RealDebridClient(QObject *parent)
    : QObject(parent)
{
    m_pollTimer.setSingleShot(true);
    m_pollTimer.setInterval(kPollMs);
    connect(&m_pollTimer, &QTimer::timeout, this, &RealDebridClient::pollJob);
    loadToken();
}

void RealDebridClient::loadToken()
{
    m_token = SecretStore::instance().get(kSecretKey);
    if (!m_token.isEmpty())
        checkAccount();
}

QNetworkRequest RealDebridClient::authedRequest(const QString &path) const
{
    QNetworkRequest req(QUrl(QString::fromLatin1(kBase) + path));
    req.setRawHeader("Authorization", "Bearer " + m_token.toUtf8());
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    return req;
}

QNetworkReply *RealDebridClient::get(const QString &path)
{
    return m_nam.get(authedRequest(path));
}

QNetworkReply *RealDebridClient::post(const QString &path, const QByteArray &form)
{
    QNetworkRequest req = authedRequest(path);
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  "application/x-www-form-urlencoded");
    return m_nam.post(req, form);
}

// --- Account ---------------------------------------------------------------

void RealDebridClient::setToken(const QString &token)
{
    m_token = token.trimmed();
    SecretStore::instance().set(kSecretKey, m_token);
    m_authed = false;
    m_username.clear(); m_type.clear(); m_expiry.clear();
    emit accountChanged();
    if (!m_token.isEmpty())
        checkAccount();
}

void RealDebridClient::clearToken()
{
    setToken(QString());
}

void RealDebridClient::checkAccount()
{
    if (m_token.isEmpty()) return;
    QNetworkReply *r = get(QStringLiteral("user"));
    connect(r, &QNetworkReply::finished, this, [this, r] {
        r->deleteLater();
        const QJsonObject o = QJsonDocument::fromJson(r->readAll()).object();
        if (r->error() != QNetworkReply::NoError || o.isEmpty()) {
            m_authed = false;
            m_username.clear(); m_type.clear(); m_expiry.clear();
            emit accountChanged();
            if (r->error() == QNetworkReply::AuthenticationRequiredError
                || r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 401)
                emit errorOccurred(tr("Real-Debrid rejected the token — check it and paste again."));
            return;
        }
        m_username = o.value(QStringLiteral("username")).toString();
        m_type = o.value(QStringLiteral("type")).toString();        // "premium" | "free"
        m_expiry = o.value(QStringLiteral("expiration")).toString().left(10);
        m_authed = (m_type == QLatin1String("premium"));
        emit accountChanged();
    });
}

// --- Stream pipeline -------------------------------------------------------

void RealDebridClient::streamMagnet(const QString &magnet)
{
    if (m_token.isEmpty()) {
        emit errorOccurred(tr("Add your Real-Debrid token in Settings first."));
        return;
    }
    if (m_busy) {
        emit errorOccurred(tr("A Real-Debrid transfer is already in progress."));
        return;
    }
    m_jobId.clear();
    m_jobName.clear();
    m_filesSelected = false;
    m_pollTries = 0;
    setBusy(true);
    setStatus(tr("Sending to Real-Debrid…"), 0);

    QUrlQuery q;
    q.addQueryItem(QStringLiteral("magnet"), magnet);
    QNetworkReply *r = post(QStringLiteral("torrents/addMagnet"),
                            q.toString(QUrl::FullyEncoded).toUtf8());
    connect(r, &QNetworkReply::finished, this, [this, r] { onAddMagnet(r); });
}

void RealDebridClient::onAddMagnet(QNetworkReply *r)
{
    r->deleteLater();
    const QJsonObject o = QJsonDocument::fromJson(r->readAll()).object();
    const QString id = o.value(QStringLiteral("id")).toString();
    if (r->error() != QNetworkReply::NoError || id.isEmpty()) {
        failJob(tr("Real-Debrid couldn't accept this magnet."));
        return;
    }
    m_jobId = id;
    setStatus(tr("Preparing on Real-Debrid…"), 0);
    m_pollTimer.start(0);   // first poll immediately
}

void RealDebridClient::pollJob()
{
    if (m_jobId.isEmpty()) return;
    if (++m_pollTries > kMaxPolls) {
        failJob(tr("Real-Debrid is taking too long — try again later."));
        return;
    }
    QNetworkReply *r = get(QStringLiteral("torrents/info/") + m_jobId);
    connect(r, &QNetworkReply::finished, this, [this, r] { onJobInfo(r); });
}

void RealDebridClient::onJobInfo(QNetworkReply *r)
{
    r->deleteLater();
    if (m_jobId.isEmpty()) return;   // cancelled mid-flight
    const QJsonObject o = QJsonDocument::fromJson(r->readAll()).object();
    if (r->error() != QNetworkReply::NoError || o.isEmpty()) {
        m_pollTimer.start();   // transient — retry
        return;
    }

    const QString status = o.value(QStringLiteral("status")).toString();
    m_jobName = o.value(QStringLiteral("filename")).toString();

    if (status == QLatin1String("waiting_files_selection")) {
        if (!m_filesSelected) {
            m_filesSelected = true;
            setStatus(tr("Selecting files…"), 0);
            selectFiles(m_jobId, QStringLiteral("all"));
        } else {
            m_pollTimer.start();
        }
        return;
    }
    if (status == QLatin1String("downloaded")) {
        const QJsonArray links = o.value(QStringLiteral("links")).toArray();
        const QJsonArray files = o.value(QStringLiteral("files")).toArray();
        const int idx = pickBestLinkIndex(files);
        if (idx < 0 || idx >= links.size()) {
            failJob(tr("No playable file in this Real-Debrid transfer."));
            return;
        }
        setStatus(tr("Unlocking stream…"), 100);
        unrestrict(links.at(idx).toString());
        return;
    }
    if (status == QLatin1String("downloading")) {
        setStatus(tr("Real-Debrid is caching…"),
                  qBound(0, int(o.value(QStringLiteral("progress")).toDouble()), 100));
        m_pollTimer.start();
        return;
    }
    if (status == QLatin1String("magnet_conversion")
        || status == QLatin1String("queued")
        || status == QLatin1String("compressing")
        || status == QLatin1String("uploading")) {
        setStatus(tr("Preparing on Real-Debrid…"), m_progress);
        m_pollTimer.start();
        return;
    }
    // magnet_error | error | virus | dead
    failJob(tr("Real-Debrid failed on this torrent (%1).").arg(status));
}

void RealDebridClient::selectFiles(const QString &id, const QString &fileIds)
{
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("files"), fileIds);
    QNetworkReply *r = post(QStringLiteral("torrents/selectFiles/") + id,
                            q.toString(QUrl::FullyEncoded).toUtf8());
    connect(r, &QNetworkReply::finished, this, [this, r] {
        r->deleteLater();
        // 204 / 202 both fine; resume polling regardless.
        if (!m_jobId.isEmpty()) m_pollTimer.start();
    });
}

void RealDebridClient::unrestrict(const QString &link)
{
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("link"), link);
    QNetworkReply *r = post(QStringLiteral("unrestrict/link"),
                            q.toString(QUrl::FullyEncoded).toUtf8());
    connect(r, &QNetworkReply::finished, this, [this, r] { onUnrestrict(r); });
}

void RealDebridClient::onUnrestrict(QNetworkReply *r)
{
    r->deleteLater();
    const QJsonObject o = QJsonDocument::fromJson(r->readAll()).object();
    const QString url = o.value(QStringLiteral("download")).toString();
    if (r->error() != QNetworkReply::NoError || url.isEmpty()) {
        failJob(tr("Real-Debrid couldn't unlock the stream link."));
        return;
    }
    const QString name = o.value(QStringLiteral("filename")).toString(m_jobName);
    m_jobId.clear();
    setStatus(QString(), 100);
    setBusy(false);
    emit streamReady(url, name);
}

void RealDebridClient::cancelStream()
{
    m_pollTimer.stop();
    m_jobId.clear();
    setStatus(QString(), 0);
    setBusy(false);
}

void RealDebridClient::failJob(const QString &msg)
{
    m_pollTimer.stop();
    m_jobId.clear();
    setStatus(QString(), 0);
    setBusy(false);
    emit errorOccurred(msg);
}

// --- Cloud library ---------------------------------------------------------

void RealDebridClient::refreshTorrents()
{
    if (m_token.isEmpty()) return;
    QNetworkReply *r = get(QStringLiteral("torrents?limit=100"));
    connect(r, &QNetworkReply::finished, this, [this, r] {
        r->deleteLater();
        if (r->error() != QNetworkReply::NoError) return;
        const QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
        QVariantList out;
        for (const QJsonValue &v : arr) {
            const QJsonObject o = v.toObject();
            out << QVariantMap{
                {QStringLiteral("id"), o.value(QStringLiteral("id")).toString()},
                {QStringLiteral("name"), o.value(QStringLiteral("filename")).toString()},
                {QStringLiteral("hash"), o.value(QStringLiteral("hash")).toString()},
                {QStringLiteral("bytes"), o.value(QStringLiteral("bytes")).toDouble()},
                {QStringLiteral("status"), o.value(QStringLiteral("status")).toString()},
                {QStringLiteral("progress"), o.value(QStringLiteral("progress")).toDouble()},
            };
        }
        m_torrents = out;
        emit torrentsChanged();
    });
}

void RealDebridClient::deleteRemote(const QString &id)
{
    if (m_token.isEmpty() || id.isEmpty()) return;
    QNetworkReply *r = m_nam.deleteResource(
        authedRequest(QStringLiteral("torrents/delete/") + id));
    connect(r, &QNetworkReply::finished, this, [this, r] {
        r->deleteLater();
        refreshTorrents();
    });
}

// --- small helpers ---------------------------------------------------------

void RealDebridClient::setBusy(bool b)
{
    if (m_busy == b) return;
    m_busy = b;
    emit busyChanged();
}

void RealDebridClient::setStatus(const QString &s, int progress)
{
    m_status = s;
    if (progress >= 0) m_progress = progress;
    emit statusChanged();
}

bool RealDebridClient::looksLikeVideo(const QString &path)
{
    static const QStringList exts = {
        QStringLiteral(".mp4"), QStringLiteral(".mkv"), QStringLiteral(".avi"),
        QStringLiteral(".mov"), QStringLiteral(".wmv"), QStringLiteral(".m4v"),
        QStringLiteral(".webm"), QStringLiteral(".flv"), QStringLiteral(".mpg"),
        QStringLiteral(".mpeg"), QStringLiteral(".ts"), QStringLiteral(".m2ts"),
    };
    const QString p = path.toLower();
    for (const QString &e : exts)
        if (p.endsWith(e)) return true;
    return false;
}

// RD's `links` array has one entry per *selected* file, in file order. Walk the
// files, counting only selected ones to map into the links index, and return
// the link position of the largest selected video (else the largest selected
// file of any kind), or -1 when nothing is selected.
int RealDebridClient::pickBestLinkIndex(const QJsonArray &files)
{
    int selIdx = -1;
    int bestVideo = -1; qint64 bestVideoBytes = -1;
    int bestAny = -1;   qint64 bestAnyBytes = -1;
    for (const QJsonValue &v : files) {
        const QJsonObject f = v.toObject();
        if (f.value(QStringLiteral("selected")).toInt() != 1) continue;
        ++selIdx;
        const qint64 bytes = qint64(f.value(QStringLiteral("bytes")).toDouble());
        const QString path = f.value(QStringLiteral("path")).toString();
        if (bytes > bestAnyBytes) { bestAnyBytes = bytes; bestAny = selIdx; }
        if (looksLikeVideo(path) && bytes > bestVideoBytes) {
            bestVideoBytes = bytes; bestVideo = selIdx;
        }
    }
    return bestVideo >= 0 ? bestVideo : bestAny;
}
