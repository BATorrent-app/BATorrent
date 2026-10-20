// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/qmlupdaterbridge.h"

#include "services/integrations/updater.h"

#include "services/platform/utils.h"

#include <QDateTime>
#include <QRegularExpression>
#include <QSettings>

QmlUpdaterBridge::QmlUpdaterBridge(QObject *parent)
    : QObject(parent), m_updater(new Updater(this))
{
    {
        QSettings s;
        m_status = s.value("lastUpdateStatus").toString();
        m_latest = s.value("lastUpdateLatest").toString();
        m_failure = s.value("lastUpdateFailure").toString();
        if (m_status == QLatin1String("checking")) m_status.clear();   // the app quit mid-check
    }
    connect(m_updater, &Updater::updateAvailable, this,
            [this](const QString &v, const QString &url, const QString &asset) {
        noteFound(v);
        if (QSettings().value("skippedUpdateVersion").toString() == v && m_silent)
            return;
        emit updateFound(v, url, asset);
    });
    connect(m_updater, &Updater::noUpdateAvailable, this, [this]() {
        noteNone();
        emit noUpdate(m_silent);
    });
    connect(m_updater, &Updater::downloadProgress, this, [this](qint64 r, qint64 t) {
        emit progress(t > 0 ? static_cast<int>((r * 100) / t) : 0);
    });
    connect(m_updater, &Updater::updateReady, this, &QmlUpdaterBridge::ready);
    connect(m_updater, &Updater::errorOccurred, this,
            [this](const QString &msg) {
        // a download that failed after a found version keeps "available"
        if (m_status == QLatin1String("checking")) noteFailed(msg);
        emit failed(msg, m_silent);
    });
}

void QmlUpdaterBridge::check(bool silent)
{
    m_silent = silent;
    setStatus(QStringLiteral("checking"), m_latest, QString());
    m_updater->checkForUpdate();
}

void QmlUpdaterBridge::setStatus(const QString &status, const QString &latest, const QString &failure)
{
    if (status == m_status && latest == m_latest && failure == m_failure) return;
    m_status = status;
    m_latest = latest;
    m_failure = failure;
    QSettings s;
    s.setValue("lastUpdateStatus", status);
    s.setValue("lastUpdateLatest", latest);
    s.setValue("lastUpdateFailure", failure);
    emit statusChanged();
}

void QmlUpdaterBridge::noteFound(const QString &version)
{
    QSettings().setValue("lastUpdateCheck", QDateTime::currentSecsSinceEpoch());
    setStatus(QStringLiteral("available"), version, QString());
    emit statusChanged();   // the time moved even when nothing else did
}

void QmlUpdaterBridge::noteNone()
{
    QSettings().setValue("lastUpdateCheck", QDateTime::currentSecsSinceEpoch());
    setStatus(QStringLiteral("current"), QStringLiteral(APP_VERSION), QString());
    emit statusChanged();
}

void QmlUpdaterBridge::noteFailed(const QString &message)
{
    // not an answer: the last good check's time stays what it was
    setStatus(QStringLiteral("failed"), m_latest, message);
}

void QmlUpdaterBridge::downloadAndInstall(const QString &url, const QString &assetName)
{
    m_updater->downloadAndInstall(url, assetName);
}

QString QmlUpdaterBridge::releaseSize() const
{
    return m_updater->latestSize() > 0 ? formatSize(m_updater->latestSize()) : QString();
}

// The bullet points of the release text, bold and links taken off
QStringList QmlUpdaterBridge::releaseHighlights(int max) const
{
    static const QRegularExpression bullet(QStringLiteral("^\\s*[-*]\\s+(.+)$"));
    static const QRegularExpression link(QStringLiteral("\\[([^\\]]+)\\]\\([^)]*\\)"));
    QStringList out;
    for (const QString &line : m_updater->latestNotes().split(QLatin1Char('\n'))) {
        const auto m = bullet.match(line);
        if (!m.hasMatch()) continue;
        QString t = m.captured(1).trimmed();
        t.replace(link, QStringLiteral("\\1"));
        t.remove(QStringLiteral("**")).remove(QLatin1Char('`'));
        if (t.isEmpty()) continue;
        out << t;
        if (out.size() >= max) break;
    }
    return out;
}

qint64 QmlUpdaterBridge::lastChecked() const
{
    return QSettings().value("lastUpdateCheck", 0).toLongLong();
}

void QmlUpdaterBridge::skipVersion(const QString &version)
{
    QSettings().setValue("skippedUpdateVersion", version);
}
