// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef QMLUPDATERBRIDGE_H
#define QMLUPDATERBRIDGE_H

#include "bridges/bridgecommon.h"

class QmlUpdaterBridge : public QObject
{
    Q_OBJECT
public:
    explicit QmlUpdaterBridge(QObject *parent = nullptr);

    Q_INVOKABLE void check(bool silent = false);
    Q_INVOKABLE void downloadAndInstall(const QString &url, const QString &assetName);
    Q_INVOKABLE void skipVersion(const QString &version);
    // the found release: its download size, and the first lines of its notes
    Q_INVOKABLE QString releaseSize() const;
    Q_INVOKABLE QStringList releaseHighlights(int max) const;
    // unix seconds of the last check that got an answer, 0 never
    Q_INVOKABLE qint64 lastChecked() const;
    // the check that just answered was the quiet one at startup
    Q_INVOKABLE bool silentCheck() const { return m_silent; }

signals:
    void updateFound(const QString &version, const QString &url, const QString &assetName);
    void noUpdate(bool silent);
    void progress(int percent);
    void ready();
    void failed(const QString &message, bool silent);

private:
    Updater *m_updater;
    bool m_silent = false;
};

#endif // QMLUPDATERBRIDGE_H
