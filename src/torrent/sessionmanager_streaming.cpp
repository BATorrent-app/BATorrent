// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// SessionManager: streaming slice. Piece-priority/deadline shaping and the
// byte-availability queries the local StreamServer polls while playing a file
// mid-download. Split out of sessionmanager.cpp (the engine monolith). The
// general index/hash helpers stay in the core file.

#include "torrent/sessionmanager.h"
#include "torrent/streamgate.h"

#include <libtorrent/torrent_info.hpp>
#include <QDebug>
#include <QFileInfo>
#include <QVariantMap>
#include <QBitArray>
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
    // with a minimum of 1 piece. This adapts to large files: a 96GB game
    // gets ~150 pieces boosted vs the previous fixed 4+2.
    const int filesPieces = lastPiece - firstPiece + 1;
    const int numToBoost = std::max(1, static_cast<int>(std::ceil(filesPieces * 0.01)));
    auto boost = [&](int p) {
        if (p >= 0 && p < numPieces)
            h.piece_priority(lt::piece_index_t(p), lt::download_priority_t(7));
    };
    for (int k = 0; k < numToBoost; ++k) boost(firstPiece + k);
    for (int k = 0; k < numToBoost; ++k) boost(lastPiece - k);
    // An MP4 with its 'moov' atom at EOF is unplayable until the last pieces
    // arrive, and FFmpeg reads an MKV's end on open too (seek head, cues): it
    // blocked 3.5 s there without this. Priority alone won't fetch them early,
    // only a deadline jumps the queue. Just the last couple of pieces; more
    // deadlines here only compete with the head.
    for (int k = 0; k < StreamGate::kTailPieces; ++k) {
        const int p = lastPiece - k;
        if (p >= firstPiece && p >= 0 && p < numPieces)
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
    // toggle the ".!bt" incomplete suffix: the on-disk name may lag the mapping
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
    const auto &fs = ti->files();
    const auto fIdx = lt::file_index_t(fileIndex);
    const qint64 fileOffset = fs.file_offset(fIdx);
    const qint64 fileSize = fs.file_size(fIdx);
    if (fileSize <= 0) return;
    const qint64 off = fileOffset + std::clamp<qint64>(startByte, 0, fileSize - 1);
    const int firstPiece = int(off / pieceSize);
    const int lastFilePiece = std::min(int((fileOffset + fileSize - 1) / pieceSize), ti->num_pieces() - 1);
    const int window = windowPieces > 0 ? windowPieces : StreamGate::windowPieces(pieceSize, fileSize);
    const int last = std::min(firstPiece + window - 1, lastFilePiece);

    int firstMissing = -1;
    for (int p = firstPiece; p <= last; ++p)
        if (!h.have_piece(lt::piece_index_t(p))) { firstMissing = p; break; }
    if (firstMissing < 0) return;   // the whole window is already here

    // Deadlines count from now, so setting the same window again every poll
    // keeps pushing it back. Only a reader that moved changes anything.
    if (!m_streamed.count(h)) m_streamed[h] = torrentHash(torrentIndex);
    StreamWindow &w = m_streamWindows[h];
    if (w.file == fileIndex && w.first == firstPiece && w.firstMissing == firstMissing) return;

    // FFmpeg reads an MKV's index at EOF on a second connection while the main
    // reader stays put. That read is not the reader moving: fetch it without
    // touching the window, or the two connections throw each other's window
    // away on every 200 ms retry.
    const bool tailRead = firstPiece > lastFilePiece - StreamGate::kTailPieces;
    if (tailRead && w.file == fileIndex) {
        for (int p = firstMissing; p <= last; ++p)
            if (!h.have_piece(lt::piece_index_t(p)))
                h.set_piece_deadline(lt::piece_index_t(p), 0);
        return;
    }

    // A seek leaves the old window's deadlines competing with the new one.
    const bool jumped = w.file != fileIndex || firstPiece < w.first || firstPiece > w.last;
    if (jumped && w.file >= 0) {
        h.clear_piece_deadlines();
        // clearing took the container index's deadlines with it
        for (int p = std::max(int(fileOffset / pieceSize), lastFilePiece - StreamGate::kTailPieces + 1); p <= lastFilePiece; ++p)
            if (!h.have_piece(lt::piece_index_t(p)))
                h.set_piece_deadline(lt::piece_index_t(p), 2000);
    }
    const bool moved = w.file != fileIndex || w.first != firstPiece;
    w = { fileIndex, firstMissing, firstPiece, last };

    // No scheduling helps when no connected peer has the piece the player is
    // stuck on; say so, so a slow swarm isn't mistaken for a slow app.
    std::vector<int> avail;
    h.piece_availability(avail);
    if (firstMissing < int(avail.size()) && avail[size_t(firstMissing)] == 0)
        qInfo().noquote() << "[stream] piece" << firstMissing << "is on none of the"
                          << cachedStatus(h).num_peers << "connected peers";

    // Priority follows the reader, the way Stremio does it: with the whole
    // file at one priority the window was no more wanted than any other piece
    // and rarest-first spread the bandwidth over all of it. After a seek the
    // part behind the reader drops to the bottom instead of being fetched first.
    if (moved) {
        const int fileFirst = int(fileOffset / pieceSize);
        const int nearEnd = std::min(last + window * StreamGate::kNearAheadWindows, lastFilePiece);
        std::vector<lt::download_priority_t> prio = h.get_piece_priorities();
        for (int p = fileFirst; p <= lastFilePiece && p < int(prio.size()); ++p) {
            int level = 4;                                   // later in the file
            if (p < firstPiece) level = 1;                   // already watched or skipped
            else if (p <= last) level = 7;                   // the deadline window
            else if (p <= nearEnd) level = 6;                // the next few minutes
            if (p > lastFilePiece - StreamGate::kTailPieces) level = 7;   // container index at EOF
            prio[size_t(p)] = lt::download_priority_t(std::uint8_t(level));
        }
        h.prioritize_pieces(prio);
    }

    for (int p = firstMissing; p <= last; ++p) {
        if (h.have_piece(lt::piece_index_t(p))) continue;
        // when playback should get here: tight up front, relaxed further out
        h.set_piece_deadline(lt::piece_index_t(p), StreamGate::deadlineMs(qint64(p) * pieceSize - off, fileSize));
    }
}

QVariantMap SessionManager::streamGeometry(int torrentIndex, int fileIndex)
{
    QVariantMap out;
    if (torrentIndex < 0 || torrentIndex >= static_cast<int>(m_torrents.size())) return out;
    auto &h = m_torrents[torrentIndex];
    if (!h.is_valid()) return out;
    auto ti = h.torrent_file();
    if (!ti || fileIndex < 0 || fileIndex >= ti->num_files()) return out;
    // registered before the pieces are read, so none finishes unseen in between
    if (!m_streamed.count(h)) m_streamed[h] = torrentHash(torrentIndex);
    const auto fIdx = lt::file_index_t(fileIndex);
    QBitArray have(ti->num_pieces());
    for (int p = 0; p < ti->num_pieces(); ++p)
        if (h.have_piece(lt::piece_index_t(p))) have.setBit(p);
    out[QStringLiteral("pieceSize")] = qint64(ti->piece_length());
    out[QStringLiteral("fileOffset")] = qint64(ti->files().file_offset(fIdx));
    out[QStringLiteral("fileSize")] = qint64(ti->files().file_size(fIdx));
    out[QStringLiteral("have")] = have;
    return out;
}
