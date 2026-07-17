// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/metadata/mkvchapters.h"

#include <QFile>
#include <QFileInfo>

namespace {

// Matroska element IDs (stored with their EBML length-marker bits intact).
constexpr quint32 ID_EBML        = 0x1A45DFA3;
constexpr quint32 ID_SEGMENT     = 0x18538067;
constexpr quint32 ID_CHAPTERS    = 0x1043A770;
constexpr quint32 ID_EDITION     = 0x45B9;
constexpr quint32 ID_CHAPTERATOM = 0xB6;
constexpr quint32 ID_TIME_START  = 0x91;
constexpr quint32 ID_TIME_END    = 0x92;
constexpr quint32 ID_DISPLAY     = 0x80;
constexpr quint32 ID_CHAPSTRING  = 0x85;

// Guards against a malformed/hostile file turning the walk into a DoS.
constexpr int MAX_CHAPTERS  = 500;
constexpr int MAX_NAME_LEN  = 4096;
constexpr int MAX_DEPTH     = 8;

struct Reader {
    QFile &f;
    qint64 end;   // absolute byte offset one past the last readable byte

    bool atEnd(qint64 pos) const { return pos >= end; }

    // EBML variable-length integer. `keepMarker` keeps the length-descriptor
    // bit set (element IDs) vs. clearing it (sizes/values). On success advances
    // `pos` and returns true; `value` gets the decoded number, `unknown` is set
    // when a size field is all-ones (unknown-size element). Fully bounds-checked.
    bool readVint(qint64 &pos, quint64 &value, bool keepMarker, bool *unknown = nullptr)
    {
        if (unknown) *unknown = false;
        if (atEnd(pos)) return false;
        if (!f.seek(pos)) return false;
        unsigned char first;
        if (f.read(reinterpret_cast<char *>(&first), 1) != 1) return false;
        if (first == 0x00) return false;   // no length marker in the first byte

        int len = 1;
        unsigned char mask = 0x80;
        while (!(first & mask)) { mask >>= 1; ++len; }
        if (len < 1 || len > 8) return false;
        if (pos + len > end) return false;

        quint64 v = keepMarker ? first : (first & (mask - 1));
        bool allOnes = ((first & (mask - 1)) == (unsigned)(mask - 1));
        for (int i = 1; i < len; ++i) {
            unsigned char b;
            if (f.read(reinterpret_cast<char *>(&b), 1) != 1) return false;
            v = (v << 8) | b;
            if (b != 0xFF) allOnes = false;
        }
        if (unknown && !keepMarker) *unknown = allOnes;
        value = v;
        pos += len;
        return true;
    }

    // Reads an element header: its ID (marker kept) and content size. Returns
    // the absolute offset of the content and whether the size was "unknown".
    bool readHeader(qint64 &pos, quint32 &id, quint64 &size, bool &unknownSize)
    {
        quint64 rawId = 0, rawSize = 0;
        if (!readVint(pos, rawId, /*keepMarker*/ true)) return false;
        if (rawId > 0xFFFFFFFFull) return false;
        if (!readVint(pos, rawSize, /*keepMarker*/ false, &unknownSize)) return false;
        id = static_cast<quint32>(rawId);
        size = rawSize;
        return true;
    }

    // Big-endian unsigned integer of `size` bytes (<= 8), for time fields.
    bool readUInt(qint64 pos, quint64 size, quint64 &out)
    {
        if (size == 0 || size > 8 || pos + qint64(size) > end) return false;
        if (!f.seek(pos)) return false;
        quint64 v = 0;
        for (quint64 i = 0; i < size; ++i) {
            unsigned char b;
            if (f.read(reinterpret_cast<char *>(&b), 1) != 1) return false;
            v = (v << 8) | b;
        }
        out = v;
        return true;
    }

    QString readString(qint64 pos, quint64 size)
    {
        if (size == 0 || size > MAX_NAME_LEN || pos + qint64(size) > end) return {};
        if (!f.seek(pos)) return {};
        QByteArray buf = f.read(qint64(size));
        return QString::fromUtf8(buf).trimmed();
    }
};

QString classifyChapter(const QString &nameRaw)
{
    const QString n = nameRaw.toLower();
    if (n.contains(QLatin1String("intro")) || n.contains(QLatin1String("opening"))
        || n.contains(QLatin1String("recap")) || n.contains(QLatin1String("previously"))
        || n.contains(QLatin1String("abertura")))
        return QStringLiteral("intro");
    if (n.contains(QLatin1String("credit")) || n.contains(QLatin1String("ending"))
        || n.contains(QLatin1String("outro")) || n.contains(QLatin1String("encerramento")))
        return QStringLiteral("credits");
    return {};
}

// ChapterTimeStart/End are absolute nanoseconds (independent of the segment
// TimestampScale). Convert to ms.
qint64 nsToMs(quint64 ns) { return qint64(ns / 1000000ull); }

void parseChapterAtom(Reader &r, qint64 start, qint64 stop, MkvChapter &ch, int depth)
{
    if (depth > MAX_DEPTH) return;
    qint64 pos = start;
    while (pos < stop && !r.atEnd(pos)) {
        quint32 id; quint64 size; bool unknownSize = false;
        qint64 headerStart = pos;
        if (!r.readHeader(pos, id, size, unknownSize)) return;
        if (unknownSize || pos + qint64(size) > stop) return;   // don't run past our parent
        const qint64 content = pos;

        if (id == ID_TIME_START) {
            quint64 ns = 0;
            if (r.readUInt(content, size, ns)) ch.startMs = nsToMs(ns);
        } else if (id == ID_TIME_END) {
            quint64 ns = 0;
            if (r.readUInt(content, size, ns)) ch.endMs = nsToMs(ns);
        } else if (id == ID_DISPLAY) {
            // ChapterDisplay → ChapString
            qint64 dpos = content;
            const qint64 dstop = content + qint64(size);
            while (dpos < dstop && !r.atEnd(dpos)) {
                quint32 did; quint64 dsize; bool du = false;
                if (!r.readHeader(dpos, did, dsize, du)) break;
                if (du || dpos + qint64(dsize) > dstop) break;
                if (did == ID_CHAPSTRING && ch.name.isEmpty())
                    ch.name = r.readString(dpos, dsize);
                dpos += qint64(dsize);
            }
        }
        (void)headerStart;
        pos = content + qint64(size);
    }
    ch.kind = classifyChapter(ch.name);
}

void parseChapters(Reader &r, qint64 start, qint64 stop, QList<MkvChapter> &out, int depth)
{
    if (depth > MAX_DEPTH) return;
    qint64 pos = start;
    while (pos < stop && !r.atEnd(pos) && out.size() < MAX_CHAPTERS) {
        quint32 id; quint64 size; bool unknownSize = false;
        if (!r.readHeader(pos, id, size, unknownSize)) return;
        if (unknownSize || pos + qint64(size) > stop) return;
        const qint64 content = pos;

        if (id == ID_EDITION) {
            parseChapters(r, content, content + qint64(size), out, depth + 1);   // recurse into the edition
        } else if (id == ID_CHAPTERATOM) {
            MkvChapter ch;
            parseChapterAtom(r, content, content + qint64(size), ch, depth + 1);
            if (ch.startMs >= 0) out.push_back(ch);
        }
        pos = content + qint64(size);
    }
}

} // namespace

QList<MkvChapter> readMkvChapters(const QString &filePath)
{
    QList<MkvChapter> out;
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return out;
    const qint64 fileSize = f.size();
    if (fileSize < 64) return out;

    Reader r{f, fileSize};
    qint64 pos = 0;

    // Top level: EBML header, then Segment.
    quint32 id; quint64 size; bool unknownSize = false;
    if (!r.readHeader(pos, id, size, unknownSize) || id != ID_EBML) return out;
    if (unknownSize || pos + qint64(size) > fileSize) return out;
    pos += qint64(size);   // skip the EBML header body

    if (!r.readHeader(pos, id, size, unknownSize) || id != ID_SEGMENT) return out;
    const qint64 segStart = pos;
    const qint64 segEnd = unknownSize ? fileSize
                                      : qMin(fileSize, segStart + qint64(size));

    // Walk the segment's children by header only, seeking past big elements
    // (clusters etc.) until we hit Chapters.
    qint64 child = segStart;
    int guard = 0;
    while (child < segEnd && guard++ < 100000) {
        quint32 cid; quint64 csize; bool cunknown = false;
        qint64 before = child;
        if (!r.readHeader(child, cid, csize, cunknown)) break;
        if (cunknown) break;                       // can't skip an unknown-size child safely
        const qint64 content = child;
        if (content + qint64(csize) > segEnd) break;
        if (cid == ID_CHAPTERS) {
            parseChapters(r, content, content + qint64(csize), out, 0);
            break;
        }
        child = content + qint64(csize);
        if (child <= before) break;                // no forward progress → bail
    }
    return out;
}
