// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The project builds against _WIN32_WINNT=0x0601, which hides every API used
// here (Windows 8/10). Qt 6 already requires Windows 10, so this file asks for it.
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00

#include "ipc/appcontainer_win.h"

#include <QDir>
#include <windows.h>
#include <aclapi.h>
#include <sddl.h>
#include <userenv.h>

namespace {

const wchar_t kProfile[] = L"BATorrent.Media";

bool hasGrant(PACL dacl, PSID sid)
{
    ULONG n = 0;
    PEXPLICIT_ACCESS_W entries = nullptr;
    if (GetExplicitEntriesFromAclW(dacl, &n, &entries) != ERROR_SUCCESS) return false;
    bool found = false;
    for (ULONG i = 0; i < n && !found; ++i)
        found = entries[i].Trustee.TrusteeForm == TRUSTEE_IS_SID
                && EqualSid(static_cast<PSID>(entries[i].Trustee.ptstrName), sid)
                && entries[i].grfAccessMode == GRANT_ACCESS;
    LocalFree(entries);
    return found;
}

// The container reads nothing it isn't given, its own DLLs included.
bool grantReadExecute(const QString &dir, PSID sid)
{
    const std::wstring path = QDir::toNativeSeparators(dir).toStdWString();
    PACL dacl = nullptr;
    PSECURITY_DESCRIPTOR sd = nullptr;
    if (GetNamedSecurityInfoW(path.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                              nullptr, nullptr, &dacl, nullptr, &sd) != ERROR_SUCCESS)
        return false;
    bool ok = hasGrant(dacl, sid);
    if (!ok) {
        EXPLICIT_ACCESS_W ea{};
        ea.grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE;
        ea.grfAccessMode = GRANT_ACCESS;
        ea.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
        ea.Trustee.ptstrName = static_cast<LPWSTR>(sid);
        PACL updated = nullptr;
        if (SetEntriesInAclW(1, &ea, dacl, &updated) == ERROR_SUCCESS) {
            ok = SetNamedSecurityInfoW(const_cast<LPWSTR>(path.c_str()), SE_FILE_OBJECT,
                                       DACL_SECURITY_INFORMATION, nullptr, nullptr,
                                       updated, nullptr) == ERROR_SUCCESS;
            LocalFree(updated);
        }
    }
    LocalFree(sd);
    return ok;
}

}

struct ContainedLaunch::Native {
    PSID sid = nullptr;
    SECURITY_CAPABILITIES caps{};
    HANDLE job = nullptr;
    DWORD childPolicy = PROCESS_CREATION_CHILD_PROCESS_RESTRICTED;   // a DWORD, not a DWORD64
    LPPROC_THREAD_ATTRIBUTE_LIST list = nullptr;
    STARTUPINFOEXW siex{};
};

ContainedLaunch::ContainedLaunch() : d(std::make_unique<Native>()) {}

ContainedLaunch::~ContainedLaunch()
{
    if (d->list) { DeleteProcThreadAttributeList(d->list); HeapFree(GetProcessHeap(), 0, d->list); }
    if (d->job) CloseHandle(d->job);   // KILL_ON_JOB_CLOSE takes the child with it
    if (d->sid) FreeSid(d->sid);
}

bool ContainedLaunch::prepare(const QString &codeDir, QString *error)
{
    auto fail = [error](const char *why) {
        if (error) *error = QStringLiteral("%1 (error %2)").arg(QLatin1String(why)).arg(GetLastError());
        return false;
    };

    HRESULT hr = CreateAppContainerProfile(kProfile, kProfile, L"BATorrent video decoder", nullptr, 0, &d->sid);
    if (hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
        hr = DeriveAppContainerSidFromAppContainerName(kProfile, &d->sid);
    if (FAILED(hr) || !d->sid) return fail("no AppContainer profile");
    LPWSTR text = nullptr;
    if (ConvertSidToStringSidW(d->sid, &text)) { m_sid = QString::fromWCharArray(text); LocalFree(text); }
    if (m_sid.isEmpty()) return fail("no AppContainer SID");
    if (!grantReadExecute(codeDir, d->sid)) return fail("cannot grant the decoder its own code");

    d->job = CreateJobObjectW(nullptr, nullptr);
    if (!d->job) return fail("no job object");
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
                                              | JOB_OBJECT_LIMIT_ACTIVE_PROCESS
                                              | JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;
    limits.BasicLimitInformation.ActiveProcessLimit = 1;
    JOBOBJECT_BASIC_UI_RESTRICTIONS ui{};
    ui.UIRestrictionsClass = JOB_OBJECT_UILIMIT_DESKTOP | JOB_OBJECT_UILIMIT_DISPLAYSETTINGS
                             | JOB_OBJECT_UILIMIT_EXITWINDOWS | JOB_OBJECT_UILIMIT_GLOBALATOMS
                             | JOB_OBJECT_UILIMIT_READCLIPBOARD | JOB_OBJECT_UILIMIT_WRITECLIPBOARD
                             | JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS;
    if (!SetInformationJobObject(d->job, JobObjectExtendedLimitInformation, &limits, sizeof limits)
        || !SetInformationJobObject(d->job, JobObjectBasicUIRestrictions, &ui, sizeof ui))
        return fail("job limits refused");

    d->caps.AppContainerSid = d->sid;
    SIZE_T size = 0;
    InitializeProcThreadAttributeList(nullptr, 3, 0, &size);
    d->list = static_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(HeapAlloc(GetProcessHeap(), 0, size));
    if (!d->list || !InitializeProcThreadAttributeList(d->list, 3, 0, &size))
        return fail("no attribute list");
    if (!UpdateProcThreadAttribute(d->list, 0, PROC_THREAD_ATTRIBUTE_SECURITY_CAPABILITIES,
                                   &d->caps, sizeof d->caps, nullptr, nullptr)
        || !UpdateProcThreadAttribute(d->list, 0, PROC_THREAD_ATTRIBUTE_JOB_LIST,
                                      &d->job, sizeof d->job, nullptr, nullptr)
        || !UpdateProcThreadAttribute(d->list, 0, PROC_THREAD_ATTRIBUTE_CHILD_PROCESS_POLICY,
                                      &d->childPolicy, sizeof d->childPolicy, nullptr, nullptr))
        return fail("attributes refused");
    return true;
}

QProcess::CreateProcessArgumentModifier ContainedLaunch::modifier()
{
    return [this](QProcess::CreateProcessArguments *a) {
        d->siex.StartupInfo = *a->startupInfo;   // keep QProcess's std handles
        d->siex.StartupInfo.cb = sizeof(STARTUPINFOEXW);
        d->siex.lpAttributeList = d->list;
        a->startupInfo = &d->siex.StartupInfo;
        a->flags |= EXTENDED_STARTUPINFO_PRESENT;
    };
}
