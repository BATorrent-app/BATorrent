// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// libFuzzer harness for the UI side of the isolated decoder: a frame header
// and slot bytes exactly as a compromised child could send them. Build with
// -DBAT_FUZZ=ON and run: ./fuzz_mediaframe -max_total_time=30

#include "ipc/mediaframe.h"
#include <QByteArray>
#include <QDataStream>
#include <cstdint>
#include <cstddef>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    const QByteArray in(reinterpret_cast<const char *>(data), static_cast<qsizetype>(size));
    QDataStream ds(in);
    ds.setVersion(ipc::kStreamVersion);
    media::FrameHeader hdr;
    ds >> hdr;
    const QByteArray slot = in.mid(int(ds.device()->pos()));
    media::unpack(hdr, reinterpret_cast<const uchar *>(slot.constData()), slot.size());
    return 0;
}
