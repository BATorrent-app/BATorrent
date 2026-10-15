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
    connect(m_updater, &Updater::updateAvailable, this,
            [this](const QString &v, const QString &url, const QString &asset) {
        QSettings s;
        s.setValue("lastUpdateCheck", QDateTime::currentSecsSinceEpoch());
        if (s.value("skippedUpdateVersion").toString() == v && m_silent)
            return;
        emit updateFound(v, url, asset);
    });
    connect(m_updater, &Updater::noUpdateAvailable, this, [this]() {
        QSettings().setValue("lastUpdateCheck", QDateTime::currentSecsSinceEpoch());
        emit noUpdate(m_silent);
    });
    connect(m_updater, &Updater::downloadProgress, this, [this](qint64 r, qint64 t) {
        emit progress(t > 0 ? static_cast<int>((r * 100) / t) : 0);
    });
    connect(m_updater, &Updater::updateReady, this, &QmlUpdaterBridge::ready);
    connect(m_updater, &Updater::errorOccurred, this,
            [this](const QString &msg) { emit failed(msg, m_silent); });
}

void QmlUpdaterBridge::check(bool silent)
{
    m_silent = silent;
    m_updater->checkForUpdate();
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
