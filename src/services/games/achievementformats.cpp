// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/games/achievementformats.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

namespace AchievementFormats {

namespace {

bool truthy(const QString &v)
{
    const QString s = v.trimmed().toLower();
    return s == QLatin1String("1") || s == QLatin1String("true");
}

qint64 number(const QString &v)
{
    bool ok = false;
    const qint64 n = v.trimmed().toLongLong(&ok);
    return ok && n > 0 ? normalizeTime(n) : 0;
}

// 3DM and RLD! store numbers as the hex of a little-endian uint32 ("0A000000" = 10).
qint64 hexLe32(const QString &v)
{
    const QByteArray raw = QByteArray::fromHex(v.trimmed().toLatin1());
    if (raw.size() < 4) return 0;
    const auto b = reinterpret_cast<const unsigned char *>(raw.constData());
    return qint64(quint32(b[0]) | quint32(b[1]) << 8 | quint32(b[2]) << 16 | quint32(b[3]) << 24);
}

QString value(const QHash<QString, QString> &kv, std::initializer_list<const char *> keys)
{
    for (const char *k : keys) {
        for (auto it = kv.cbegin(); it != kv.cend(); ++it)
            if (it.key().compare(QLatin1String(k), Qt::CaseInsensitive) == 0) return it.value();
    }
    return {};
}

Unlocks goldberg(const QByteArray &bytes)
{
    Unlocks out;
    const QJsonDocument doc = QJsonDocument::fromJson(bytes);
    const auto take = [&out](const QString &id, const QJsonObject &o) {
        const QJsonValue earned = o.value(QLatin1String("earned"));
        if (id.isEmpty() || !(earned.isBool() ? earned.toBool() : earned.toInt() == 1)) return;
        out.insert(id, normalizeTime(qint64(o.value(QLatin1String("earned_time")).toDouble())));
    };
    if (doc.isArray()) {
        for (const QJsonValue &v : doc.array())
            take(v.toObject().value(QLatin1String("name")).toString(), v.toObject());
    } else {
        const QJsonObject root = doc.object();
        for (auto it = root.begin(); it != root.end(); ++it) take(it.key(), it.value().toObject());
    }
    return out;
}

Unlocks codex(const Ini &ini)
{
    Unlocks out;
    for (auto s = ini.cbegin(); s != ini.cend(); ++s) {
        const auto &kv = s.value();
        bool got = truthy(value(kv, { "Achieved", "HaveAchieved", "Unlocked" }));
        // A progress achievement can sit at its maximum without the flag (CODEX Gears 5).
        const qint64 cur = value(kv, { "CurProgress" }).toLongLong();
        const qint64 max = value(kv, { "MaxProgress" }).toLongLong();
        if (!got && cur > 0 && cur == max) got = true;
        if (got) out.insert(s.key(), number(value(kv, { "UnlockTime", "HaveAchievedTime", "Time" })));
    }
    return out;
}

Unlocks onlineFix(const Ini &ini)
{
    Unlocks out;
    for (auto s = ini.cbegin(); s != ini.cend(); ++s) {
        if (truthy(value(s.value(), { "achieved" })))
            out.insert(s.key(), number(value(s.value(), { "timestamp", "TimeUnlocked" })));
    }
    return out;
}

Unlocks skidrow(const Ini &ini)
{
    Unlocks out;
    const auto sec = ini.constFind(QStringLiteral("Achievements"));
    if (sec == ini.cend()) return out;
    for (auto it = sec->cbegin(); it != sec->cend(); ++it) {
        const QStringList f = it.value().split(QLatin1Char('@'));
        if (f.value(0).trimmed() == QLatin1String("1")) out.insert(it.key(), number(f.last()));
    }
    return out;
}

Unlocks threeDm(const Ini &ini)
{
    Unlocks out;
    const auto state = ini.value(QStringLiteral("State"));
    const auto time = ini.value(QStringLiteral("Time"));
    for (auto it = state.cbegin(); it != state.cend(); ++it)
        if (it.value().trimmed() == QLatin1String("0101"))
            out.insert(it.key(), normalizeTime(hexLe32(time.value(it.key()))));
    return out;
}

Unlocks rld(const Ini &ini)
{
    Unlocks out;
    for (auto s = ini.cbegin(); s != ini.cend(); ++s) {
        if (s.key().compare(QLatin1String("Steam"), Qt::CaseInsensitive) == 0) continue;
        if (hexLe32(value(s.value(), { "State" })) == 1)
            out.insert(s.key(), normalizeTime(hexLe32(value(s.value(), { "Time" }))));
    }
    return out;
}

Unlocks creamApi(const Ini &ini)
{
    Unlocks out;
    for (auto s = ini.cbegin(); s != ini.cend(); ++s)
        if (truthy(value(s.value(), { "achieved" })))
            out.insert(s.key(), number(value(s.value(), { "unlocktime" })));
    return out;
}

void keep(Progress &out, const QString &id, qint64 cur, qint64 max)
{
    if (!id.isEmpty() && max > 0 && cur >= 0) out.insert(id, { qMin(cur, max), max });
}

Progress goldbergProgress(const QByteArray &bytes)
{
    Progress out;
    const QJsonDocument doc = QJsonDocument::fromJson(bytes);
    const auto take = [&out](const QString &id, const QJsonObject &o) {
        keep(out, id, qint64(o.value(QLatin1String("progress")).toDouble()),
             qint64(o.value(QLatin1String("max_progress")).toDouble()));
    };
    if (doc.isArray()) {
        for (const QJsonValue &v : doc.array())
            take(v.toObject().value(QLatin1String("name")).toString(), v.toObject());
    } else {
        const QJsonObject root = doc.object();
        for (auto it = root.begin(); it != root.end(); ++it) take(it.key(), it.value().toObject());
    }
    return out;
}

Progress codexProgress(const Ini &ini)
{
    Progress out;
    for (auto s = ini.cbegin(); s != ini.cend(); ++s)
        keep(out, s.key(), value(s.value(), { "CurProgress" }).toLongLong(),
             value(s.value(), { "MaxProgress" }).toLongLong());
    return out;
}

Progress rldProgress(const Ini &ini)
{
    Progress out;
    for (auto s = ini.cbegin(); s != ini.cend(); ++s) {
        if (s.key().compare(QLatin1String("Steam"), Qt::CaseInsensitive) == 0) continue;
        keep(out, s.key(), hexLe32(value(s.value(), { "CurProgress" })), hexLe32(value(s.value(), { "MaxProgress" })));
    }
    return out;
}

} // namespace

Progress parseProgress(Format format, const QByteArray &bytes)
{
    switch (format) {
    case Format::GoldbergJson: return goldbergProgress(bytes);
    case Format::CodexIni:     return codexProgress(parseIni(bytes));
    case Format::RldIni:       return rldProgress(parseIni(bytes));
    default:                   return {};
    }
}

qint64 normalizeTime(qint64 t)
{
    if (t <= 0) return 0;
    return t > 100000000000LL ? t / 1000 : t;
}

Ini parseIni(const QByteArray &bytes)
{
    Ini out;
    QString text = QString::fromUtf8(bytes);
    if (text.startsWith(QChar(0xFEFF))) text.remove(0, 1);
    QString section;
    for (const QString &raw : text.split(QLatin1Char('\n'))) {
        const QString line = raw.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char(';')) || line.startsWith(QLatin1Char('#'))) continue;
        if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']'))) {
            section = line.mid(1, line.size() - 2).trimmed();
            continue;
        }
        const int eq = line.indexOf(QLatin1Char('='));
        if (eq <= 0) continue;
        out[section].insert(line.left(eq).trimmed(), line.mid(eq + 1).trimmed());
    }
    return out;
}

Unlocks parse(Format format, const QByteArray &bytes)
{
    switch (format) {
    case Format::GoldbergJson: return goldberg(bytes);
    case Format::CodexIni:     return codex(parseIni(bytes));
    case Format::OnlineFixIni: return onlineFix(parseIni(bytes));
    case Format::SkidrowIni:   return skidrow(parseIni(bytes));
    case Format::ThreeDmIni:   return threeDm(parseIni(bytes));
    case Format::RldIni:       return rld(parseIni(bytes));
    case Format::CreamApiIni:  return creamApi(parseIni(bytes));
    }
    return {};
}

} // namespace AchievementFormats
