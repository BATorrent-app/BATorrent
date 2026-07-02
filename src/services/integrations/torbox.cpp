// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/integrations/torbox.h"

#include "services/security/secretstore.h"

#include <QHttpMultiPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>

namespace {
constexpr auto kBase = "https://api.torbox.app/v1/api/";
constexpr auto kSecretKey = "torBoxToken";
constexpr int kPollMs = 2500;
constexpr int kMaxPolls = 400;   // ~16 min ceiling for an uncached download
}

TorBoxClient::TorBoxClient(QObject *parent)
    : IDebridProvider(parent)
{
    m_nam.setTransferTimeout(30000);
    m_pollTimer.setSingleShot(true);
    m_pollTimer.setInterval(kPollMs);
    connect(&m_pollTimer, &QTimer::timeout, this, &TorBoxClient::pollJob);
    loadToken();
}

void TorBoxClient::loadToken()
{
    m_token = SecretStore::instance().get(kSecretKey);
    if (!m_token.isEmpty())
        checkAccount();
}

QNetworkRequest TorBoxClient::authedRequest(const QString &path) const
{
    QNetworkRequest req(QUrl(QString::fromLatin1(kBase) + path));
    req.setRawHeader("Authorization", "Bearer " + m_token.toUtf8());
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    return req;
}

QNetworkReply *TorBoxClient::get(const QString &path)
{
    return m_nam.get(authedRequest(path));
}

// --- Account ---------------------------------------------------------------

void TorBoxClient::setToken(const QString &token)
{
    m_token = token.trimmed();
    SecretStore::instance().set(kSecretKey, m_token);
    m_authed = false;
    m_email.clear(); m_plan.clear(); m_expiry.clear();
    emit accountChanged();
    if (!m_token.isEmpty())
        checkAccount();
}

void TorBoxClient::clearToken()
{
    setToken(QString());
}

void TorBoxClient::checkAccount()
{
    if (m_token.isEmpty()) return;
    QNetworkReply *r = get(QStringLiteral("user/me?settings=false"));
    connect(r, &QNetworkReply::finished, this, [this, r] {
        r->deleteLater();
        const QJsonObject root = QJsonDocument::fromJson(r->readAll()).object();
        const QJsonObject d = root.value(QStringLiteral("data")).toObject();
        if (r->error() != QNetworkReply::NoError
            || !root.value(QStringLiteral("success")).toBool() || d.isEmpty()) {
            m_authed = false;
            m_email.clear(); m_plan.clear(); m_expiry.clear();
            emit accountChanged();
            if (r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 401)
                emit errorOccurred(tr("TorBox rejected the token — check it and paste again."));
            return;
        }
        m_email = d.value(QStringLiteral("email")).toString();
        const int plan = d.value(QStringLiteral("plan")).toInt();
        static const char *kPlans[] = {"Free", "Essential", "Pro", "Standard"};
        m_plan = (plan >= 0 && plan < 4) ? QString::fromLatin1(kPlans[plan])
                                         : tr("Plan %1").arg(plan);
        m_expiry = d.value(QStringLiteral("premium_expires_at")).toString().left(10);
        m_authed = (plan > 0);
        emit accountChanged();
    });
}

// --- Stream pipeline -------------------------------------------------------

void TorBoxClient::streamMagnet(const QString &magnet)
{
    if (m_token.isEmpty()) {
        emit errorOccurred(tr("Add your TorBox API key in Settings first."));
        return;
    }
    if (m_busy) {
        emit errorOccurred(tr("A TorBox transfer is already in progress."));
        return;
    }
    m_jobId.clear();
    m_jobName.clear();
    m_pollTries = 0;
    setBusy(true);
    setStatus(tr("Sending to TorBox…"), 0);

    auto *mp = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader,
                   QStringLiteral("form-data; name=\"magnet\""));
    part.setBody(magnet.toUtf8());
    mp->append(part);

    QNetworkRequest req = authedRequest(QStringLiteral("torrents/createtorrent"));
    QNetworkReply *r = m_nam.post(req, mp);
    mp->setParent(r);
    connect(r, &QNetworkReply::finished, this, [this, r] { onCreate(r); });
}

void TorBoxClient::onCreate(QNetworkReply *r)
{
    r->deleteLater();
    const QJsonObject root = QJsonDocument::fromJson(r->readAll()).object();
    const QJsonObject d = root.value(QStringLiteral("data")).toObject();
    if (r->error() != QNetworkReply::NoError
        || !root.value(QStringLiteral("success")).toBool()) {
        const QString detail = root.value(QStringLiteral("detail")).toString();
        failJob(detail.isEmpty() ? tr("TorBox couldn't accept this magnet.") : detail);
        return;
    }
    const QJsonValue idVal = d.value(QStringLiteral("torrent_id"));
    m_jobId = idVal.isString() ? idVal.toString() : QString::number(idVal.toInt());
    if (m_jobId.isEmpty() || m_jobId == QLatin1String("0")) {
        failJob(tr("TorBox couldn't accept this magnet."));
        return;
    }
    setStatus(tr("Preparing on TorBox…"), 0);
    m_pollTimer.start(0);   // first poll immediately
}

void TorBoxClient::pollJob()
{
    if (m_jobId.isEmpty()) return;
    if (++m_pollTries > kMaxPolls) {
        failJob(tr("TorBox is taking too long — try again later."));
        return;
    }
    QNetworkReply *r = get(QStringLiteral("torrents/mylist?bypass_cache=true&id=") + m_jobId);
    connect(r, &QNetworkReply::finished, this, [this, r] { onJobInfo(r); });
}

void TorBoxClient::onJobInfo(QNetworkReply *r)
{
    r->deleteLater();
    if (m_jobId.isEmpty()) return;   // cancelled mid-flight
    const QJsonObject root = QJsonDocument::fromJson(r->readAll()).object();
    // mylist with id returns the torrent in `data` (object); 404 while it hasn't
    // registered yet — both are transient, keep polling.
    if (r->error() != QNetworkReply::NoError
        || !root.value(QStringLiteral("success")).toBool()) {
        m_pollTimer.start();
        return;
    }
    const QJsonObject o = root.value(QStringLiteral("data")).toObject();
    if (o.isEmpty()) { m_pollTimer.start(); return; }

    m_jobName = o.value(QStringLiteral("name")).toString();
    const bool finished = o.value(QStringLiteral("download_finished")).toBool();
    const bool present = o.value(QStringLiteral("download_present")).toBool();

    if (finished && present) {
        const auto best = pickBestFile(o.value(QStringLiteral("files")).toArray());
        if (best.first < 0) {
            failJob(tr("No playable file in this TorBox transfer."));
            return;
        }
        setStatus(tr("Unlocking stream…"), 100);
        m_jobName = best.second.isEmpty() ? m_jobName : best.second;
        requestLink(best.first);
        return;
    }

    const QString state = o.value(QStringLiteral("download_state")).toString();
    const int pct = qBound(0, int(o.value(QStringLiteral("progress")).toDouble() * 100.0), 100);
    if (state.startsWith(QLatin1String("stalled"))
        || state == QLatin1String("error") || state == QLatin1String("missingFiles")) {
        failJob(tr("TorBox failed on this torrent (%1).").arg(state));
        return;
    }
    setStatus(tr("TorBox is caching…"), pct);
    m_pollTimer.start();
}

void TorBoxClient::requestLink(int fileId)
{
    // requestdl wants the token as a query param (it mints a per-request CDN link).
    QUrl url(QString::fromLatin1(kBase) + QStringLiteral("torrents/requestdl"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("token"), m_token);
    q.addQueryItem(QStringLiteral("torrent_id"), m_jobId);
    q.addQueryItem(QStringLiteral("file_id"), QString::number(fileId));
    url.setQuery(q);
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", "Bearer " + m_token.toUtf8());
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    const QString name = m_jobName;
    QNetworkReply *r = m_nam.get(req);
    connect(r, &QNetworkReply::finished, this, [this, r, name] { onRequestLink(r, name); });
}

void TorBoxClient::onRequestLink(QNetworkReply *r, const QString &name)
{
    r->deleteLater();
    const QJsonObject root = QJsonDocument::fromJson(r->readAll()).object();
    const QString url = root.value(QStringLiteral("data")).toString();
    if (r->error() != QNetworkReply::NoError
        || !root.value(QStringLiteral("success")).toBool() || url.isEmpty()) {
        failJob(tr("TorBox couldn't unlock the stream link."));
        return;
    }
    m_jobId.clear();
    setStatus(QString(), 100);
    setBusy(false);
    emit streamReady(url, name);
}

void TorBoxClient::cancelStream()
{
    m_pollTimer.stop();
    m_jobId.clear();
    setStatus(QString(), 0);
    setBusy(false);
}

void TorBoxClient::failJob(const QString &msg)
{
    m_pollTimer.stop();
    m_jobId.clear();
    setStatus(QString(), 0);
    setBusy(false);
    emit errorOccurred(msg);
}

// --- small helpers ---------------------------------------------------------

void TorBoxClient::setBusy(bool b)
{
    if (m_busy == b) return;
    m_busy = b;
    emit busyChanged();
}

void TorBoxClient::setStatus(const QString &s, int progress)
{
    m_status = s;
    if (progress >= 0) m_progress = progress;
    emit statusChanged();
}

bool TorBoxClient::looksLikeVideo(const QString &path)
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

// TorBox files carry their own `id` (index within the torrent). Return the id +
// name of the largest video file, else the largest file of any kind, else {-1}.
QPair<int, QString> TorBoxClient::pickBestFile(const QJsonArray &files)
{
    int bestVideo = -1;  qint64 bestVideoBytes = -1;  QString bestVideoName;
    int bestAny = -1;    qint64 bestAnyBytes = -1;    QString bestAnyName;
    for (const QJsonValue &v : files) {
        const QJsonObject f = v.toObject();
        const int id = f.value(QStringLiteral("id")).toInt(-1);
        if (id < 0) continue;
        const qint64 bytes = qint64(f.value(QStringLiteral("size")).toDouble());
        const QString name = f.value(QStringLiteral("name")).toString();
        if (bytes > bestAnyBytes) { bestAnyBytes = bytes; bestAny = id; bestAnyName = name; }
        if (looksLikeVideo(name) && bytes > bestVideoBytes) {
            bestVideoBytes = bytes; bestVideo = id; bestVideoName = name;
        }
    }
    if (bestVideo >= 0) return {bestVideo, bestVideoName};
    return {bestAny, bestAnyName};
}
