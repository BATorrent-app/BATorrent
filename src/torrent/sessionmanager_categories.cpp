// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// SessionManager — categories & tags slice. Category assignment (with the
// category-save-path / temp-path move integration) and per-torrent tags. Split
// out of sessionmanager.cpp verbatim; no behaviour change.

#include "torrent/sessionmanager.h"

#include <QSettings>
#include <QDir>
#include <QSet>
#include <QStringList>
#include <QMap>
#include <algorithm>

void SessionManager::setTorrentCategory(int index, const QString &category)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    QString hash = torrentHash(index);
    if (hash.isEmpty())
        return;

    if (category.isEmpty())
        m_categories.remove(hash);
    else
        m_categories[hash] = category;

    // If the category has a save path and the torrent is still downloading,
    // update the intended destination (temp path integration).
    if (!category.isEmpty() && m_categorySavePaths.contains(category)) {
        const QString &catPath = m_categorySavePaths[category];
        if (!catPath.isEmpty()) {
            lt::torrent_status st = cachedStatus(m_torrents[index]);
            bool downloading = (st.state == lt::torrent_status::downloading
                             || st.state == lt::torrent_status::downloading_metadata);
            if (downloading) {
                if (!m_tempPath.isEmpty() && QDir(m_tempPath).exists()) {
                    m_torrentIntendedPath[hash] = catPath;
                } else {
                    m_torrents[index].move_storage(catPath.toStdString());
                }
            }
        }
    }
}

QStringList SessionManager::categories() const
{
    QStringList list = {"Movies", "Games", "Software", "Music", "Other"};
    // Add any custom categories that aren't in the built-in list
    for (auto it = m_categories.cbegin(); it != m_categories.cend(); ++it) {
        if (!list.contains(it.value()) && !it.value().isEmpty())
            list.append(it.value());
    }
    // Also include categories that have save paths but no torrent assigned yet
    for (auto it = m_categorySavePaths.cbegin(); it != m_categorySavePaths.cend(); ++it) {
        if (!list.contains(it.key()))
            list.append(it.key());
    }
    return list;
}

void SessionManager::setCategorySavePath(const QString &category, const QString &path)
{
    if (path.isEmpty())
        m_categorySavePaths.remove(category);
    else
        m_categorySavePaths[category] = path;
    QSettings s("BATorrent", "BATorrent");
    s.beginGroup("categorySavePaths");
    if (path.isEmpty()) s.remove(category);
    else s.setValue(category, path);
    s.endGroup();
}

QString SessionManager::categorySavePath(const QString &category) const
{
    return m_categorySavePaths.value(category);
}

QMap<QString, QString> SessionManager::allCategorySavePaths() const
{
    return m_categorySavePaths;
}

QStringList SessionManager::torrentTags(int index) const
{
    QString hash = torrentHash(index);
    if (hash.isEmpty()) return {};
    return m_torrentTags.value(hash);
}

void SessionManager::setTorrentTags(int index, const QStringList &tags)
{
    QString hash = torrentHash(index);
    if (hash.isEmpty()) return;
    // Strip empty/whitespace entries, dedupe case-insensitively.
    QStringList clean;
    for (const QString &raw : tags) {
        QString t = raw.trimmed();
        if (t.isEmpty()) continue;
        bool dup = false;
        for (const QString &e : clean) if (e.compare(t, Qt::CaseInsensitive) == 0) { dup = true; break; }
        if (!dup) clean.append(t);
    }
    QSettings s("BATorrent", "BATorrent");
    if (clean.isEmpty()) {
        m_torrentTags.remove(hash);
        s.remove("torrentTags/" + hash);
    } else {
        m_torrentTags[hash] = clean;
        s.setValue("torrentTags/" + hash, clean.join(","));
    }
    emit torrentsUpdated();
}

QStringList SessionManager::allTags() const
{
    QSet<QString> seen;
    for (auto it = m_torrentTags.cbegin(); it != m_torrentTags.cend(); ++it)
        for (const QString &t : it.value()) seen.insert(t);
    QStringList out(seen.begin(), seen.end());
    std::sort(out.begin(), out.end(), [](const QString &a, const QString &b) {
        return a.compare(b, Qt::CaseInsensitive) < 0;
    });
    return out;
}
