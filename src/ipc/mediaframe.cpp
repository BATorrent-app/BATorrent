// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "ipc/mediaframe.h"
#include <QVideoFrameFormat>
#include <algorithm>
#include <cstring>

namespace {

constexpr qint64 kAlign = 64;

qint64 aligned(qint64 n) { return (n + kAlign - 1) / kAlign * kAlign; }

}

namespace media {

qint64 packedSize(const QVideoFrame &mapped)
{
    qint64 total = 0;
    for (int i = 0; i < mapped.planeCount() && i < kMaxPlanes; ++i)
        total = aligned(total) + mapped.mappedBytes(i);
    return aligned(total);
}

bool pack(const QVideoFrame &mapped, uchar *slot, qint64 slotBytes, FrameHeader &hdr)
{
    const int planes = mapped.planeCount();
    if (!slot || planes <= 0 || planes > kMaxPlanes || packedSize(mapped) > slotBytes) return false;
    hdr.pixelFormat = qint32(mapped.pixelFormat());
    hdr.width = mapped.width();
    hdr.height = mapped.height();
    hdr.startUs = mapped.startTime();
    hdr.endUs = mapped.endTime();
    hdr.planeCount = planes;
    qint64 offset = 0;
    for (int i = 0; i < planes; ++i) {
        offset = aligned(offset);
        Plane &p = hdr.planes[size_t(i)];
        p.stride = mapped.bytesPerLine(i);
        p.bytes = mapped.mappedBytes(i);
        p.offset = offset;
        std::memcpy(slot + offset, mapped.bits(i), size_t(p.bytes));
        offset += p.bytes;
    }
    return true;
}

bool isAllowedFormat(qint32 pixelFormat)
{
    switch (QVideoFrameFormat::PixelFormat(pixelFormat)) {
    case QVideoFrameFormat::Format_ARGB8888:
    case QVideoFrameFormat::Format_ARGB8888_Premultiplied:
    case QVideoFrameFormat::Format_XRGB8888:
    case QVideoFrameFormat::Format_BGRA8888:
    case QVideoFrameFormat::Format_BGRA8888_Premultiplied:
    case QVideoFrameFormat::Format_BGRX8888:
    case QVideoFrameFormat::Format_ABGR8888:
    case QVideoFrameFormat::Format_XBGR8888:
    case QVideoFrameFormat::Format_RGBA8888:
    case QVideoFrameFormat::Format_RGBX8888:
    case QVideoFrameFormat::Format_YUV420P:
    case QVideoFrameFormat::Format_YUV422P:
    case QVideoFrameFormat::Format_YV12:
    case QVideoFrameFormat::Format_UYVY:
    case QVideoFrameFormat::Format_YUYV:
    case QVideoFrameFormat::Format_NV12:
    case QVideoFrameFormat::Format_NV21:
    case QVideoFrameFormat::Format_P010:
    case QVideoFrameFormat::Format_P016:
    case QVideoFrameFormat::Format_YUV420P10:
    case QVideoFrameFormat::Format_Y8:
    case QVideoFrameFormat::Format_Y16:
        return true;
    default:
        return false;
    }
}

bool headerFits(const FrameHeader &hdr, qint64 slotBytes)
{
    if (!isAllowedFormat(hdr.pixelFormat)) return false;
    if (hdr.width <= 0 || hdr.height <= 0 || hdr.width > kMaxDimension || hdr.height > kMaxDimension)
        return false;
    if (hdr.planeCount <= 0 || hdr.planeCount > kMaxPlanes || slotBytes <= 0) return false;
    for (int i = 0; i < hdr.planeCount; ++i) {
        const Plane &p = hdr.planes[size_t(i)];
        if (p.stride <= 0 || p.offset < 0 || p.bytes <= 0) return false;
        if (p.offset > slotBytes || p.bytes > slotBytes - p.offset) return false;
    }
    return true;
}

QVideoFrame unpack(const FrameHeader &hdr, const uchar *slot, qint64 slotBytes)
{
    if (!slot || !headerFits(hdr, slotBytes)) return {};
    QVideoFrame out(QVideoFrameFormat(QSize(hdr.width, hdr.height),
                                      QVideoFrameFormat::PixelFormat(hdr.pixelFormat)));
    if (!out.map(QVideoFrame::WriteOnly)) return {};
    if (out.planeCount() != hdr.planeCount) { out.unmap(); return {}; }
    for (int i = 0; i < hdr.planeCount; ++i) {
        const Plane &src = hdr.planes[size_t(i)];
        const qint64 dstStride = out.bytesPerLine(i);
        if (dstStride <= 0) { out.unmap(); return {}; }
        const qint64 rows = out.mappedBytes(i) / dstStride;
        const qint64 rowBytes = std::min<qint64>(dstStride, src.stride);
        if (rows <= 0 || (rows - 1) * qint64(src.stride) + rowBytes > src.bytes) {
            out.unmap();
            return {};
        }
        uchar *dst = out.bits(i);
        const uchar *from = slot + src.offset;
        for (qint64 r = 0; r < rows; ++r)
            std::memcpy(dst + r * dstStride, from + r * src.stride, size_t(rowBytes));
    }
    out.unmap();
    out.setStartTime(hdr.startUs);
    out.setEndTime(hdr.endUs);
    return out;
}

}
