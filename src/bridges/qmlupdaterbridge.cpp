// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/qmlupdaterbridge.h"

#include "services/integrations/updater.h"

#include <QSettings>

QmlUpdaterBridge::QmlUpdaterBridge(QObject *parent)
    : QObject(parent), m_updater(new Updater(this))
{
    connect(m_updater, &Updater::updateAvailable, this,
            [this](const QString &v, const QString &url, const QString &asset) {
        QSettings s;
        if (s.value("skippedUpdateVersion").toString() == v && m_silent)
            return;
        emit updateFound(v, url, asset);
    });
    connect(m_updater, &Updater::noUpdateAvailable, this,
            [this]() { emit noUpdate(m_silent); });
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

void QmlUpdaterBridge::skipVersion(const QString &version)
{
    QSettings().setValue("skippedUpdateVersion", version);
}
