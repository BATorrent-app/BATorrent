// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/security/crashhandler.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <cstdio>
#include <cstring>
#include <ctime>

#if defined(Q_OS_WIN)
#include <windows.h>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <exception>
#else
#include <csignal>
#include <execinfo.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace {

char g_dir[1024] = {0};
char g_version[64] = {0};

#if !defined(Q_OS_WIN)
void writeStr(int fd, const char *s) { ssize_t r = ::write(fd, s, std::strlen(s)); (void)r; }

void posixHandler(int sig)
{
    char path[1200];
    std::snprintf(path, sizeof(path), "%s/crash-%ld.log", g_dir, long(::time(nullptr)));
    int fd = ::open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd >= 0) {
        writeStr(fd, "BATorrent crash report\nversion: ");
        writeStr(fd, g_version);
        writeStr(fd, "\nsignal: ");
        writeStr(fd, sig == SIGSEGV ? "SIGSEGV" : sig == SIGABRT ? "SIGABRT"
                   : sig == SIGFPE ? "SIGFPE" : sig == SIGILL ? "SIGILL"
                   : sig == SIGBUS ? "SIGBUS" : "?");
        writeStr(fd, "\nbacktrace:\n");
        void *frames[64];
        int n = ::backtrace(frames, 64);
        ::backtrace_symbols_fd(frames, n, fd);
        ::close(fd);
    }
    ::signal(sig, SIG_DFL);
    ::raise(sig);
}
#else
volatile LONG g_reported = 0;

// SetUnhandledExceptionFilter only sees SEH. An uncaught C++ exception goes
// terminate -> abort, and MSVC's invalid-parameter and purecall paths kill the
// process outright: none of them reach the filter, which is why a crash report
// could arrive with no frames at all.
void winWriteReport(const char *reason, unsigned long code)
{
    if (InterlockedExchange(&g_reported, 1) != 0) return;

    // Allocation-light: no SymInitialize/dbghelp, since the loader lock or the
    // heap may already be gone. Raw addresses only; WER/Crashpad still run.
    char path[1200];
    std::snprintf(path, sizeof(path), "%s\\crash-%lld.log", g_dir, (long long)::time(nullptr));
    HANDLE file = CreateFileA(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;

    char header[256];
    int n = std::snprintf(header, sizeof(header),
                          "BATorrent crash report\nversion: %s\nreason: %s\ncode: 0x%lx\nframes:\n",
                          g_version, reason, code);
    DWORD written = 0;
    if (n > 0) WriteFile(file, header, DWORD(n), &written, nullptr);
    void *frames[64];
    USHORT count = CaptureStackBackTrace(0, 64, frames, nullptr);
    for (USHORT i = 0; i < count; ++i) {
        char line[64];
        int ln = std::snprintf(line, sizeof(line), "  0x%p\n", frames[i]);
        if (ln > 0) WriteFile(file, line, DWORD(ln), &written, nullptr);
    }
    CloseHandle(file);
}

LONG WINAPI winHandler(EXCEPTION_POINTERS *info)
{
    winWriteReport("unhandled-exception",
                   info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionCode : 0ul);
    return EXCEPTION_CONTINUE_SEARCH;   // let WER / Crashpad handle termination
}

void winTerminate()
{
    winWriteReport("terminate", 0);
    std::abort();
}

void winAbort(int sig)
{
    winWriteReport("abort", 0);
    ::signal(sig, SIG_DFL);
    ::raise(sig);
}

#if defined(_MSC_VER)
void winPurecall() { winWriteReport("purecall", 0); }

void winInvalidParameter(const wchar_t *, const wchar_t *, const wchar_t *,
                         unsigned int, uintptr_t)
{
    winWriteReport("invalid-parameter", 0);
    std::abort();
}
#endif
#endif

} // namespace

namespace CrashHandler {

void install(const QString &crashDir, const QString &version)
{
    QDir().mkpath(crashDir);
    const QByteArray d = QFile::encodeName(crashDir);
    std::strncpy(g_dir, d.constData(), sizeof(g_dir) - 1);
    const QByteArray v = version.toUtf8();
    std::strncpy(g_version, v.constData(), sizeof(g_version) - 1);

#if defined(Q_OS_WIN)
    SetUnhandledExceptionFilter(winHandler);
    std::set_terminate(winTerminate);
    ::signal(SIGABRT, winAbort);
#if defined(_MSC_VER)
    _set_purecall_handler(winPurecall);
    _set_invalid_parameter_handler(winInvalidParameter);
#endif
#else
    for (int s : { SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS })
        ::signal(s, posixHandler);
#endif
}

QString lastReport(const QString &crashDir)
{
    QDir dir(crashDir);
    const auto reports = dir.entryInfoList({QStringLiteral("crash-*.log")},
                                           QDir::Files, QDir::Time);
    if (reports.isEmpty()) return {};
    QFile f(reports.first().absoluteFilePath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    return QString::fromUtf8(f.readAll());
}

void clearReports(const QString &crashDir)
{
    QDir dir(crashDir);
    for (const QFileInfo &fi : dir.entryInfoList({QStringLiteral("crash-*.log")}, QDir::Files))
        QFile::remove(fi.absoluteFilePath());
}

} // namespace CrashHandler
