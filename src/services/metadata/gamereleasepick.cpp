// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/metadata/gamereleasepick.h"

#include <QRegularExpression>

namespace GameReleasePick {

QString parseVersion(const QString &title)
{
    // Group 1 = "v1.2.3"; group 2 = bare "3.2.2" (needs a dot so years aren't versions).
    static const QRegularExpression re(
        QStringLiteral(
            "(?:^|[.\\s\\-_\\[\\(])"
            "(?:v(\\d+(?:\\.\\d+){0,3})|(\\d+\\.\\d+(?:\\.\\d+){0,2}))"
            "(?=[.\\s\\-_\\]\\)]|$)"),
        QRegularExpression::CaseInsensitiveOption);
    const auto m = re.match(title);
    if (!m.hasMatch())
        return {};
    const QString tagged = m.captured(1);
    return tagged.isEmpty() ? m.captured(2) : tagged;
}

int compareVersions(const QString &a, const QString &b)
{
    // Empty build = oldest. Same idea as Python: pad missing parts with 0.
    if (a.isEmpty() || b.isEmpty())
        return a.isEmpty() == b.isEmpty() ? 0 : (a.isEmpty() ? -1 : 1);

    const QStringList pa = a.split(QLatin1Char('.'));
    const QStringList pb = b.split(QLatin1Char('.'));
    for (int i = 0, n = qMax(pa.size(), pb.size()); i < n; ++i) {
        const int va = i < pa.size() ? pa[i].toInt() : 0;
        const int vb = i < pb.size() ? pb[i].toInt() : 0;
        if (va != vb)
            return va < vb ? -1 : 1;
    }
    return 0;
}

int compareCandidates(const Candidate &a, const Candidate &b)
{
    if (a.hasUri != b.hasUri)
        return a.hasUri ? 1 : -1;

    // Newer build wins even if it came from a public indexer — catalog-first
    // must not hide a strictly fresher BitSearch hit.
    const int ver = compareVersions(a.version, b.version);
    if (ver != 0) return ver;

    if (a.fromCatalog != b.fromCatalog)
        return a.fromCatalog ? 1 : -1;

    if (a.uploadDate != b.uploadDate) {
        if (a.uploadDate.isEmpty()) return -1;
        if (b.uploadDate.isEmpty()) return 1;
        return a.uploadDate < b.uploadDate ? -1 : 1;
    }

    if (a.seeders != b.seeders)
        return a.seeders < b.seeders ? -1 : 1;
    return 0;
}

int best(const QList<Candidate> &cands)
{
    int bestIdx = -1;
    for (int i = 0; i < cands.size(); ++i) {
        if (!cands[i].hasUri) continue;
        if (bestIdx < 0 || compareCandidates(cands[i], cands[bestIdx]) > 0)
            bestIdx = i;
    }
    return bestIdx;
}

}
