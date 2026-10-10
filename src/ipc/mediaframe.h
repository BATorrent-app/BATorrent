// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_MEDIAFRAME_H
#define BATORRENT_MEDIAFRAME_H

#include <QVideoFrame>
#include "ipc/mediaprotocol.h"

// Moving one decoded frame through a shared-memory slot. pack() runs in the
// media child; unpack() runs in the UI and treats the header as hostile: it
// sizes every copy from the frame it allocated itself, never from the child.
namespace media {

// Bytes a mapped frame needs in a slot, planes 64-byte aligned.
qint64 packedSize(const QVideoFrame &mapped);

// `mapped` must be mapped ReadOnly. Fills `hdr` (slot left to the caller).
bool pack(const QVideoFrame &mapped, uchar *slot, qint64 slotBytes, FrameHeader &hdr);

bool isAllowedFormat(qint32 pixelFormat);
bool headerFits(const FrameHeader &hdr, qint64 slotBytes);

// A fresh frame copied out of `slot`, or an invalid QVideoFrame to drop.
QVideoFrame unpack(const FrameHeader &hdr, const uchar *slot, qint64 slotBytes);

}

#endif
