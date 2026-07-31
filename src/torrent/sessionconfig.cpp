// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "torrent/sessionconfig.h"
#include "torrent/types.h"

#include <libtorrent/settings_pack.hpp>
#include <QRegularExpression>
#include <QSettings>
#include <QVariant>

namespace SessionConfig {

QString listenInterfaces(const QString &listenAddr, int port, bool forceIpv4)
{
    if (listenAddr != QLatin1String("0.0.0.0") || forceIpv4)
        return QStringLiteral("%1:%2").arg(listenAddr).arg(port);
    return QStringLiteral("0.0.0.0:%1,[::]:%1").arg(port);
}

static QString shellQuote(const QString &s)
{
    QString q = s;
    q.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QLatin1Char('\'') + q + QLatin1Char('\'');
}

QString expandOnCompleteCommand(const QString &tmpl,
                                const QString &name,
                                const QString &savePath,
                                const QString &hash,
                                qint64 totalSize)
{
    QString cmd = tmpl;
    cmd.replace(QLatin1String("%N"), shellQuote(name));
    cmd.replace(QLatin1String("%D"), shellQuote(savePath));
    cmd.replace(QLatin1String("%H"), shellQuote(hash));
    cmd.replace(QLatin1String("%Z"), shellQuote(QString::number(totalSize)));
    cmd.replace(QLatin1String("%F"), shellQuote(savePath + QLatin1Char('/') + name));
    return cmd;
}

QStringList parseExtractPasswords(const QString &raw)
{
    QStringList pw;
    const auto parts = raw.split(QRegularExpression(QStringLiteral("[;\\n]")),
                                 Qt::SkipEmptyParts);
    for (const QString &p : parts) {
        const QString t = p.trimmed();
        if (!t.isEmpty())
            pw << t;
    }
    return pw;
}

qint64 autoCompleteSecondsFromIndex(int index)
{
    static const qint64 days[] = {0, 1, 3, 7, 14, 30};
    if (index < 0 || index >= 6)
        return 0;
    return days[index] * 86400;
}

int portStatusCode(bool listenOk, bool portmapOk)
{
    if (!listenOk)
        return 3;
    if (portmapOk)
        return 1;
    return 2;
}

std::map<int, std::string> planContentLayout(int mode,
                                             const std::string &torrentName,
                                             const std::vector<std::string> &filePaths)
{
    std::map<int, std::string> out;
    const int numFiles = static_cast<int>(filePaths.size());
    if (numFiles == 0 || mode == 0)
        return out;

    if (mode == 2) {
        const std::string &root = filePaths[0];
        const auto slash = root.find('/');
        if (slash == std::string::npos || numFiles <= 1)
            return out;
        const std::string prefix = root.substr(0, slash + 1);
        for (const auto &p : filePaths) {
            if (p.compare(0, prefix.size(), prefix) != 0)
                return {};
        }
        for (int i = 0; i < numFiles; ++i)
            out[i] = filePaths[static_cast<size_t>(i)].substr(prefix.size());
        return out;
    }

    if (mode == 1 && numFiles == 1) {
        const std::string &filePath = filePaths[0];
        if (filePath.find('/') == std::string::npos)
            out[0] = torrentName + "/" + filePath;
    }
    return out;
}


AdvancedSettings loadAdvanced(const QSettings &s)
{
    AdvancedSettings a;
    a.aioThreads = s.value(QStringLiteral("adv/aioThreads"), 10).toInt();
    a.hashingThreads = s.value(QStringLiteral("adv/hashingThreads"), 2).toInt();
    a.filePoolSize = s.value(QStringLiteral("adv/filePoolSize"), 100).toInt();
    a.checkingMemUsage = s.value(QStringLiteral("adv/checkingMemUsage"), 512).toInt();
    a.diskIOReadMode = s.value(QStringLiteral("adv/diskIOReadMode"), 0).toInt();
    a.diskIOWriteMode = s.value(QStringLiteral("adv/diskIOWriteMode"), 0).toInt();
    a.connectionsLimit = s.value(QStringLiteral("adv/connectionsLimit"), 500).toInt();
    a.connectionSpeed = s.value(QStringLiteral("adv/connectionSpeed"), 30).toInt();
    a.maxUploadsPerTorrent = s.value(QStringLiteral("adv/maxUploadsPerTorrent"), 4).toInt();
    a.maxConnectionsPerTorrent = s.value(QStringLiteral("adv/maxConnectionsPerTorrent"), 100).toInt();
    a.unchokeSlotsLimit = s.value(QStringLiteral("adv/unchokeSlotsLimit"), 20).toInt();
    a.chokingAlgorithm = s.value(QStringLiteral("adv/chokingAlgorithm"), 0).toInt();
    a.seedChokingAlgorithm = s.value(QStringLiteral("adv/seedChokingAlgorithm"), 0).toInt();
    a.sendBufferWatermark = s.value(QStringLiteral("adv/sendBufferWatermark"), 500).toInt();
    a.outgoingPortMin = s.value(QStringLiteral("adv/outgoingPortMin"), 0).toInt();
    a.outgoingPortMax = s.value(QStringLiteral("adv/outgoingPortMax"), 0).toInt();
    a.rateLimitIpOverhead = s.value(QStringLiteral("adv/rateLimitIpOverhead"), false).toBool();
    a.ignoreLimitsOnLAN = s.value(QStringLiteral("adv/ignoreLimitsOnLAN"), true).toBool();
    return a;
}

void persistAdvanced(QSettings &s, const AdvancedSettings &a)
{
    s.setValue(QStringLiteral("adv/aioThreads"), a.aioThreads);
    s.setValue(QStringLiteral("adv/hashingThreads"), a.hashingThreads);
    s.setValue(QStringLiteral("adv/filePoolSize"), a.filePoolSize);
    s.setValue(QStringLiteral("adv/checkingMemUsage"), a.checkingMemUsage);
    s.setValue(QStringLiteral("adv/diskIOReadMode"), a.diskIOReadMode);
    s.setValue(QStringLiteral("adv/diskIOWriteMode"), a.diskIOWriteMode);
    s.setValue(QStringLiteral("adv/connectionsLimit"), a.connectionsLimit);
    s.setValue(QStringLiteral("adv/connectionSpeed"), a.connectionSpeed);
    s.setValue(QStringLiteral("adv/maxUploadsPerTorrent"), a.maxUploadsPerTorrent);
    s.setValue(QStringLiteral("adv/maxConnectionsPerTorrent"), a.maxConnectionsPerTorrent);
    s.setValue(QStringLiteral("adv/unchokeSlotsLimit"), a.unchokeSlotsLimit);
    s.setValue(QStringLiteral("adv/chokingAlgorithm"), a.chokingAlgorithm);
    s.setValue(QStringLiteral("adv/seedChokingAlgorithm"), a.seedChokingAlgorithm);
    s.setValue(QStringLiteral("adv/sendBufferWatermark"), a.sendBufferWatermark);
    s.setValue(QStringLiteral("adv/outgoingPortMin"), a.outgoingPortMin);
    s.setValue(QStringLiteral("adv/outgoingPortMax"), a.outgoingPortMax);
    s.setValue(QStringLiteral("adv/rateLimitIpOverhead"), a.rateLimitIpOverhead);
    s.setValue(QStringLiteral("adv/ignoreLimitsOnLAN"), a.ignoreLimitsOnLAN);
}

void fillAdvancedPack(libtorrent::settings_pack &pack, const AdvancedSettings &a)
{
    pack.set_int(libtorrent::settings_pack::aio_threads, a.aioThreads);
    pack.set_int(libtorrent::settings_pack::hashing_threads, a.hashingThreads);
    pack.set_int(libtorrent::settings_pack::file_pool_size, a.filePoolSize);
    pack.set_int(libtorrent::settings_pack::checking_mem_usage, a.checkingMemUsage);
    pack.set_int(libtorrent::settings_pack::disk_io_read_mode, a.diskIOReadMode);
    pack.set_int(libtorrent::settings_pack::disk_io_write_mode, a.diskIOWriteMode);
    pack.set_int(libtorrent::settings_pack::connections_limit, a.connectionsLimit);
    pack.set_int(libtorrent::settings_pack::connection_speed, a.connectionSpeed);
    pack.set_int(libtorrent::settings_pack::unchoke_slots_limit, a.unchokeSlotsLimit);
    pack.set_int(libtorrent::settings_pack::choking_algorithm, a.chokingAlgorithm);
    pack.set_int(libtorrent::settings_pack::seed_choking_algorithm, a.seedChokingAlgorithm);
    pack.set_int(libtorrent::settings_pack::send_buffer_watermark, a.sendBufferWatermark * 1024);
    pack.set_bool(libtorrent::settings_pack::rate_limit_ip_overhead, a.rateLimitIpOverhead);
    if (a.outgoingPortMin > 0 && a.outgoingPortMax >= a.outgoingPortMin) {
        pack.set_int(libtorrent::settings_pack::outgoing_port, a.outgoingPortMin);
        pack.set_int(libtorrent::settings_pack::num_outgoing_ports,
                     a.outgoingPortMax - a.outgoingPortMin + 1);
    }
    (void)a.ignoreLimitsOnLAN; // peer classes, not settings_pack
}

bool patchAdvancedKey(AdvancedSettings &a, const QString &key, const QVariant &v)
{
    if (key == QLatin1String("advAioThreads"))          a.aioThreads = v.toInt();
    else if (key == QLatin1String("advHashingThreads")) a.hashingThreads = v.toInt();
    else if (key == QLatin1String("advFilePool"))       a.filePoolSize = v.toInt();
    else if (key == QLatin1String("advCheckingMem"))    a.checkingMemUsage = v.toInt();
    else if (key == QLatin1String("advSendBuffer"))     a.sendBufferWatermark = v.toInt();
    else if (key == QLatin1String("advConnLimit"))      a.connectionsLimit = v.toInt();
    else if (key == QLatin1String("advConnSpeed"))      a.connectionSpeed = v.toInt();
    else if (key == QLatin1String("advUnchokeSlots"))   a.unchokeSlotsLimit = v.toInt();
    else if (key == QLatin1String("advMaxUploadsTor"))  a.maxUploadsPerTorrent = v.toInt();
    else if (key == QLatin1String("advMaxConnsTor"))    a.maxConnectionsPerTorrent = v.toInt();
    else if (key == QLatin1String("advChokingAlgo"))    a.chokingAlgorithm = v.toInt() == 1 ? 2 : 0;
    else if (key == QLatin1String("advSeedChoking"))    a.seedChokingAlgorithm = v.toInt();
    else if (key == QLatin1String("advRateOverhead"))   a.rateLimitIpOverhead = v.toBool();
    else if (key == QLatin1String("advIgnoreLan"))      a.ignoreLimitsOnLAN = v.toBool();
    else return false;
    return true;
}

std::vector<int> excludedFileIndexes(const QStringList &patterns,
                                     const QStringList &filePaths)
{
    std::vector<int> out;
    if (patterns.isEmpty() || filePaths.isEmpty())
        return out;

    QList<QRegularExpression> regexes;
    for (const QString &p : patterns) {
        const QString trimmed = p.trimmed();
        if (trimmed.isEmpty())
            continue;
        QRegularExpression re(trimmed, QRegularExpression::CaseInsensitiveOption);
        if (re.isValid())
            regexes.append(re);
    }
    if (regexes.isEmpty())
        return out;

    out.reserve(static_cast<size_t>(filePaths.size()));
    for (int i = 0; i < filePaths.size(); ++i) {
        for (const auto &re : regexes) {
            if (re.match(filePaths.at(i)).hasMatch()) {
                out.push_back(i);
                break;
            }
        }
    }
    return out;
}

} // namespace SessionConfig
