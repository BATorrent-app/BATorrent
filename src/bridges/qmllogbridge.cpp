// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/qmllogbridge.h"

#include "services/platform/logger.h"
#include "services/security/crashhandler.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QFile>
#include <QStandardPaths>
#include <QSysInfo>
#include <QUrl>
#include <QUrlQuery>

QmlLogBridge::QmlLogBridge(QObject *parent) : QObject(parent)
{
    m_pollTimer.setInterval(1000);
    connect(&m_pollTimer, &QTimer::timeout, this, &QmlLogBridge::poll);
}

int QmlLogBridge::level() const { return int(Logger::instance().level()); }

void QmlLogBridge::setLevel(int l)
{
    Logger::instance().setLevel(Logger::Level(l));
    m_lastSize = 0;
    poll();
    emit levelChanged();
}

QStringList QmlLogBridge::levelNames() const
{
    return { "Trace", "Debug", "Info", "Warning", "Error" };
}

QString QmlLogBridge::logsDir() const { return Logger::instance().logsDir(); }

void QmlLogBridge::start() { poll(); m_pollTimer.start(); }
void QmlLogBridge::stop() { m_pollTimer.stop(); }

void QmlLogBridge::poll()
{
    QFile f(Logger::instance().currentLogPath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    const qint64 size = f.size();
    if (size == m_lastSize) return;
    if (size < m_lastSize) { m_text.clear(); m_lastSize = 0; }
    f.seek(m_lastSize);
    m_text += QString::fromUtf8(f.readAll());
    m_lastSize = size;
    int nl = m_text.count('\n');
    if (nl > 20000) {
        int drop = m_text.indexOf('\n', 0);
        while (nl-- > 20000 && drop >= 0) drop = m_text.indexOf('\n', drop + 1);
        if (drop > 0) m_text = m_text.mid(drop + 1);
    }
    emit textChanged();
}

void QmlLogBridge::clearLog()
{
    Logger::instance().clear();
    m_text.clear();
    m_lastSize = 0;
    emit textChanged();
}

void QmlLogBridge::openLogsFolder()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(Logger::instance().logsDir()));
}

bool QmlLogBridge::exportLogs(const QString &filePath)
{
    QString path = filePath;
    if (path.startsWith("file://")) path = QUrl(path).toLocalFile();
    QFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    out.write(Logger::instance().readAllLogs().toUtf8());
    out.close();
    return true;
}

bool QmlLogBridge::previousSessionCrashed() const
{
    return Logger::instance().previousSessionCrashed();
}

QString QmlLogBridge::crashReportUrl() const
{
    const QString version = QCoreApplication::applicationVersion();
    const QString crashDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                             + QStringLiteral("/crashes");
    QString backtrace = CrashHandler::lastReport(crashDir);
    if (backtrace.isEmpty()) backtrace = QStringLiteral("(no backtrace captured)");
    const QString body = QStringLiteral(
        "**Version:** %1\n**OS:** %2\n\n"
        "**What were you doing when it happened?**\n\n(describe here)\n\n"
        "<details><summary>Crash backtrace (auto-captured)</summary>\n\n"
        "```\n%3\n```\n</details>\n\n"
        "<details><summary>Log tail (auto-captured from the previous run)</summary>\n\n"
        "```\n%4\n```\n</details>\n")
        .arg(version, QSysInfo::prettyProductName(), backtrace, Logger::instance().crashTail());
    QUrl url(QStringLiteral("https://github.com/BATorrent-app/BATorrent/issues/new"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("title"),
                   QStringLiteral("[crash] BATorrent %1 ended unexpectedly").arg(version));
    q.addQueryItem(QStringLiteral("labels"), QStringLiteral("bug"));
    q.addQueryItem(QStringLiteral("body"), body);
    url.setQuery(q);
    return url.toString(QUrl::FullyEncoded);
}

QString QmlLogBridge::defaultExportName() const
{
    const QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    return desktop + "/batorrent-logs-"
        + QDateTime::currentDateTime().toString("yyyy-MM-dd-HHmmss") + ".txt";
}
