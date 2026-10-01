#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdio.h>
#include <wchar.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    WCHAR selfPath[MAX_PATH];
    if (!GetModuleFileNameW(NULL, selfPath, MAX_PATH)) {
        return 1;
    }

    // selfPath is e.g. D:\...\Coverage Analysis\bin\coverage_gui.exe
    WCHAR* lastSlash = wcsrchr(selfPath, L'\\');
    if (lastSlash) *lastSlash = L'\0'; // removes \coverage_gui.exe
    lastSlash = wcsrchr(selfPath, L'\\');
    if (lastSlash) *lastSlash = L'\0'; // removes \bin, now at root workspace

    WCHAR targetExe[MAX_PATH];
    swprintf(targetExe, MAX_PATH, L"%ls\\build\\bin\\coverage_gui.exe", selfPath);

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
    ZeroMemory(&pi, sizeof(pi));

    if (CreateProcessW(NULL, cmdBuffer, NULL, NULL, FALSE, 0, NULL, selfPath, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return 0;
    }

    // Try relative fallback if launched from build/executables/
    WCHAR fallbackExe[MAX_PATH];
    swprintf(fallbackExe, MAX_PATH, L"%ls\\coverage_gui.exe", dllDir);
    WCHAR fallbackCmd[32768];
    swprintf(fallbackCmd, 32768, L"\"%ls\" %ls", fallbackExe, args);
    if (CreateProcessW(NULL, fallbackCmd, NULL, NULL, FALSE, 0, NULL, selfPath, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return 0;
    }

    MessageBoxW(NULL, L"无法定位目标核心程序 build\\bin\\coverage_gui.exe，请确保工程已完成编译。",
                L"启动失败 - Embedded C Coverage Studio", MB_OK | MB_ICONERROR);
    return 1;
}
