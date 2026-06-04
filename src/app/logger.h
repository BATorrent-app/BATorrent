// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_LOGGER_H
#define BATORRENT_LOGGER_H

#include <QFile>
#include <QMutex>
#include <QString>

class Logger
{
public:
    enum Level { Trace = 0, Debug = 1, Info = 2, Warning = 3, Error = 4, Critical = 5 };

    static Logger &instance();

    // Configuration. setLevel persists to QSettings under "logLevel"; load on
    // next startup via init().
    void init();
    void setLevel(Level l);
    Level level() const { return m_level; }

    // Emit a record. Filtered against the current level. Safe to call from
    // any thread.
    void log(Level l, const QString &msg);

    // File-system access for the Log viewer dialog and the manual export.
    QString logsDir() const;
    QString currentLogPath() const;
    // Concatenate the current log + any rotated siblings (.1 / .2) into one
    // continuous text, oldest entries first. Used by the "Save logs..." UX.
    QString readAllLogs() const;
    // Wipe the current log file (rotated siblings are left alone).
    void clear();

    static QString levelName(Level l);
    static Level levelFromName(const QString &name);

private:
    Logger() = default;
    void rotateIfNeeded();
    void writeLine(Level l, const QString &msg);

    QFile m_file;
    QMutex m_mutex;
    Level m_level = Info;
    qint64 m_maxBytes = 5 * 1024 * 1024;  // 5 MB per file
    int m_keepRotations = 3;
};

#endif
