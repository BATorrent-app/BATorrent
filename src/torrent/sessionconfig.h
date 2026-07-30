// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QString>
#include <QStringList>
#include <QtGlobal>
#include <string>
#include <vector>
#include <map>

// Pure session-config helpers. SessionManager owns the live knobs / lt::session;
// these keep the policy bits testable and out of the setter soup.
namespace SessionConfig {

// Dual-stack listen_interfaces string, or v4-only when bound / forceIpv4.
QString listenInterfaces(const QString &listenAddr, int port, bool forceIpv4);

// Expand run-on-complete placeholders (%N %D %H %Z %F) with shell-safe quoting.
QString expandOnCompleteCommand(const QString &tmpl,
                                const QString &name,
                                const QString &savePath,
                                const QString &hash,
                                qint64 totalSize);

QStringList parseExtractPasswords(const QString &raw);

// Settings combo index → seconds (0,1,3,7,14,30 days). Out of range → 0.
qint64 autoCompleteSecondsFromIndex(int index);

// 1 open · 2 firewalled/unknown · 3 closed
int portStatusCode(bool listenOk, bool portmapOk);

// Content-layout rename plan. Keys are file indices; values are new relative paths.
// mode: 0 original · 1 create subfolder (single-file) · 2 strip common root.
std::map<int, std::string> planContentLayout(int mode,
                                             const std::string &torrentName,
                                             const std::vector<std::string> &filePaths);

} // namespace SessionConfig
