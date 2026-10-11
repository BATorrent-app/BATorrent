// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/security/mediaguard.h"
#include "torrent/iengine.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QSettings>

namespace {

constexpr char kSettingsKey[] = "quarantinedMedia";
const QString kQuarantineSuffix = QStringLiteral(".quarantine");

QString keyFor(const QString &infoHash, int fileIndex)
{
    return infoHash.toLower() + QLatin1Char(':') + QString::number(fileIndex);
}

}

MediaGuard::MediaGuard(IEngine *engine, QObject *parent)
    : QObject(parent), m_engine(engine)
{
    const QVariantMap saved = QSettings("BATorrent", "BATorrent").value(kSettingsKey).toMap();
    for (auto it = saved.cbegin(); it != saved.cend(); ++it)
        m_quarantined.insert(it.key(), it.value().toString());
    connect(m_engine, &IEngine::torrentFinished, this, &MediaGuard::onTorrentFinished);
}

bool MediaGuard::isQuarantined(const QString &infoHash, int fileIndex) const
{
    return m_quarantined.contains(keyFor(infoHash, fileIndex));
}

int MediaGuard::quarantinedFileOf(const QString &infoHash) const
{
    const QString prefix = infoHash.toLower() + QLatin1Char(':');
    for (auto it = m_quarantined.cbegin(); it != m_quarantined.cend(); ++it)
        if (it.key().startsWith(prefix)) return it.key().mid(prefix.size()).toInt();
    return -1;
}

void MediaGuard::remind(const QString &infoHash, int fileIndex)
{
    const auto it = m_quarantined.constFind(keyFor(infoHash, fileIndex));
    const int t = m_engine->torrentIndexByInfoHash(infoHash);
    if (it == m_quarantined.cend() || t < 0) return;
    emit quarantined(infoHash, fileIndex, leafName(t, fileIndex), it.value());
}

QString MediaGuard::leafName(int torrentIndex, int fileIndex) const
{
    const auto files = m_engine->filesAt(torrentIndex);
    if (fileIndex < 0 || fileIndex >= int(files.size())) return {};
    QString name = QFileInfo(files[fileIndex].path).fileName();
    for (const QString &suffix : { QStringLiteral(".!bt"), kQuarantineSuffix })
        if (name.endsWith(suffix)) name.chop(suffix.size());
    return name;
}

MediaGuard::Gate MediaGuard::admit(const QString &infoHash, int fileIndex)
{
    const QString key = keyFor(infoHash, fileIndex);
    if (m_quarantined.contains(key)) return Gate::Block;
    if (m_cleared.contains(key)) return Gate::Allow;

    const int t = m_engine->torrentIndexByInfoHash(infoHash);
    if (t < 0) return Gate::Allow;
    const QString path = m_engine->streamFilePath(t, fileIndex);
    if (path.isEmpty() || !MediaSniff::isVideoName(path)) return Gate::Allow;

    const qint64 size = m_engine->streamFileSize(t, fileIndex);
    const qint64 want = MediaSniff::headWanted(size);
    if (want <= 0 || m_engine->streamContiguousAvailableBytes(t, fileIndex, 0, want) < want)
        return Gate::Wait;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return Gate::Wait;
    const QByteArray head = f.read(want);

    switch (MediaSniff::judge(head, size)) {
    case MediaSniff::Verdict::NeedMore:
        return Gate::Wait;
    case MediaSniff::Verdict::Disguised:
        quarantine(t, fileIndex, key, MediaSniff::identify(head));
        return Gate::Block;
    case MediaSniff::Verdict::Lure:
        m_cleared.insert(key);
        qWarning() << "[guard] licence/codec lure in" << path;
        emit lureFound(infoHash, fileIndex, leafName(t, fileIndex));
        return Gate::Allow;
    case MediaSniff::Verdict::Ok:
        m_cleared.insert(key);
        return Gate::Allow;
    }
    return Gate::Allow;
}

void MediaGuard::onTorrentFinished(const QString &, const QString &infoHash)
{
    const int t = m_engine->torrentIndexByInfoHash(infoHash);
    if (t < 0) return;
    const auto files = m_engine->filesAt(t);
    for (int i = 0; i < int(files.size()); ++i)
        if (files[i].priority > 0 && MediaSniff::isVideoName(files[i].path)) admit(infoHash, i);
}

void MediaGuard::quarantine(int torrentIndex, int fileIndex, const QString &key, MediaSniff::Kind kind)
{
    const auto files = m_engine->filesAt(torrentIndex);
    if (fileIndex < 0 || fileIndex >= int(files.size())) return;
    QString rel = files[fileIndex].path;
    if (rel.endsWith(QLatin1String(".!bt"))) rel.chop(4);

    const QString kindKey = MediaSniff::kindKey(kind);
    m_quarantined.insert(key, kindKey);
    QVariantMap saved;
    for (auto it = m_quarantined.cbegin(); it != m_quarantined.cend(); ++it) saved.insert(it.key(), it.value());
    QSettings("BATorrent", "BATorrent").setValue(kSettingsKey, saved);

    m_engine->pauseTorrent(torrentIndex);
    m_engine->setFilePriority(torrentIndex, fileIndex, 0);
    m_engine->renameFile(torrentIndex, fileIndex, rel + kQuarantineSuffix);

    const QString hash = m_engine->torrentHashAt(torrentIndex);
    qWarning() << "[guard] quarantined" << rel << "as" << kindKey;
    emit quarantined(hash, fileIndex, QFileInfo(rel).fileName(), kindKey);
}
