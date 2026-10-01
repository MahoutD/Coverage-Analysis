#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdio.h>
#include <wchar.h>

int wmain(int argc, wchar_t *argv[]) {
    (void)argc;
    (void)argv;

    WCHAR selfPath[MAX_PATH];
    if (!GetModuleFileNameW(NULL, selfPath, MAX_PATH)) {
        return 1;
    }

    // selfPath is e.g. D:\...\Coverage Analysis\bin\coverage_cli.exe
    WCHAR* lastSlash = wcsrchr(selfPath, L'\\');
    if (lastSlash) *lastSlash = L'\0'; // removes \coverage_cli.exe
    lastSlash = wcsrchr(selfPath, L'\\');
    if (lastSlash) *lastSlash = L'\0'; // removes \bin, now at root workspace

    WCHAR targetExe[MAX_PATH];
    swprintf(targetExe, MAX_PATH, L"%ls\\build\\bin\\coverage_cli.exe", selfPath);

    WCHAR dllDir[MAX_PATH];
    swprintf(dllDir, MAX_PATH, L"%ls\\build\\bin", selfPath);

    // Prepend dllDir to PATH
    WCHAR oldPath[32768];
    DWORD oldLen = GetEnvironmentVariableW(L"PATH", oldPath, 32768);
    if (oldLen > 0 && oldLen < 32000) {
        WCHAR newPath[32768];
        swprintf(newPath, 32768, L"%ls;%ls", dllDir, oldPath);
        SetEnvironmentVariableW(L"PATH", newPath);
    } else {
        SetEnvironmentVariableW(L"PATH", dllDir);
    }
    SetDllDirectoryW(dllDir);

    // Find the arguments after the first token in GetCommandLineW()
    LPWSTR rawCmd = GetCommandLineW();
    LPWSTR args = L"";
    if (rawCmd) {
        if (*rawCmd == L'"') {
            rawCmd++;
            while (*rawCmd && *rawCmd != L'"') rawCmd++;
            if (*rawCmd == L'"') rawCmd++;
        } else {
            while (*rawCmd && *rawCmd != L' ' && *rawCmd != L'\t') rawCmd++;
        }
        while (*rawCmd == L' ' || *rawCmd == L'\t') rawCmd++;
        args = rawCmd;
    }

    WCHAR cmdBuffer[32768];
    swprintf(cmdBuffer, 32768, L"\"%ls\" %ls", targetExe, args);

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessW(NULL, cmdBuffer, NULL, NULL, TRUE, 0, NULL, selfPath, &si, &pi)) {
        fwprintf(stderr, L"Error: Unable to launch target executable: %ls\n", targetExe);
        return 1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return (int)exitCode;
}
