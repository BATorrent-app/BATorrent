// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/platform/statshistory.h"

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>

namespace {
QString todayKey() { return QDate::currentDate().toString(Qt::ISODate); }
constexpr qint64 kFlushIntervalSec = 60;
}

StatsHistory::StatsHistory(const QString &filePath)
    : m_path(filePath)
{
    load();
    // first run: materialize the file right away so a missing file always
    // means "collector not wired", never "no traffic yet"
    if (!QFileInfo::exists(m_path)) {
        m_dirty = true;
        flush();
    }
}

StatsHistory::~StatsHistory()
{
    flush();
}

void StatsHistory::load()
{
    QFile f(m_path);
    if (!f.open(QIODevice::ReadOnly)) return;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (doc.isObject()) m_days = doc.object().value(QStringLiteral("days")).toObject();
}

void StatsHistory::bump(const QString &key, qint64 delta)
{
    if (delta <= 0) return;
    const QString day = todayKey();
    QJsonObject entry = m_days.value(day).toObject();
    entry[key] = entry.value(key).toDouble() + double(delta);
    m_days[day] = entry;
    m_dirty = true;
}

void StatsHistory::recordTransfer(qint64 downAbs, qint64 upAbs)
{
    // first sample of a session only seeds the baseline
    if (m_lastDown >= 0) {
        bump(QStringLiteral("down"), downAbs - m_lastDown);
        bump(QStringLiteral("up"), upAbs - m_lastUp);
    }
    m_lastDown = downAbs;
    m_lastUp = upAbs;
    maybeFlush();
}

void StatsHistory::recordAdded()
{
    bump(QStringLiteral("added"), 1);
}

void StatsHistory::recordCompleted(const QString &category)
{
    bump(QStringLiteral("completed"), 1);
    const QString day = todayKey();
    QJsonObject entry = m_days.value(day).toObject();
    QJsonObject cats = entry.value(QStringLiteral("cats")).toObject();
    const QString key = category.isEmpty() ? QStringLiteral("uncategorized") : category;
    cats[key] = cats.value(key).toInt() + 1;
    entry[QStringLiteral("cats")] = cats;
    m_days[day] = entry;
    m_dirty = true;
}

void StatsHistory::maybeFlush()
{
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    if (now - m_lastFlushAt >= kFlushIntervalSec) flush();
}

void StatsHistory::flush()
{
    if (!m_dirty) return;
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile f(m_path);
    if (!f.open(QIODevice::WriteOnly)) return;
    QJsonObject root;
    root[QStringLiteral("v")] = 1;
    root[QStringLiteral("days")] = m_days;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    if (f.commit()) {
        m_dirty = false;
        m_lastFlushAt = QDateTime::currentSecsSinceEpoch();
    }
}
