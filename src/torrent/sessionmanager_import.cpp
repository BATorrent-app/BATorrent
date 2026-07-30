// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// SessionManager — qBittorrent import + .torrent export. Split out of
// sessionmanager.cpp verbatim; no behaviour change.

#include "torrent/sessionmanager.h"

#include <libtorrent/torrent_info.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/bencode.hpp>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUrl>

int SessionManager::importFromQBittorrent(const QString &defaultSavePath)
{
    // qBittorrent stores data in BT_backup:
    //   Linux:   ~/.local/share/qBittorrent/BT_backup/
    //   Windows: %APPDATA%\qBittorrent\BT_backup\
    //   macOS:   ~/Library/Application Support/qBittorrent/BT_backup/
#ifdef Q_OS_WIN
    // GenericDataLocation resolves to %APPDATA% on Windows, which is the
    // parent dir qBittorrent stores its own state under. AppDataLocation
    // appends the app's own name and would land in the wrong place.
    QString btBackup = QDir(QStandardPaths::writableLocation(
        QStandardPaths::GenericDataLocation)).filePath("qBittorrent/BT_backup");
#elif defined(Q_OS_MACOS)
    QString btBackup = QDir::homePath()
        + "/Library/Application Support/qBittorrent/BT_backup";
#else
    QString btBackup = QDir::homePath() + "/.local/share/qBittorrent/BT_backup";
#endif
    QDir dir(btBackup);
    if (!dir.exists())
        return 0;

    QStringList torrentFiles = dir.entryList({"*.torrent"}, QDir::Files);
    int imported = 0;

    for (const auto &fileName : torrentFiles) {
        QString torrentPath = dir.filePath(fileName);
        QString baseName = fileName.left(fileName.length() - 8); // remove .torrent
        QString resumePath = dir.filePath(baseName + ".fastresume");

        try {
            lt::add_torrent_params atp;

            // Try to load fastresume data first (contains save_path and state)
            if (QFile::exists(resumePath)) {
                QFile resumeFile(resumePath);
                if (resumeFile.open(QIODevice::ReadOnly)) {
                    QByteArray data = resumeFile.readAll();
                    lt::error_code ec;
                    atp = lt::read_resume_data(
                        lt::span<const char>(data.data(), data.size()), ec);
                    if (ec) {
                        // Fastresume failed, load torrent file instead
                        atp.ti = std::make_shared<lt::torrent_info>(torrentPath.toStdString());
                        atp.save_path = defaultSavePath.toStdString();
                    }
                }
            } else {
                atp.ti = std::make_shared<lt::torrent_info>(torrentPath.toStdString());
                atp.save_path = defaultSavePath.toStdString();
            }

            // If save_path is empty, use default
            if (atp.save_path.empty())
                atp.save_path = defaultSavePath.toStdString();

            // Check if we already have this torrent
            bool duplicate = false;
            for (const auto &h : m_torrents) {
                if (h.is_valid() && h.info_hashes() == atp.info_hashes) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) continue;

            lt::torrent_handle h = m_session.add_torrent(atp);
            m_torrents.push_back(h);
            ++imported;
        } catch (const std::exception &e) {
            qWarning() << "importFromQBittorrent skipped" << fileName << ":" << e.what();
        } catch (...) {
            qWarning() << "importFromQBittorrent skipped" << fileName
                       << ": unknown exception";
        }
    }

    if (imported > 0)
        emit torrentsUpdated();

    return imported;
}

bool SessionManager::exportTorrent(int index, const QString &destPath)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size())) return false;
    const auto &h = m_torrents[index];
    if (!h.is_valid()) return false;
    auto ti = h.torrent_file();
    if (!ti) return false;   // magnet whose metadata hasn't arrived yet
    try {
        lt::create_torrent ct(*ti);
        std::vector<char> buf;
        lt::bencode(std::back_inserter(buf), ct.generate());
        const QString local = destPath.startsWith(QStringLiteral("file:"))
            ? QUrl(destPath).toLocalFile() : destPath;
        QFile f(local);
        if (!f.open(QIODevice::WriteOnly)) return false;
        f.write(buf.data(), static_cast<qint64>(buf.size()));
        return true;
    } catch (...) {
        return false;
    }
}

