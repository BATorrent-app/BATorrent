// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/security/mediasniff.h"
#include <QFileInfo>
#include <QSet>
#include <algorithm>

namespace {

bool at(const QByteArray &b, qint64 off, const char *sig, int len)
{
    if (off < 0 || off + len > b.size()) return false;
    return std::equal(sig, sig + len, b.constData() + off);
}

quint32 le32(const QByteArray &b, qint64 off)
{
    const auto *p = reinterpret_cast<const uchar *>(b.constData() + off);
    return quint32(p[0]) | quint32(p[1]) << 8 | quint32(p[2]) << 16 | quint32(p[3]) << 24;
}

quint64 le64(const QByteArray &b, qint64 off)
{
    return quint64(le32(b, off)) | quint64(le32(b, off + 4)) << 32;
}

// GUIDs as they sit on disk (the first three fields little-endian).
const char kAsfHeader[16]     = { '\x30','\x26','\xB2','\x75','\x8E','\x66','\xCF','\x11',
                                  '\xA6','\xD9','\x00','\xAA','\x00','\x62','\xCE','\x6C' };
const char kAsfEncryption[16] = { '\xFB','\xB3','\x11','\x22','\x23','\xBD','\xD2','\x11',
                                  '\xB4','\xB7','\x00','\xA0','\xC9','\x55','\xFC','\x6E' };
const char kAsfExtEncrypt[16] = { '\x14','\xE6','\x8A','\x29','\x22','\x26','\x17','\x4C',
                                  '\xB9','\x35','\xDA','\xE0','\x7E','\xE9','\x28','\x9C' };
const char kAsfScript[16]     = { '\x30','\x1A','\xFB','\x1E','\x62','\x0B','\xD0','\x11',
                                  '\xA3','\x9B','\x00','\xA0','\xC9','\x03','\x48','\xF6' };

// "MZ" alone is two bytes any file could start with; the PE signature its
// header points at is what makes it a Windows program.
bool isPortableExecutable(const QByteArray &b)
{
    if (!at(b, 0, "MZ", 2) || b.size() < 0x40) return false;
    return at(b, le32(b, 0x3C), "PE\0\0", 4);
}

bool isTransportStream(const QByteArray &b)
{
    auto synced = [&](qint64 first, qint64 stride) {
        for (int i = 0; i < 3; ++i)
            if (first + i * stride >= b.size() || b.at(first + i * stride) != '\x47') return false;
        return true;
    };
    return synced(0, 188) || synced(4, 192);
}

bool isWebPage(const QByteArray &b)
{
    qint64 i = at(b, 0, "\xEF\xBB\xBF", 3) ? 3 : 0;
    while (i < b.size() && i < 512 && QChar::isSpace(uchar(b.at(i)))) ++i;
    const QByteArray lead = b.mid(i, 16).toLower();
    return lead.startsWith("<!doctype html") || lead.startsWith("<html")
        || lead.startsWith("<script") || lead.startsWith("<asx");
}

}

namespace MediaSniff {

qint64 headWanted(qint64 fileSize)
{
    return std::clamp<qint64>(fileSize, 0, kHeadBytes);
}

bool isVideoName(const QString &fileName)
{
    static const QSet<QString> exts = {
        "mkv","mp4","avi","mov","wmv","m4v","ts","flv","webm","m2ts","mpg","mpeg"
    };
    QString n = fileName;
    if (n.endsWith(QLatin1String(".!bt"))) n.chop(4);
    return exts.contains(QFileInfo(n).suffix().toLower());
}

Kind identify(const QByteArray &b)
{
    if (at(b, 0, "\x1A\x45\xDF\xA3", 4)) return Kind::Matroska;
    if (at(b, 0, kAsfHeader, 16)) return Kind::Asf;
    for (const char *box : { "ftyp", "moov", "mdat", "free", "skip", "wide", "pnot" })
        if (at(b, 4, box, 4)) return Kind::Mp4;
    if (at(b, 0, "RIFF", 4) && (at(b, 8, "AVI ", 4) || at(b, 8, "AVIX", 4))) return Kind::Avi;
    if (at(b, 0, "FLV\x01", 4)) return Kind::Flv;
    if (at(b, 0, "\x00\x00\x01\xBA", 4) || at(b, 0, "\x00\x00\x01\xB3", 4)) return Kind::MpegPs;
    if (isTransportStream(b)) return Kind::MpegTs;

    if (isPortableExecutable(b)
        || at(b, 0, "\x7F" "ELF", 4)
        || at(b, 0, "\xFE\xED\xFA\xCE", 4) || at(b, 0, "\xFE\xED\xFA\xCF", 4)
        || at(b, 0, "\xCE\xFA\xED\xFE", 4) || at(b, 0, "\xCF\xFA\xED\xFE", 4)
        || at(b, 0, "\xCA\xFE\xBA\xBE", 4)
        || at(b, 0, "\x4C\x00\x00\x00\x01\x14\x02\x00", 8)   // Windows shortcut (.lnk)
        || at(b, 0, "#!", 2))
        return Kind::Executable;
    if (at(b, 0, "PK\x03\x04", 4) || at(b, 0, "PK\x05\x06", 4)
        || at(b, 0, "Rar!\x1A\x07", 6) || at(b, 0, "7z\xBC\xAF\x27\x1C", 6)
        || at(b, 0, "\x1F\x8B", 2) || at(b, 0, "MSCF", 4))
        return Kind::Archive;
    if (at(b, 0, "%PDF-", 5) || at(b, 0, "\xD0\xCF\x11\xE0\xA1\xB1\x1A\xE1", 8))
        return Kind::Document;
    if (isWebPage(b)) return Kind::WebPage;
    return Kind::Unknown;
}

bool asfHasLure(const QByteArray &b)
{
    constexpr qint64 kObjHead = 24;   // GUID + 64-bit size
    if (!at(b, 0, kAsfHeader, 16) || b.size() < 30) return false;
    const qint64 end = qint64(std::min<quint64>(le64(b, 16), quint64(b.size())));
    qint64 off = 30;
    while (off + kObjHead <= end) {
        if (at(b, off, kAsfEncryption, 16) || at(b, off, kAsfExtEncrypt, 16)
            || at(b, off, kAsfScript, 16))
            return true;
        const quint64 size = le64(b, off + 16);
        if (size < quint64(kObjHead) || size > quint64(end - off)) break;
        off += qint64(size);
    }
    return false;
}

Verdict judge(const QByteArray &head, qint64 fileSize)
{
    if (fileSize <= 0 || head.size() < headWanted(fileSize)) return Verdict::NeedMore;
    switch (identify(head)) {
    case Kind::Executable: case Kind::Archive: case Kind::Document: case Kind::WebPage:
        return Verdict::Disguised;
    case Kind::Asf:
        return asfHasLure(head) ? Verdict::Lure : Verdict::Ok;
    default:
        return Verdict::Ok;
    }
}

QString kindKey(Kind kind)
{
    switch (kind) {
    case Kind::Executable: return QStringLiteral("program");
    case Kind::Archive:    return QStringLiteral("archive");
    case Kind::Document:   return QStringLiteral("document");
    case Kind::WebPage:    return QStringLiteral("webpage");
    default:               return QStringLiteral("unknown");
    }
}

}
