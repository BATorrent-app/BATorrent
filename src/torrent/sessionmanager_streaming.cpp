// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// SessionManager — streaming slice. Piece-priority/deadline shaping and the
// byte-availability queries the local StreamServer polls while playing a file
// mid-download. Split out of sessionmanager.cpp (the engine monolith) verbatim;
// no behaviour change. The general index/hash helpers stay in the core file.

#include "torrent/sessionmanager.h"

#include <libtorrent/torrent_info.hpp>
#include <QFileInfo>
#include <QVariantMap>
#include <algorithm>
#include <cmath>

void SessionManager::prioritizeFilePieceBoundaries(int torrentIndex, int fileIndex)
{
    if (torrentIndex < 0 || torrentIndex >= static_cast<int>(m_torrents.size()))
        return;
    auto &h = m_torrents[torrentIndex];
    if (!h.is_valid()) return;
    auto ti = h.torrent_file();
    if (!ti) return;
    const auto &fs = ti->files();
    if (fileIndex < 0 || fileIndex >= ti->num_files()) return;

    const auto fIdx = lt::file_index_t(fileIndex);
    const qint64 fileOffset = fs.file_offset(fIdx);
    const qint64 fileSize   = fs.file_size(fIdx);
    if (fileSize <= 0) return;
    const int pieceSize = ti->piece_length();
    if (pieceSize <= 0) return;
    const int numPieces = ti->num_pieces();

    const int firstPiece = int(fileOffset / pieceSize);
    const int lastPiece  = int((fileOffset + fileSize - 1) / pieceSize);

    // Boost 1% of the file's piece count at each end (like qBittorrent),
    // with a minimum of 1 piece. This adapts to large files — a 96GB game
    // gets ~150 pieces boosted vs the previous fixed 4+2.
    const int filesPieces = lastPiece - firstPiece + 1;
    const int numToBoost = std::max(1, static_cast<int>(std::ceil(filesPieces * 0.01)));
    auto boost = [&](int p) {
        if (p >= 0 && p < numPieces)
            h.piece_priority(lt::piece_index_t(p), lt::download_priority_t(7));
    };
    for (int k = 0; k < numToBoost; ++k) boost(firstPiece + k);
    // The tail matters for playback start: an MP4 with its 'moov' atom at EOF is
    // unplayable until the last pieces arrive. Under sequential_download a high
    // priority alone won't fetch them early — only a deadline jumps the in-order
    // queue. Give the tail a deadline so the player can start without waiting for
    // the whole file. (The head is covered by sequential + the reactive window.)
    for (int k = 0; k < numToBoost; ++k) {
        const int p = lastPiece - k;
        boost(p);
        if (p >= 0 && p < numPieces)
            h.set_piece_deadline(lt::piece_index_t(p), 2000);
    }
}

QVariantMap SessionManager::streamFileStats(int torrentIndex, int fileIndex) const
{
    QVariantMap out;
    if (torrentIndex < 0 || torrentIndex >= static_cast<int>(m_torrents.size())) return out;
    const auto &h = m_torrents[torrentIndex];
    if (!h.is_valid()) return out;
    auto ti = h.torrent_file();
    if (!ti || fileIndex < 0 || fileIndex >= ti->num_files()) return out;

    const qint64 fileSize = ti->files().file_size(lt::file_index_t(fileIndex));
    std::vector<std::int64_t> fp;
    h.file_progress(fp, lt::torrent_handle::piece_granularity);
    const qint64 done = (fileIndex < static_cast<int>(fp.size())) ? fp[fileIndex] : 0;
    const qint64 buffered = streamContiguousAvailableBytes(torrentIndex, fileIndex, 0, fileSize);

    out[QStringLiteral("totalBytes")]      = fileSize;
    out[QStringLiteral("downloadedBytes")] = done;
    out[QStringLiteral("progress")]        = fileSize > 0 ? double(done) / double(fileSize) : 0.0;
    out[QStringLiteral("buffered")]        = fileSize > 0 ? double(buffered) / double(fileSize) : 0.0;
    return out;
}

QString SessionManager::streamFilePath(int torrentIndex, int fileIndex) const
{
    if (torrentIndex < 0 || torrentIndex >= static_cast<int>(m_torrents.size())) return {};
    const auto &h = m_torrents[torrentIndex];
    if (!h.is_valid()) return {};
    auto ti = h.torrent_file();
    if (!ti) return {};
    const auto &fs = ti->files();
    if (fileIndex < 0 || fileIndex >= ti->num_files()) return {};

    const QString savePath = QString::fromStdString(h.status(lt::torrent_handle::query_save_path).save_path);
    QString rel = QString::fromStdString(fs.file_path(lt::file_index_t(fileIndex)));
    rel.replace('\\', '/');   // libtorrent uses the native separator on Windows
    QString abs = savePath + QLatin1Char('/') + rel;

    if (QFileInfo::exists(abs)) return abs;
    // toggle the ".!bt" incomplete suffix — the on-disk name may lag the mapping
    if (abs.endsWith(QStringLiteral(".!bt"))) {
        const QString plain = abs.chopped(4);
        if (QFileInfo::exists(plain)) return plain;
    } else if (QFileInfo::exists(abs + QStringLiteral(".!bt"))) {
        return abs + QStringLiteral(".!bt");
    }
    return abs;   // best guess (may not exist yet)
}

qint64 SessionManager::streamFileSize(int torrentIndex, int fileIndex) const
{
    if (torrentIndex < 0 || torrentIndex >= static_cast<int>(m_torrents.size())) return 0;
    const auto &h = m_torrents[torrentIndex];
    if (!h.is_valid()) return 0;
    auto ti = h.torrent_file();
    if (!ti) return 0;
    if (fileIndex < 0 || fileIndex >= ti->num_files()) return 0;
    return ti->files().file_size(lt::file_index_t(fileIndex));
}

bool SessionManager::streamByteAvailable(int torrentIndex, int fileIndex, qint64 byteInFile) const
{
    if (torrentIndex < 0 || torrentIndex >= static_cast<int>(m_torrents.size())) return false;
    const auto &h = m_torrents[torrentIndex];
    if (!h.is_valid()) return false;
    auto ti = h.torrent_file();
    if (!ti) return false;
    if (fileIndex < 0 || fileIndex >= ti->num_files()) return false;
    const int pieceSize = ti->piece_length();
    if (pieceSize <= 0) return false;
    const qint64 off = ti->files().file_offset(lt::file_index_t(fileIndex)) + byteInFile;
    const int piece = int(off / pieceSize);
    if (piece < 0 || piece >= ti->num_pieces()) return false;
    return h.have_piece(lt::piece_index_t(piece));
}

qint64 SessionManager::streamContiguousAvailableBytes(int torrentIndex, int fileIndex,
                                                      qint64 fromByte, qint64 cap) const
{
    if (torrentIndex < 0 || torrentIndex >= static_cast<int>(m_torrents.size())) return 0;
    const auto &h = m_torrents[torrentIndex];
    if (!h.is_valid()) return 0;
    auto ti = h.torrent_file();
    if (!ti) return 0;
    if (fileIndex < 0 || fileIndex >= ti->num_files()) return 0;
    const int pieceSize = ti->piece_length();
    if (pieceSize <= 0) return 0;
    const auto &fs = ti->files();
    const qint64 fileOffset = fs.file_offset(lt::file_index_t(fileIndex));
    const qint64 fileSize   = fs.file_size(lt::file_index_t(fileIndex));
    if (fromByte < 0 || fromByte >= fileSize) return 0;

    const qint64 globalPos = fileOffset + fromByte;
    const int numPieces = ti->num_pieces();
    int piece = int(globalPos / pieceSize);
    if (piece < 0 || piece >= numPieces || !h.have_piece(lt::piece_index_t(piece))) return 0;

    // Extend across the contiguous run of available pieces.
    qint64 availEndGlobal = qint64(piece + 1) * pieceSize;
    while (availEndGlobal - globalPos < cap) {
        const int next = int(availEndGlobal / pieceSize);
        if (next >= numPieces || !h.have_piece(lt::piece_index_t(next))) break;
        availEndGlobal = qint64(next + 1) * pieceSize;
    }
    availEndGlobal = qMin(availEndGlobal, fileOffset + fileSize);
    return qMin(cap, availEndGlobal - globalPos);
}

void SessionManager::streamSetDeadlineWindow(int torrentIndex, int fileIndex,
                                             qint64 startByte, int windowPieces)
{
    if (torrentIndex < 0 || torrentIndex >= static_cast<int>(m_torrents.size())) return;
    auto &h = m_torrents[torrentIndex];
    if (!h.is_valid()) return;
    auto ti = h.torrent_file();
    if (!ti) return;
    if (fileIndex < 0 || fileIndex >= ti->num_files()) return;
    const int pieceSize = ti->piece_length();
    if (pieceSize <= 0) return;
    const int numPieces = ti->num_pieces();
    const qint64 off = ti->files().file_offset(lt::file_index_t(fileIndex)) + qMax<qint64>(0, startByte);
    const int firstPiece = int(off / pieceSize);
    for (int k = 0; k < windowPieces; ++k) {
        const int p = firstPiece + k;
        if (p < 0 || p >= numPieces) break;
        if (h.have_piece(lt::piece_index_t(p))) continue;
        h.piece_priority(lt::piece_index_t(p), lt::download_priority_t(7));
        h.set_piece_deadline(lt::piece_index_t(p), k * 40);   // ms — increasing = playback order
    }
}
