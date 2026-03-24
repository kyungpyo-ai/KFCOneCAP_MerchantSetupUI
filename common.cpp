#include "stdafx.h"
#include "common.h"

BOOL g_bPendingRestart = FALSE;

// Global flag: set TRUE before intentional exit so ExitInstance() returns 42.

// ----------------------------------------------------------------
// GetExeDirectory
// ----------------------------------------------------------------
CString GetExeDirectory()
{
    TCHAR buf[MAX_PATH] = {};
    ::GetModuleFileName(NULL, buf, MAX_PATH);
    CString path(buf);
    int pos = path.ReverseFind(_T('\\'));
    return (pos >= 0) ? path.Left(pos + 1) : CString(_T(".\\"));
}

// ----------------------------------------------------------------
// LaunchExeInSameDir
// ----------------------------------------------------------------
BOOL LaunchExeInSameDir(LPCTSTR exeName)
{
    if (!exeName || !exeName[0]) return FALSE;

    CString fullPath = GetExeDirectory() + exeName;

    if (::GetFileAttributes(fullPath) == INVALID_FILE_ATTRIBUTES)
        return FALSE;

    STARTUPINFO si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    BOOL ok = ::CreateProcess(
        fullPath, NULL, NULL, NULL, FALSE, 0, NULL,
        GetExeDirectory(), &si, &pi);

    if (ok)
    {
        ::CloseHandle(pi.hThread);
        ::CloseHandle(pi.hProcess);
    }
    return ok;
}

// ----------------------------------------------------------------
// TerminateExeByName
// ----------------------------------------------------------------
BOOL TerminateExeByName(LPCTSTR exeName, DWORD gracePeriodMs)
{
    if (!exeName || !exeName[0]) return FALSE;

    BOOL bFound = FALSE;
    HANDLE hSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return FALSE;

    PROCESSENTRY32 pe = {};
    pe.dwSize = sizeof(pe);

    if (::Process32First(hSnap, &pe))
    {
        do
        {
            if (::lstrcmpi(pe.szExeFile, exeName) != 0) continue;

            bFound = TRUE;
            DWORD targetPid = pe.th32ProcessID;

            HANDLE hProc = ::OpenProcess(
                PROCESS_TERMINATE | SYNCHRONIZE, FALSE, targetPid);
            if (!hProc) continue;

            struct WndCloseCtx { DWORD pid; };
            WndCloseCtx ctx = { targetPid };
            ::EnumWindows([](HWND hwnd, LPARAM lp) -> BOOL {
                DWORD pid = 0;
                ::GetWindowThreadProcessId(hwnd, &pid);
                if (pid == reinterpret_cast<WndCloseCtx*>(lp)->pid)
                    ::PostMessage(hwnd, WM_CLOSE, 0, 0);
                return TRUE;
            }, reinterpret_cast<LPARAM>(&ctx));

            if (::WaitForSingleObject(hProc, gracePeriodMs) != WAIT_OBJECT_0)
                ::TerminateProcess(hProc, 1);

            ::CloseHandle(hProc);

        } while (::Process32Next(hSnap, &pe));
    }
    ::CloseHandle(hSnap);
    return bFound;
}
// ----------------------------------------------------------------
// RestartApplication: exit code 43 tells watchdog to restart
// ----------------------------------------------------------------
void RestartApplication()
{

    g_bPendingRestart = TRUE;
    ::EnumThreadWindows(::GetCurrentThreadId(), [](HWND hwnd, LPARAM) -> BOOL {
        ::PostMessage(hwnd, WM_CLOSE, 0, 0);
        return TRUE;
    }, 0);
}
