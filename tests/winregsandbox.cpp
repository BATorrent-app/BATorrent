// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// Linked into every test exe. On Windows QSettings is the registry, and test
// mode does not redirect it the way it redirects ~/.config on Linux/macOS, so
// the suites' QSettings("BATorrent", "BATorrent").clear() would wipe the real
// app's settings. Before main() runs, HKCU is remapped for this process to a
// throwaway key of its own (one per process, so ctest --parallel runs don't
// share state), and that key is deleted on exit.

#ifdef _WIN32

#include <windows.h>
#include <cstdio>
#include <cstdlib>

namespace {

wchar_t g_path[64];

void dropSandbox()
{
    RegOverridePredefKey(HKEY_CURRENT_USER, nullptr);
    RegDeleteTreeW(HKEY_CURRENT_USER, g_path);
}

const bool g_installed = [] {
    swprintf(g_path, 64, L"Software\\BATorrentTests\\%lu", GetCurrentProcessId());
    HKEY key = nullptr;
    // A leftover from a killed run with a reused pid starts clean too.
    RegDeleteTreeW(HKEY_CURRENT_USER, g_path);
    if (RegCreateKeyExW(HKEY_CURRENT_USER, g_path, 0, nullptr, 0, KEY_ALL_ACCESS,
                        nullptr, &key, nullptr) != ERROR_SUCCESS) {
        // Better no tests than tests that write to the real settings.
        fputs("winregsandbox: cannot create the sandbox key\n", stderr);
        std::abort();
    }
    if (RegOverridePredefKey(HKEY_CURRENT_USER, key) != ERROR_SUCCESS) {
        fputs("winregsandbox: cannot remap HKEY_CURRENT_USER\n", stderr);
        std::abort();
    }
    RegCloseKey(key);
    std::atexit(dropSandbox);
    return true;
}();

} // namespace

#endif
