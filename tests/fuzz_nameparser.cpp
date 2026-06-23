// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// libFuzzer harness for the release-name parser — the regex-heavy code that
// runs on every (untrusted) torrent name. Build with -DBAT_FUZZ=ON and run:
//   ./fuzz_nameparser -max_total_time=30
// Catches crashes and ReDoS-style runaways (pair with -timeout=...).

#include "app/nameparser.h"
#include "app/suspiciousscan.h"
#include "app/archivescan.h"
#include <QString>
#include <QStringList>
#include <cstdint>
#include <cstddef>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    const QString s = QString::fromUtf8(reinterpret_cast<const char *>(data), static_cast<int>(size));
    // Three untrusted-input parsers in one harness:
    NameParser::parse(s);                               // release-name parsing
    ArchiveScan::isArchive(s);                          // archive extension heuristic
    SuspiciousScan::scan({ { s, 1024 } });              // malware-awareness heuristic
    return 0;
}
