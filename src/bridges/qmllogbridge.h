// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef QMLLOGBRIDGE_H
#define QMLLOGBRIDGE_H

#include "bridges/bridgecommon.h"

class QmlLogBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text NOTIFY textChanged)
    Q_PROPERTY(int level READ level WRITE setLevel NOTIFY levelChanged)
    Q_PROPERTY(QStringList levelNames READ levelNames CONSTANT)
    Q_PROPERTY(QString logsDir READ logsDir CONSTANT)
    // This session's lines, read into fields for the Logs window
    Q_PROPERTY(QVariantList counts READ counts NOTIFY entriesChanged)          // [errors, warnings, info, all], each with the levels above it
    Q_PROPERTY(QString sessionLine READ sessionLine NOTIFY entriesChanged)     // "Session started 11:57 · 4.9.0 · Qt 6.7.3 · libtorrent 2.0.12"
public:
    explicit QmlLogBridge(QObject *parent = nullptr);

    QString text() const { return m_text; }
    int level() const;
    void setLevel(int l);
    QStringList levelNames() const;
    QString logsDir() const;

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void clearLog();
    Q_INVOKABLE void openLogsFolder();
    Q_INVOKABLE bool exportLogs(const QString &filePath);
    Q_INVOKABLE QString defaultExportName() const;
    Q_INVOKABLE bool previousSessionCrashed() const;
    Q_INVOKABLE QString crashReportUrl() const;
    // minLevel: 0 everything, 1 info and up, 2 warnings and up, 3 errors;
    // filter matches the module or the message; the newest `max` lines
    Q_INVOKABLE QVariantList entries(int minLevel, const QString &filter, int max) const;
    // the session's last lines on the clipboard, with the user's paths and name taken out
    Q_INVOKABLE int copyForReport(int lines) const;
    QVariantList counts() const;
    QString sessionLine() const { return m_sessionLine; }

signals:
    void textChanged();
    void levelChanged();
    void entriesChanged();

private slots:
    void poll();

private:
    struct Entry { QString time; int level = 1; QString module; QString msg; QString raw; };
    void parse(const QString &chunk);
    QVector<Entry> m_entries;
    QString m_partial;
    QString m_sessionLine;
    QString m_text;
    qint64 m_lastSize = 0;
    QTimer m_pollTimer;
};

#endif // QMLLOGBRIDGE_H
