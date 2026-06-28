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
#include <dbghelp.h>
#else
#include <csignal>
#include <execinfo.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace {

// Captured at install() so the signal/exception handler touches no Qt and no heap.
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
        ::backtrace_symbols_fd(frames, n, fd);   // async-signal-safe
        ::close(fd);
    }
    ::signal(sig, SIG_DFL);   // let the OS produce its own report / core dump
    ::raise(sig);
}
#else
LONG WINAPI winHandler(EXCEPTION_POINTERS *info)
{
    char path[1200];
    std::snprintf(path, sizeof(path), "%s\\crash-%lld.log", g_dir, (long long)::time(nullptr));
    FILE *f = std::fopen(path, "w");
    if (f) {
        std::fprintf(f, "BATorrent crash report\nversion: %s\ncode: 0x%lx\n",
                     g_version, info->ExceptionRecord->ExceptionCode);
        HANDLE proc = GetCurrentProcess();
        SymInitialize(proc, nullptr, TRUE);
        void *frames[64];
        USHORT n = CaptureStackBackTrace(0, 64, frames, nullptr);
        char buf[sizeof(SYMBOL_INFO) + 256];
        auto *sym = reinterpret_cast<SYMBOL_INFO *>(buf);
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = 255;
        for (USHORT i = 0; i < n; ++i) {
            DWORD64 addr = reinterpret_cast<DWORD64>(frames[i]);
            DWORD64 disp = 0;
            if (SymFromAddr(proc, addr, &disp, sym))
                std::fprintf(f, "  %s + 0x%llx\n", sym->Name, (unsigned long long)disp);
            else
                std::fprintf(f, "  0x%llx\n", (unsigned long long)addr);
        }
        std::fclose(f);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
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
