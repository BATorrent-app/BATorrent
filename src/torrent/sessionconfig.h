// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include "torrent/types.h"

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QtGlobal>
#include <string>
#include <vector>
#include <map>

namespace libtorrent { struct settings_pack; }
class QSettings;

// Pure session-config helpers + AdvancedSettings load/apply. SessionManager
// owns the live knobs / lt::session; these keep policy bits testable.
namespace SessionConfig {

QString listenInterfaces(const QString &listenAddr, int port, bool forceIpv4);

QString expandOnCompleteCommand(const QString &tmpl,
                                const QString &name,
                                const QString &savePath,
                                const QString &hash,
                                qint64 totalSize);

QStringList parseExtractPasswords(const QString &raw);

qint64 autoCompleteSecondsFromIndex(int index);

int portStatusCode(bool listenOk, bool portmapOk);

std::map<int, std::string> planContentLayout(int mode,
                                             const std::string &torrentName,
                                             const std::vector<std::string> &filePaths);

AdvancedSettings loadAdvanced(const QSettings &s);
void persistAdvanced(QSettings &s, const AdvancedSettings &a);
void fillAdvancedPack(libtorrent::settings_pack &pack, const AdvancedSettings &a);

// Mutates `a` when key is a known adv* setting. Returns false if unknown.
bool patchAdvancedKey(AdvancedSettings &a, const QString &key, const QVariant &v);

// File indexes whose path matches any valid regex in `patterns` (case-insensitive).
// Invalid patterns are skipped. Empty patterns → empty result.
std::vector<int> excludedFileIndexes(const QStringList &patterns,
                                     const QStringList &filePaths);

} // namespace SessionConfig
