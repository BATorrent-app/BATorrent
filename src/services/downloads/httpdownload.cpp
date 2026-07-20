// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/downloads/httpdownload.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QUrlQuery>
#include <QRegularExpression>

namespace {

constexpr int kStallMs = 30000;                 // no progress this long ⇒ stall
constexpr qint64 kMaxTotal = qint64(1) << 42;   // 4 TiB sanity cap on advertised size

void prepare(QNetworkRequest &req)
{
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QStringLiteral("BATorrent/") + QCoreApplication::applicationVersion());
    // never let Qt's inactivity timeout kill a long transfer; we do our own
    // stall detection instead.
    req.setTransferTimeout(0);
}

// "bytes 0-0/123456" -> 123456 ; -1 if absent/unparseable/"*"
qint64 totalFromContentRange(const QByteArray &cr)
{
    const int slash = cr.lastIndexOf('/');
    if (slash < 0) return -1;
    const QByteArray tail = cr.mid(slash + 1).trimmed();
    if (tail == "*" || tail.isEmpty()) return -1;
    bool ok = false;
    const qint64 v = tail.toLongLong(&ok);
    return (ok && v > 0 && v <= kMaxTotal) ? v : -1;
}

} // namespace

HttpDownload::HttpDownload(const QString &id, const QUrl &url,
                          const QString &saveDir, const QString &fileName,
                          QObject *parent)
    : QObject(parent), m_id(id), m_url(url), m_saveDir(saveDir), m_fileName(fileName)
{
    if (m_fileName.isEmpty()) {
        m_fileName = QFileInfo(url.path()).fileName();
        if (m_fileName.isEmpty()) m_fileName = QStringLiteral("download-") + id;
    }
}

QString HttpDownload::finalPath() const { return QDir(m_saveDir).filePath(m_fileName); }
QString HttpDownload::partPath() const  { return finalPath() + QStringLiteral(".part"); }

qint64 HttpDownload::receivedBytes() const
{
    qint64 sum = 0;
    for (const Segment &s : m_segments) sum += s.received;
    return sum;
}

double HttpDownload::progress() const
{
    if (m_total <= 0) return 0.0;
    return double(receivedBytes()) / double(m_total);
}

void HttpDownload::setState(State s)
{
    if (m_state == s) return;
    m_state = s;
    emit stateChanged();
}

void HttpDownload::fail(const QString &why)
{
    m_error = why;
    abortReplies();
    if (m_file.isOpen()) m_file.close();
    if (m_sampler) m_sampler->stop();
    setState(State::Failed);
    emit finished(false);
}

void HttpDownload::abortReplies()
{
    // abort() emits finished() synchronously, which reenters onSegmentFinished
    // and clears s.reply — so null the slot *before* aborting, or the next line
    // would dereference a reply the reentrant call just cleared.
    if (m_probe) { QNetworkReply *p = m_probe; m_probe = nullptr; p->abort(); p->deleteLater(); }
    for (Segment &s : m_segments) {
        if (s.reply) { QNetworkReply *r = s.reply; s.reply = nullptr; r->abort(); r->deleteLater(); }
    }
}

void HttpDownload::start()
{
    if (m_state == State::Downloading || m_state == State::Probing) return;
    m_error.clear();

    if (!m_sampler) {
        m_sampler = new QTimer(this);
        m_sampler->setInterval(1000);
        connect(m_sampler, &QTimer::timeout, this, [this]() {
            const qint64 now = receivedBytes();
            m_rate = int(now - m_lastSampledBytes);
            if (m_rate > 0) m_lastProgressMs = m_clock.elapsed();
            m_lastSampledBytes = now;
            emit progressed();
            if (m_state == State::Downloading
                && m_clock.elapsed() - m_lastProgressMs > kStallMs) {
                if (!m_stallRetried) {   // one silent kick before giving up
                    m_stallRetried = true;
                    m_lastProgressMs = m_clock.elapsed();
                    for (int i = 0; i < m_segments.size(); ++i)
                        if (m_segments[i].reply == nullptr && !m_segments[i].done())
                            spawnSegment(i);
                } else {
                    fail(QStringLiteral("stalled"));
                }
            }
        });
    }
    m_clock.start();
    m_lastProgressMs = 0;
    m_lastSampledBytes = receivedBytes();

    // Already know the layout (restored from a sidecar): skip the probe.
    if (m_total > 0 && !m_segments.isEmpty()) { beginSegments(); return; }

    setState(State::Probing);
    QNetworkRequest req(m_url);
    prepare(req);
    req.setRawHeader("Range", "bytes=0-0");   // 206 ⇒ ranges + total; 200 ⇒ no ranges
    m_probe = m_nam.get(req);
    connect(m_probe, &QNetworkReply::finished, this, &HttpDownload::onProbeFinished);
}

void HttpDownload::onProbeFinished()
{
    if (!m_probe) return;
    QNetworkReply *r = m_probe;
    m_probe = nullptr;
    r->deleteLater();

    if (r->error() != QNetworkReply::NoError) { fail(r->errorString()); return; }

    const int code = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (r->hasRawHeader("Content-Disposition") && m_fileName.startsWith(QStringLiteral("download-"))) {
        const QString fn = fileNameFromReply(r, m_url);
        if (!fn.isEmpty()) m_fileName = fn;
    }

    if (code == 206) {
        m_acceptRanges = true;
        m_total = totalFromContentRange(r->rawHeader("Content-Range"));
    } else {
        // Server ignored the Range (200) ⇒ single stream; size from Content-Length.
        m_acceptRanges = false;
        bool ok = false;
        const qint64 len = r->header(QNetworkRequest::ContentLengthHeader).toLongLong(&ok);
        m_total = (ok && len > 0 && len <= kMaxTotal) ? len : -1;
    }

    // Plan the segments (or one stream when ranges/size unavailable).
    m_segments.clear();
    if (m_acceptRanges && m_total > 0) {
        for (const bat::ByteRange &br : bat::planSegments(m_total, m_conns))
            m_segments.push_back(Segment{br, 0, nullptr});
    } else {
        // one stream; range.end = total-1 if known, else sentinel -1 (append)
        m_segments.push_back(Segment{ bat::ByteRange{0, m_total > 0 ? m_total - 1 : -1}, 0, nullptr });
    }
    beginSegments();
}

void HttpDownload::beginSegments()
{
    QDir().mkpath(m_saveDir);
    if (!m_file.isOpen()) {
        m_file.setFileName(partPath());
        if (!m_file.open(QIODevice::ReadWrite)) { fail(QStringLiteral("cannot open ") + partPath()); return; }
        if (m_total > 0 && m_file.size() != m_total) m_file.resize(m_total);   // preallocate
    }
    setState(State::Downloading);
    m_stallRetried = false;
    m_sampler->start();
    for (int i = 0; i < m_segments.size(); ++i)
        if (!m_segments[i].done()) spawnSegment(i);
    maybeComplete();   // all-restored-complete edge case
}

void HttpDownload::spawnSegment(int idx)
{
    Segment &s = m_segments[idx];
    if (s.reply || s.done()) return;
    QNetworkRequest req(m_url);
    prepare(req);
    if (m_acceptRanges && s.range.end >= 0) {
        const qint64 from = s.range.start + s.received;
        req.setRawHeader("Range", QByteArray("bytes=")
                         + QByteArray::number(from) + '-' + QByteArray::number(s.range.end));
    }
    s.reply = m_nam.get(req);
    s.reply->setProperty("segIdx", idx);
    connect(s.reply, &QNetworkReply::readyRead, this, &HttpDownload::onSegmentReadyRead);
    connect(s.reply, &QNetworkReply::finished, this, &HttpDownload::onSegmentFinished);
}

void HttpDownload::onSegmentReadyRead()
{
    auto *r = qobject_cast<QNetworkReply *>(sender());
    if (!r || m_state != State::Downloading) return;
    const int idx = r->property("segIdx").toInt();
    if (idx < 0 || idx >= m_segments.size()) return;
    Segment &s = m_segments[idx];

    const QByteArray chunk = r->readAll();
    if (chunk.isEmpty()) return;
    const qint64 writeAt = s.range.start + s.received;
    if (!m_file.seek(writeAt) || m_file.write(chunk) != chunk.size()) {
        fail(QStringLiteral("disk write failed")); return;
    }
    s.received += chunk.size();
    // A ranged segment must never overrun its slice (a misbehaving server).
    if (s.range.end >= 0 && s.range.start + s.received > s.range.end + 1) {
        fail(QStringLiteral("segment overrun")); return;
    }
}

void HttpDownload::onSegmentFinished()
{
    auto *r = qobject_cast<QNetworkReply *>(sender());
    if (!r) return;
    const int idx = r->property("segIdx").toInt();
    r->deleteLater();
    if (idx < 0 || idx >= m_segments.size()) return;
    Segment &s = m_segments[idx];
    if (s.reply != r) return;   // stale (aborted) reply
    s.reply = nullptr;

    if (m_state != State::Downloading) return;   // paused/cancelled mid-flight
    if (r->error() != QNetworkReply::NoError) { fail(r->errorString()); return; }

    // Unknown-size single stream: the total is whatever arrived.
    if (m_total <= 0 && s.range.end < 0) {
        m_total = s.received;
        s.range.end = s.received - 1;
    }
    maybeComplete();
}

void HttpDownload::maybeComplete()
{
    for (const Segment &s : m_segments) if (!s.done()) return;

    m_file.flush();
    const qint64 got = receivedBytes();
    m_file.close();

    // Integrity gate: a truncated download or an HTML error page (200) must not
    // be renamed to the real name and handed to the user as the file.
    if (m_total > 0 && got != m_total) {
        fail(QStringLiteral("size mismatch: got %1 of %2").arg(got).arg(m_total));
        return;
    }

    QFile::remove(finalPath());
    if (!QFile::rename(partPath(), finalPath())) {
        fail(QStringLiteral("cannot finalize ") + finalPath());
        return;
    }
    if (m_sampler) m_sampler->stop();
    m_rate = 0;
    setState(State::Completed);
    emit progressed();
    emit finished(true);
}

void HttpDownload::pause()
{
    if (m_state != State::Downloading && m_state != State::Probing) return;
    abortReplies();
    if (m_file.isOpen()) m_file.flush();
    if (m_sampler) m_sampler->stop();
    m_rate = 0;
    setState(State::Paused);
    emit progressed();
}

void HttpDownload::resume()
{
    if (m_state != State::Paused && m_state != State::Failed) return;
    start();
}

void HttpDownload::cancel(bool deletePartial)
{
    abortReplies();
    if (m_file.isOpen()) m_file.close();
    if (m_sampler) m_sampler->stop();
    if (deletePartial) QFile::remove(partPath());
}

QString HttpDownload::fileNameFromReply(QNetworkReply *r, const QUrl &url)
{
    const QByteArray cd = r->rawHeader("Content-Disposition");
    if (!cd.isEmpty()) {
        // filename*=UTF-8''name  OR  filename="name"
        static const QRegularExpression re(
            QStringLiteral("filename\\*?=(?:UTF-8''|\")?([^\";]+)"),
            QRegularExpression::CaseInsensitiveOption);
        const auto m = re.match(QString::fromUtf8(cd));
        if (m.hasMatch()) {
            QString fn = QUrl::fromPercentEncoding(m.captured(1).trimmed().toUtf8());
            fn = fn.section('/', -1).section('\\', -1);   // strip any path
            if (!fn.isEmpty()) return fn;
        }
    }
    return QFileInfo(url.path()).fileName();
}

QVariantMap HttpDownload::toState() const
{
    QVariantMap m;
    m["id"] = m_id;
    m["url"] = m_url.toString();
    m["saveDir"] = m_saveDir;
    m["fileName"] = m_fileName;
    m["total"] = m_total;
    m["acceptRanges"] = m_acceptRanges;
    m["state"] = int(m_state == State::Completed ? State::Completed : State::Paused);
    QVariantList segs;
    for (const Segment &s : m_segments) {
        QVariantMap sm;
        // ByteRange keeps std int64_t (a pure, Qt-free header). On GCC/Linux
        // int64_t is `long`, which has no exact QVariant ctor (→ ambiguous), so
        // convert to qint64 (`long long`) at this Qt boundary.
        sm["start"] = qint64(s.range.start); sm["end"] = qint64(s.range.end); sm["received"] = s.received;
        segs << sm;
    }
    m["segments"] = segs;
    return m;
}

void HttpDownload::restoreState(const QVariantMap &m)
{
    m_total = m.value("total", -1).toLongLong();
    m_acceptRanges = m.value("acceptRanges", false).toBool();
    m_state = State(m.value("state", int(State::Paused)).toInt());
    m_segments.clear();
    for (const QVariant &v : m.value("segments").toList()) {
        const QVariantMap sm = v.toMap();
        Segment s;
        s.range.start = sm.value("start").toLongLong();
        s.range.end = sm.value("end").toLongLong();
        s.received = sm.value("received").toLongLong();
        m_segments.push_back(s);
    }
}
