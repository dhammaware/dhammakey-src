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

#include "Guard.h"
#include <inputscope.h>
#include <oleacc.h>
#include <wchar.h>
#include <stdio.h>

namespace guard {

const wchar_t* KindName(int kind) {
    switch (kind) {
        case PASSWORD: return L"Ô mật khẩu";
        case URL:      return L"Thanh địa chỉ";
        case CONSOLE:  return L"Dòng lệnh";
        case ASCII:    return L"Ô email / số";
        default:       return L"";
    }
}

static bool IsConsoleExe(const wchar_t* name) {
    static const wchar_t* const kExe[] = {
        L"cmd.exe", L"powershell.exe", L"pwsh.exe", L"conhost.exe", L"openconsole.exe",
        L"windowsterminal.exe", L"wsl.exe", L"bash.exe", L"mintty.exe", L"putty.exe",
        L"kitty.exe", L"alacritty.exe", L"wezterm-gui.exe", L"conemu.exe", L"conemu64.exe",
        L"tabby.exe", L"hyper.exe"};
    for (const wchar_t* e : kExe)
        if (_wcsicmp(name, e) == 0) return true;
    return false;
}

bool IsConsoleProcess() {
    static int cached = -1;
    if (cached < 0) {
        wchar_t path[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        const wchar_t* name = wcsrchr(path, L'\\');
        cached = IsConsoleExe(name ? name + 1 : path) ? 1 : 0;
    }
    return cached == 1;
}

static bool IsConsoleClass(const wchar_t* cls) {
    return wcscmp(cls, L"ConsoleWindowClass") == 0 || wcscmp(cls, L"CASCADIA_HOSTING_WINDOW_CLASS") == 0 ||
           wcscmp(cls, L"PseudoConsoleWindow") == 0 || wcscmp(cls, L"mintty") == 0 ||
           wcscmp(cls, L"PuTTY") == 0 || wcscmp(cls, L"VirtualConsoleClass") == 0;
}

int FromFocusWindow() {
    if (IsConsoleProcess()) return CONSOLE;
    GUITHREADINFO gi = {sizeof(gi)};
    if (!GetGUIThreadInfo(GetCurrentThreadId(), &gi) || !gi.hwndFocus) return NONE;
    wchar_t cls[64] = {};
    GetClassNameW(gi.hwndFocus, cls, 64);
    if ((_wcsicmp(cls, L"Edit") == 0 || wcsstr(cls, L"EDIT") || wcsstr(cls, L"Edit")) &&
        (GetWindowLongW(gi.hwndFocus, GWL_STYLE) & ES_PASSWORD))
        return PASSWORD;
    HWND root = GetAncestor(gi.hwndFocus, GA_ROOT);
    if (root && GetClassNameW(root, cls, 64) && IsConsoleClass(cls)) return CONSOLE;
    return NONE;
}

// Lấy IAccessible của một VARIANT trả về từ get_accFocus. Trả về nullptr nếu không có.
static IAccessible* AccFromVariant(IAccessible* parent, const VARIANT& v, LONG& childId) {
    childId = CHILDID_SELF;
    if (v.vt == VT_DISPATCH && v.pdispVal) {
        IAccessible* a = nullptr;
        if (SUCCEEDED(v.pdispVal->QueryInterface(IID_IAccessible, (void**)&a))) return a;
        return nullptr;
    }
    if (v.vt == VT_I4) {
        parent->AddRef();
        childId = v.lVal;
        return parent;
    }
    return nullptr;
}

// Tên ô trợ năng có giống thanh địa chỉ của trình duyệt không (0.3.34). Chrome/Edge đặt tên kiểu
// "Address and search bar" / "Thanh địa chỉ và tìm kiếm"; so không phân biệt hoa thường.
static bool LooksLikeAddressBar(const wchar_t* name) {
    std::wstring n = name;
    CharLowerBuffW(&n[0], (DWORD)n.size());
    return n.find(L"address") != std::wstring::npos ||
           n.find(L"\u0111\u1ecba ch\u1ec9") != std::wstring::npos ||   // "địa chỉ"
           n.find(L"search or type") != std::wstring::npos;
}

int FocusIsProtected(std::wstring& info) {
    GUITHREADINFO gi = {sizeof(gi)};
    if (!GetGUIThreadInfo(GetCurrentThreadId(), &gi) || !gi.hwndFocus) { info += L"msaa:nohwnd "; return -1; }
    wchar_t cls[64] = {};
    GetClassNameW(gi.hwndFocus, cls, 64);
    info += L"hwnd:"; info += cls; info += L" ";
    IAccessible* cur = nullptr;
    if (FAILED(AccessibleObjectFromWindow(gi.hwndFocus, (DWORD)OBJID_CLIENT, IID_IAccessible, (void**)&cur)) || !cur) {
        info += L"msaa:noacc ";
        return -1;
    }
    // Đi xuống theo get_accFocus: cửa sổ → trang (tài liệu, role 15) → … → ô đang gõ.
    // 0.3.27 dừng ở tầng đầu nên chỉ thấy "cả trang" (đã thử trên máy thật ở trang đăng nhập Microsoft). (0.3.28)
    int result = -1;
    bool sawDocument = false;   // đã đi qua phần trang web (role 15)? Ô trên trang không được coi là thanh địa chỉ
    LONG id = CHILDID_SELF;
    for (int depth = 0; depth < 12 && cur; ++depth) {
        VARIANT self; VariantInit(&self); self.vt = VT_I4; self.lVal = id;
        VARIANT st; VariantInit(&st);
        VARIANT role; VariantInit(&role);
        cur->get_accState(self, &st);
        cur->get_accRole(self, &role);
        long r = role.vt == VT_I4 ? (long)role.lVal : -1L;
        long sv = st.vt == VT_I4 ? (long)st.lVal : 0L;
        wchar_t buf[48];
        swprintf(buf, 48, L"[%d:r%ld s%lX] ", depth, r, (unsigned long)sv);
        info += buf;
        VariantClear(&role);
        VariantClear(&st);
        if (sv & STATE_SYSTEM_PROTECTED) { result = 1; break; }
        if (r == 15 /* ROLE_SYSTEM_DOCUMENT */) sawDocument = true;
        if (r == ROLE_SYSTEM_TEXT) {
            result = 0;
            if (!sawDocument) {   // ô chữ nằm ngoài trang web: có thể là thanh địa chỉ (cửa sổ ẩn danh báo IS_PRIVATE như ô thường)
                BSTR nm = nullptr;
                if (SUCCEEDED(cur->get_accName(self, &nm)) && nm) {
                    info += L"name:"; info += nm; info += L" ";
                    if (LooksLikeAddressBar(nm)) result = 2;
                    SysFreeString(nm);
                }
            }
            break;
        }
        if (id != CHILDID_SELF) break;          // phần tử đơn giản, không đi xuống được nữa
        VARIANT f; VariantInit(&f);
        if (FAILED(cur->get_accFocus(&f)) || f.vt == VT_EMPTY) { VariantClear(&f); break; }
        if (f.vt == VT_I4 && f.lVal == CHILDID_SELF) { VariantClear(&f); break; }   // chính nó đang có con trỏ
        LONG nid = CHILDID_SELF;
        IAccessible* next = AccFromVariant(cur, f, nid);
        VariantClear(&f);
        cur->Release();
        cur = next;
        id = nid;
    }
    if (cur) cur->Release();
    return result;
}

int FromInputScope(int scope) {
    switch (scope) {
        case IS_PASSWORD:
        case 63: /* IS_NUMERIC_PASSWORD */
        case 64: /* IS_NUMERIC_PIN */
        case 65: /* IS_ALPHANUMERIC_PIN */
        case 66: /* IS_ALPHANUMERIC_PIN_SET */
            return PASSWORD;
        case IS_URL:
            return URL;
        case IS_EMAIL_USERNAME:
        case IS_EMAIL_SMTPEMAILADDRESS:
        case IS_DIGITS:
        case IS_NUMBER:
        case IS_TELEPHONE_FULLTELEPHONENUMBER:
        case IS_TELEPHONE_COUNTRYCODE:
        case IS_TELEPHONE_AREACODE:
        case IS_TELEPHONE_LOCALNUMBER:
            return ASCII;
        default:
            return NONE;
    }
}

} // namespace guard
