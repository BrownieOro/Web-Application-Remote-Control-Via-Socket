#include "app_list_start_stop.h"
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <sstream>
#include <algorithm>
#include <functional>

// =====================================================
// 🟦 Helper: Lấy snapshot của tất cả processes
// =====================================================
HANDLE createProcessSnapshot() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    return (snap == INVALID_HANDLE_VALUE) ? NULL : snap;
}

// =====================================================
// 🟦 Helper: Chuẩn hóa tên file → viết thường
//     Ví dụ: "Chrome.EXE" → "chrome.exe"
// =====================================================
std::string toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

// ====================================================
// 🟦 Helper: Duyệt toàn bộ process và áp dụng callback
// =====================================================
void forEachProcess(const std::function<void(const PROCESSENTRY32&)>& fn) {
    HANDLE snap = createProcessSnapshot();
    if (!snap) return;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);

    if (Process32First(snap, &pe)) {
        do {
            fn(pe);
        } while (Process32Next(snap, &pe));
    }

    CloseHandle(snap);
}

// =====================================================
// 🟦 Helper: Get process name as lowercase string
// =====================================================
std::string getProcessNameLower(const PROCESSENTRY32& pe) {
    std::string name(pe.szExeFile, pe.szExeFile + strlen(pe.szExeFile));
    return toLower(name);
}

// =====================================================
// 🟦 1. LIST ALL APPLICATIONS
// =====================================================
std::string listApps() {
    std::ostringstream out;
    out << "=== RUNNING APPLICATIONS ===\n";

    forEachProcess([&out](const PROCESSENTRY32& pe) {
        out << pe.th32ProcessID << " - " << pe.szExeFile << "\n";
    });

    return out.str();
}

// =====================================================
// 🟦 2. START APPLICATION
// =====================================================
std::string startApp(const std::string& app_name) {
    if (app_name.empty())
        return "✗ Invalid app name\n";

    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);

    BOOL ok = CreateProcessA(
        NULL,
        (LPSTR)app_name.c_str(),
        NULL, NULL, FALSE,
        CREATE_NEW_CONSOLE,
        NULL, NULL,
        &si, &pi
    );

    if (!ok)
        return "✗ Failed to start: " + app_name + "\n";

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return "✓ Started: " + app_name + "\n";
}

// =====================================================
// 🟦 3. STOP APPLICATION
//     Tối ưu:
//       - Dùng tên lowercase
//       - Stop nhiều instance
//       - Kill an toàn
// =====================================================
std::string stopApp(const std::string& app_name) {
    if (app_name.empty())
        return "✗ Invalid app name\n";

    std::string target = toLower(app_name);
    int killed = 0;

    forEachProcess([&target, &killed](const PROCESSENTRY32& pe) {
        if (getProcessNameLower(pe) == target) {
            HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
            if (hProc) {
                TerminateProcess(hProc, 0);
                CloseHandle(hProc);
                killed++;
            }
        }
    });

    if (killed == 0)
        return "✗ App not running: " + app_name + "\n";

    return "✓ Stopped " + std::to_string(killed) + " instance(s) of " + app_name + "\n";
}
