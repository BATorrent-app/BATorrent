// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "logger.h"
#include <QDateTime>
#include <QDir>
#include <QMutexLocker>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>

Logger &Logger::instance()
{
    static Logger inst;
    return inst;
}

void Logger::init()
{
    QSettings s("BATorrent", "BATorrent");
    const QString lvl = s.value("logLevel", "Info").toString();
    m_level = levelFromName(lvl);

    // Env var override — useful for one-off troubleshooting without touching
    // QSettings or rebuilding.
    QByteArray env = qgetenv("BATORRENT_LOG_LEVEL");
    if (!env.isEmpty())
        m_level = levelFromName(QString::fromUtf8(env));

    QDir dir(logsDir());
    if (!dir.exists()) dir.mkpath(".");

    m_file.setFileName(currentLogPath());
    (void)m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);

    // Detect unclean shutdown: if the previous session didn't write
    // "--- log closed ---" before the file ended, it crashed.
    {
        QFile readCheck(currentLogPath());
        if (readCheck.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray tail = readCheck.readAll().right(4000);
            readCheck.close();
            if (!tail.isEmpty() && !tail.right(200).contains("--- log closed ---")
                && tail.contains("--- log opened ---")) {
                m_prevCrashed = true;
                m_crashTail = QString::fromUtf8(tail.right(1800));
                writeLine(Warning, QStringLiteral("--- previous session ended abnormally (crash or force-kill) ---"));
            }
        }
    }
    writeLine(Info, QStringLiteral("--- log opened ---"));
}

void Logger::setLevel(Level l)
{
    QMutexLocker lock(&m_mutex);
    m_level = l;
    QSettings("BATorrent", "BATorrent").setValue("logLevel", levelName(l));
}

void Logger::log(Level l, const QString &msg)
{
    if (l < m_level) return;
    QMutexLocker lock(&m_mutex);
    rotateIfNeeded();
    writeLine(l, msg);
}

void Logger::writeLine(Level l, const QString &msg)
{
    if (!m_file.isOpen()) return;
    const QString ts = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    QTextStream ts_out(&m_file);
    ts_out << ts << " [" << levelName(l) << "] " << msg << '\n';
    m_file.flush();
}

void Logger::rotateIfNeeded()
{
    if (m_file.size() < m_maxBytes) return;
    m_file.close();
    QDir dir(logsDir());
    // Cascade: .2 → drop, .1 → .2, current → .1
    for (int i = m_keepRotations - 1; i >= 1; --i) {
        QString from = QStringLiteral("batorrent.log.%1").arg(i);
        QString to   = QStringLiteral("batorrent.log.%1").arg(i + 1);
        if (dir.exists(to))    dir.remove(to);
        if (dir.exists(from))  dir.rename(from, to);
    }
    if (dir.exists("batorrent.log"))
        dir.rename("batorrent.log", "batorrent.log.1");
    m_file.setFileName(currentLogPath());
    (void)m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
}

QString Logger::logsDir() const
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
        .filePath("logs");
}

QString Logger::currentLogPath() const
{
    return QDir(logsDir()).filePath("batorrent.log");
}

QString Logger::readAllLogs() const
{
    // Concatenate oldest-first so the resulting export reads chronologically.
    QDir dir(logsDir());
    QStringList parts;
    for (int i = m_keepRotations; i >= 1; --i) {
        QString p = dir.filePath(QStringLiteral("batorrent.log.%1").arg(i));
        if (!QFile::exists(p)) continue;
        QFile f(p);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text))
            parts.append(QString::fromUtf8(f.readAll()));
    }
    QFile cur(currentLogPath());
    if (cur.open(QIODevice::ReadOnly | QIODevice::Text))
        parts.append(QString::fromUtf8(cur.readAll()));
    return parts.join(QString());
}

void Logger::clear()
{
    QMutexLocker lock(&m_mutex);
    m_file.close();
    QFile::remove(currentLogPath());
    m_file.setFileName(currentLogPath());
    (void)m_file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    writeLine(Info, QStringLiteral("--- log cleared by user ---"));
}

QString Logger::levelName(Level l)
{
    switch (l) {
    case Trace:    return QStringLiteral("TRACE");
    case Debug:    return QStringLiteral("DEBUG");
    case Info:     return QStringLiteral("INFO");
    case Warning:  return QStringLiteral("WARN");
    case Error:    return QStringLiteral("ERROR");
    case Critical: return QStringLiteral("CRIT");
    }
    return QStringLiteral("INFO");
}

Logger::Level Logger::levelFromName(const QString &name)
{
    const QString u = name.toUpper();
    if (u == "TRACE") return Trace;
    if (u == "DEBUG") return Debug;
    if (u == "INFO")  return Info;
    if (u == "WARN" || u == "WARNING") return Warning;
    if (u == "ERROR") return Error;
    if (u == "CRIT" || u == "CRITICAL") return Critical;
    return Info;
}
