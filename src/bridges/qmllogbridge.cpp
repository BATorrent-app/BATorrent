// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/qmllogbridge.h"

#include "services/platform/logger.h"
#include "services/security/crashhandler.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QRegularExpression>
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
    if (size < m_lastSize) { m_text.clear(); m_lastSize = 0; m_entries.clear(); m_partial.clear(); }
    f.seek(m_lastSize);
    const QString chunk = QString::fromUtf8(f.readAll());
    m_text += chunk;
    m_lastSize = size;
    parse(chunk);
    emit entriesChanged();
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
    m_entries.clear();
    m_partial.clear();
    emit textChanged();
    emit entriesChanged();
}

// "2026-10-15T11:45:23.998Z [WARN] [session] text"; a line without the
// stamp continues the one before it (Qt's binding-loop warnings take two).
// Only the running session is kept: "--- log opened ---" starts over.
void QmlLogBridge::parse(const QString &chunk)
{
    static const QRegularExpression head(QStringLiteral(
        "^(\\d{4}-\\d\\d-\\d\\dT\\d\\d:\\d\\d:\\d\\d(?:\\.\\d+)?Z) \\[(\\w+)\\] ?(.*)$"));
    static const QRegularExpression mod(QStringLiteral("^\\[([^\\]]{1,24})\\] ?(.*)$"));
    static const QRegularExpression qmlPath(QStringLiteral("^(?:file:///|qrc:/)\\S*?([^/\\s]+\\.qml)(:\\d+)?(?::\\d+)?:? ?(.*)$"));
    QString text = m_partial + chunk;
    const int end = text.lastIndexOf(QLatin1Char('\n'));
    m_partial = end < 0 ? text : text.mid(end + 1);
    if (end < 0) return;
    const QStringList lines = text.left(end).split(QLatin1Char('\n'));
    for (QString line : lines) {
        if (line.endsWith(QLatin1Char('\r'))) line.chop(1);
        if (line.isEmpty()) continue;
        const auto m = head.match(line);
        if (!m.hasMatch()) {
            if (!m_entries.isEmpty()) { m_entries.last().msg += QLatin1Char('\n') + line; m_entries.last().raw += QLatin1Char('\n') + line; }
            continue;
        }
        QString rest = m.captured(3);
        if (rest.contains(QLatin1String("--- log opened ---"))) {
            m_entries.clear();
            m_sessionLine.clear();
        }
        Entry e;
        e.raw = line;
        const QDateTime at = QDateTime::fromString(m.captured(1), Qt::ISODateWithMs);
        e.time = at.isValid() ? at.toLocalTime().toString(QStringLiteral("HH:mm:ss")) : m.captured(1).mid(11, 8);
        const QString lv = m.captured(2).toUpper();
        e.level = (lv == QLatin1String("ERROR") || lv == QLatin1String("CRITICAL") || lv == QLatin1String("FATAL")) ? 3
                : lv.startsWith(QLatin1String("WARN")) ? 2
                : lv == QLatin1String("INFO") ? 1 : 0;
        const auto mm = mod.match(rest);
        if (mm.hasMatch()) { e.module = mm.captured(1); rest = mm.captured(2); }
        const auto qm = qmlPath.match(rest);
        if (qm.hasMatch()) { e.module = QStringLiteral("qml"); rest = qm.captured(1) + qm.captured(2) + QStringLiteral(" \u2014 ") + qm.captured(3); }
        if (e.module == QLatin1String("qml")) {
            const auto again = qmlPath.match(rest);
            if (again.hasMatch()) rest = again.captured(1) + again.captured(2) + QStringLiteral(" \u2014 ") + again.captured(3);
        }
        e.msg = rest;
        if (e.module == QLatin1String("versions") && m_sessionLine.isEmpty())
            m_sessionLine = e.time.left(5) + QStringLiteral("\t") + rest;
        m_entries.push_back(e);
    }
    if (m_entries.size() > 20000) m_entries.remove(0, m_entries.size() - 20000);
}

QVariantList QmlLogBridge::counts() const
{
    int by[4] = {0, 0, 0, 0};
    for (const Entry &e : m_entries) ++by[qBound(0, e.level, 3)];
    return { by[3], by[3] + by[2], by[3] + by[2] + by[1], by[3] + by[2] + by[1] + by[0] };
}

QVariantList QmlLogBridge::entries(int minLevel, const QString &filter, int max) const
{
    QVariantList out;
    const QString f = filter.trimmed();
    for (int i = m_entries.size() - 1; i >= 0 && out.size() < max; --i) {
        const Entry &e = m_entries[i];
        if (e.level < minLevel) continue;
        if (!f.isEmpty() && !e.module.contains(f, Qt::CaseInsensitive) && !e.msg.contains(f, Qt::CaseInsensitive)) continue;
        QVariantMap m;
        m[QStringLiteral("time")] = e.time;
        m[QStringLiteral("level")] = e.level;
        m[QStringLiteral("module")] = e.module;
        m[QStringLiteral("msg")] = e.msg;
        out.prepend(m);
    }
    return out;
}

int QmlLogBridge::copyForReport(int lines) const
{
    const int from = qMax(0, int(m_entries.size()) - lines);
    QStringList out;
    const QString home = QDir::homePath();
    const QString homeNative = QDir::toNativeSeparators(home);
    const QString user = QDir(home).dirName();
    for (int i = from; i < m_entries.size(); ++i) {
        QString l = m_entries[i].raw;
        l.replace(home, QStringLiteral("~"), Qt::CaseInsensitive);
        l.replace(homeNative, QStringLiteral("~"), Qt::CaseInsensitive);
        if (user.size() > 2) l.replace(user, QStringLiteral("<user>"), Qt::CaseInsensitive);
        out << l;
    }
    if (auto *cb = QGuiApplication::clipboard()) cb->setText(out.join(QLatin1Char('\n')));
    return out.size();
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
