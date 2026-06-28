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

signals:
    void textChanged();
    void levelChanged();

private slots:
    void poll();

private:
    QString m_text;
    qint64 m_lastSize = 0;
    QTimer m_pollTimer;
};

#endif // QMLLOGBRIDGE_H
