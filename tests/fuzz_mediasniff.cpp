// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// libFuzzer harness for the container sniffer: it reads the head of every
// downloaded video, which is attacker-controlled bytes. Build with
// -DBAT_FUZZ=ON and run: ./fuzz_mediasniff -max_total_time=30

#include "services/security/mediasniff.h"
#include <QByteArray>
#include <cstdint>
#include <cstddef>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    const QByteArray head(reinterpret_cast<const char *>(data), static_cast<qsizetype>(size));
    MediaSniff::identify(head);
    MediaSniff::asfHasLure(head);
    MediaSniff::judge(head, head.size());
    return 0;
}
