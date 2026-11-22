/*
 * SPDX-FileCopyrightText: 2026 Ujudhammo
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
 *
 * DhammaKey - a DhammaWare project
 * Homepage: https://dhammaware.org
 *
 * Copyright (C) 2026 Ujudhammo.
 *
 * TERMS OF USE AND NONCOMMERCIAL COMMITMENT:
 * This software is offered as dana (a free gift) to serve the community, to
 * support study and research, and to help spread the Dhamma. You may use, read,
 * study, modify and contribute improvements to this source code for
 * noncommercial purposes.
 *
 * PROHIBITED: copying, packaging, redistributing or using this source code or
 * any derivative work for commercial purposes, for sale, or for any fee.
 *
 * The full, legally binding terms are in the LICENSE file at the project root.
 * Third-party components keep their own licenses; see THIRD-PARTY-NOTICES.txt.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DAMAGES ARISING FROM ITS USE.
 */

#include "Conflict.h"
#include <tlhelp32.h>
#include <shlobj.h>
#include <VersionHelpers.h>
#include <cwctype>
#include <vector>

namespace conflict {
namespace {

// Chuỗi nhận dạng (chữ thường) → tên hiển thị. Thêm bộ gõ mới chỉ cần thêm một dòng.
struct Known { const wchar_t* key; const wchar_t* name; };
const Known kKnown[] = {
    {L"unikey",      L"UniKey"},
    {L"evkey",       L"EVKey"},       // phải đứng trước "vkey"
    {L"gotiengviet", L"GoTiengViet"},
    {L"openkey",     L"OpenKey"},
    {L"vietkey",     L"Vietkey"},
    {L"vk2000",      L"Vietkey"},
    {L"labankey",    L"Laban Key"},
    {L"vpskey",      L"VPSKeys"},
    {L"winvnkey",    L"WinVNKey"},
    {L"tocky",       L"TocKyVNKey"},
    {L"fvik",        L"FVIK"},
    {L"vkey.exe",    L"VKey"},        // bộ gõ mã nguồn mở VKey
};

std::wstring Lower(std::wstring s) {
    for (auto& c : s) c = (wchar_t)towlower(c);
    return s;
}

const wchar_t* Match(const std::wstring& text) {
    std::wstring l = Lower(text);
    for (const Known& k : kKnown)
        if (l.find(k.key) != std::wstring::npos) return k.name;
    return nullptr;
}

template <class F> void ForEachProcess(F f) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;
    PROCESSENTRY32W pe = {};
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do { f(pe); } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
}

const wchar_t kRun[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const wchar_t kApprovedRun[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run";
const wchar_t kApprovedRun32[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run32";
const wchar_t kApprovedFolder[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\StartupFolder";

// Một chỗ Windows đọc để tự chạy chương trình khi mở máy
struct RunKey { HKEY root; REGSAM view; const wchar_t* approved; };
const RunKey kRunKeys[] = {
    {HKEY_CURRENT_USER, 0, kApprovedRun},
    {HKEY_LOCAL_MACHINE, KEY_WOW64_64KEY, kApprovedRun},
    {HKEY_LOCAL_MACHINE, KEY_WOW64_32KEY, kApprovedRun32},
};

struct Entry {
    const RunKey* rk = nullptr;  // nullptr: lối tắt trong thư mục Startup
    std::wstring value;          // tên giá trị Run, hoặc tên tệp .lnk
    std::wstring path;           // đường dẫn đầy đủ của tệp .lnk
    const wchar_t* name = nullptr;
};

// Windows 8+: Task Manager đánh dấu "Disabled" ở StartupApproved (byte đầu lẻ = tắt)
// StartupApproved luôn nằm ở nhánh 64-bit (bản DhammaKey 32-bit cũng phải đọc đúng chỗ)
bool IsApprovedOff(HKEY root, const wchar_t* key, const std::wstring& value) {
    HKEY k;
    if (RegOpenKeyExW(root, key, 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &k) != ERROR_SUCCESS) return false;
    BYTE b[12] = {};
    DWORD sz = sizeof(b), type = 0;
    bool off = RegQueryValueExW(k, value.c_str(), nullptr, &type, b, &sz) == ERROR_SUCCESS &&
               type == REG_BINARY && sz > 0 && (b[0] & 1);
    RegCloseKey(k);
    return off;
}

bool SetApprovedOff(HKEY root, const wchar_t* key, const std::wstring& value) {
    HKEY k;
    if (RegCreateKeyExW(root, key, 0, nullptr, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, nullptr, &k, nullptr) != ERROR_SUCCESS)
        return false;
    BYTE b[12] = {3};
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    memcpy(b + 4, &ft, sizeof(ft));
    bool ok = RegSetValueExW(k, value.c_str(), 0, REG_BINARY, b, sizeof(b)) == ERROR_SUCCESS;
    RegCloseKey(k);
    return ok;
}

std::vector<Entry> FindEntries() {
    std::vector<Entry> out;
    bool win8 = IsWindows8OrGreater();
    for (const RunKey& rk : kRunKeys) {
        HKEY k;
        if (RegOpenKeyExW(rk.root, kRun, 0, KEY_READ | rk.view, &k) != ERROR_SUCCESS) continue;
        for (DWORD i = 0;; ++i) {
            wchar_t name[512];
            DWORD nameLen = 512, type = 0, dataLen = 0;
            BYTE data[2048];
            dataLen = sizeof(data) - sizeof(wchar_t);
            LONG r = RegEnumValueW(k, i, name, &nameLen, nullptr, &type, data, &dataLen);
            if (r == ERROR_NO_MORE_ITEMS) break;
            if (r != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) continue;
            data[dataLen] = data[dataLen + 1] = 0;
            const wchar_t* who = Match((const wchar_t*)data);
            if (!who) who = Match(name);
            if (!who) continue;
            if (win8 && IsApprovedOff(rk.root, rk.approved, name)) continue;
            Entry e;
            e.rk = &rk;
            e.value = name;
            e.name = who;
            out.push_back(e);
        }
        RegCloseKey(k);
    }
    // Lối tắt trong thư mục Startup của người dùng
    wchar_t dir[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_STARTUP, nullptr, 0, dir))) {
        std::wstring pattern = std::wstring(dir) + L"\\*.lnk";
        WIN32_FIND_DATAW fd;
        HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                const wchar_t* who = Match(fd.cFileName);
                if (!who) continue;
                if (win8 && IsApprovedOff(HKEY_CURRENT_USER, kApprovedFolder, fd.cFileName)) continue;
                Entry e;
                e.value = fd.cFileName;
                e.path = std::wstring(dir) + L"\\" + fd.cFileName;
                e.name = who;
                out.push_back(e);
            } while (FindNextFileW(h, &fd));
            FindClose(h);
        }
    }
    return out;
}

// Windows 7 không có StartupApproved: cất giá trị Run sang HKCU\Software\DhammaKey\DisabledRun
bool MoveRunValueWin7(const Entry& e) {
    if (e.rk->root != HKEY_CURRENT_USER) return false;
    HKEY run, backup;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRun, 0, KEY_READ | KEY_SET_VALUE, &run) != ERROR_SUCCESS) return false;
    bool ok = false;
    BYTE data[2048];
    DWORD type = 0, len = sizeof(data);
    if (RegQueryValueExW(run, e.value.c_str(), nullptr, &type, data, &len) == ERROR_SUCCESS &&
        RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\DhammaKey\\DisabledRun", 0, nullptr, 0, KEY_SET_VALUE,
                        nullptr, &backup, nullptr) == ERROR_SUCCESS) {
        if (RegSetValueExW(backup, e.value.c_str(), 0, type, data, len) == ERROR_SUCCESS)
            ok = RegDeleteValueW(run, e.value.c_str()) == ERROR_SUCCESS;
        RegCloseKey(backup);
    }
    RegCloseKey(run);
    return ok;
}

} // namespace

std::wstring FindRunning() {
    std::wstring found;
    ForEachProcess([&](const PROCESSENTRY32W& pe) {
        if (found.empty()) {
            if (const wchar_t* who = Match(pe.szExeFile)) found = who;
        }
    });
    return found;
}

std::wstring FindAutostart() {
    std::vector<Entry> v = FindEntries();
    return v.empty() ? std::wstring() : std::wstring(v[0].name);
}

Result DisableAndStop() {
    Result r;
    bool win8 = IsWindows8OrGreater();
    for (const Entry& e : FindEntries()) {
        bool ok;
        if (!e.rk) {
            ok = win8 ? SetApprovedOff(HKEY_CURRENT_USER, kApprovedFolder, e.value)
                      : MoveFileW(e.path.c_str(), (e.path + L".disabled").c_str()) != FALSE;
        } else {
            ok = win8 ? SetApprovedOff(e.rk->root, e.rk->approved, e.value) : MoveRunValueWin7(e);
        }
        ok ? ++r.disabled : ++r.failed;
    }
    DWORD self = GetCurrentProcessId();
    ForEachProcess([&](const PROCESSENTRY32W& pe) {
        if (pe.th32ProcessID == self || !Match(pe.szExeFile)) return;
        HANDLE p = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
        if (p && TerminateProcess(p, 0)) ++r.stopped;
        else ++r.stopFailed;
        if (p) CloseHandle(p);
    });
    return r;
}

} // namespace conflict
