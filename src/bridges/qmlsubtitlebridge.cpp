// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/qmlsubtitlebridge.h"

#include "services/metadata/metadataresolver.h"
#include "services/platform/contentlanguage.h"
#include "services/subtitles/subtitlesearch.h"
#include "torrent/iengine.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

QmlSubtitleBridge::QmlSubtitleBridge(IEngine *session, QObject *parent)
    : QObject(parent), m_session(session), m_search(new SubtitleSearch(this))
{
    connect(m_search, &SubtitleSearch::resultsChanged, this, &QmlSubtitleBridge::resultsChanged);
    connect(m_search, &SubtitleSearch::searchFinished, this, [this]() {
        m_searching = false;
        emit searchingChanged();
    });
    connect(m_search, &SubtitleSearch::downloadFinished, this, &QmlSubtitleBridge::subtitleReady);
    connect(m_search, &SubtitleSearch::errorOccurred, this, &QmlSubtitleBridge::searchError);
}

bool QmlSubtitleBridge::hasOpenSubtitlesKey() const
{
    bool has = !QSettings("BATorrent", "BATorrent").value("osApiKey").toString().trimmed().isEmpty();
#ifdef BAT_OS_KEY
    has = true;
#endif
    return has;
}

QVariantList QmlSubtitleBridge::results() const
{
    QVariantList out;
    const auto rs = m_search->results();
    out.reserve(rs.size());
    for (const auto &r : rs) {
        QVariantMap m;
        m["provider"] = r.provider;
        m["name"] = r.name;
        m["language"] = r.language;
        out << m;
    }
    return out;
}

void QmlSubtitleBridge::searchFor(const QString &infoHash, int fileIndex, const QString &mediaTitle,
                                  const QStringList &langs)
{
    QString video = m_session->streamFilePath(m_session->torrentIndexByInfoHash(infoHash), fileIndex);
    if (video.endsWith(QLatin1String(".!bt"))) video.chop(4);
    const QFileInfo vi(video);
    if (video.isEmpty()) {
        m_targetDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        m_baseName = mediaTitle;
    } else {
        m_targetDir = vi.dir().absolutePath();
        m_baseName = vi.completeBaseName();
    }
    // the filename carries S/E + release tags the parser feeds on; the resolved
    // display title doesn't
    const QString queryName = vi.fileName().isEmpty() ? mediaTitle : vi.fileName();
    QStringList useLangs = langs;
    if (useLangs.isEmpty()) {
        useLangs << ContentLanguage::subtitleCode();
        if (!useLangs.contains(QStringLiteral("en"))) useLangs << QStringLiteral("en");
    }
    int tmdbId = 0;
    if (m_resolver && m_resolver->hasCached(infoHash)) {
        const auto meta = m_resolver->cached(infoHash);
        if (meta.valid) tmdbId = meta.tmdbId;
    }
    m_searching = true;
    emit searchingChanged();
    m_search->search(queryName, useLangs, tmdbId);
}

void QmlSubtitleBridge::download(int index)
{
    if (m_targetDir.isEmpty()) return;
    m_search->download(index, m_targetDir, m_baseName);
}
