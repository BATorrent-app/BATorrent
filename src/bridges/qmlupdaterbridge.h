// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef QMLUPDATERBRIDGE_H
#define QMLUPDATERBRIDGE_H

#include "bridges/bridgecommon.h"

class QmlUpdaterBridge : public QObject
{
    Q_OBJECT
    // Where the last check left things, kept across launches for Settings:
    // "" never checked, "checking", "current", "available", "failed".
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY statusChanged)   // what the last answer offered
    Q_PROPERTY(QString failure READ failure NOTIFY statusChanged)               // why the last check failed
    Q_PROPERTY(qint64 lastCheckedAt READ lastChecked NOTIFY statusChanged)
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

    QString status() const { return m_status; }
    QString latestVersion() const { return m_latest; }
    QString failure() const { return m_failure; }
    // What a check's answer does to the status (public so a test can drive it
    // without the network): found a version, found none, or failed.
    void noteFound(const QString &version);
    void noteNone();
    void noteFailed(const QString &message);

signals:
    void updateFound(const QString &version, const QString &url, const QString &assetName);
    void noUpdate(bool silent);
    void progress(int percent);
    void ready();
    void failed(const QString &message, bool silent);
    void statusChanged();

private:
    Updater *m_updater;
    bool m_silent = false;
    QString m_status;
    QString m_latest;
    QString m_failure;
    void setStatus(const QString &status, const QString &latest, const QString &failure);
};

#endif // QMLUPDATERBRIDGE_H
