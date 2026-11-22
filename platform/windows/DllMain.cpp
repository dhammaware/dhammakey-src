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

// DLL COM: class factory + đăng ký / gỡ đăng ký bộ gõ với Windows (TSF)
#include "Globals.h"
#include "TextService.h"
#include "Conflict.h"
#include <olectl.h>
#include <string>
#include <new>

HINSTANCE g_hInst = nullptr;
LONG g_dllRefs = 0;
void DllAddRef() { InterlockedIncrement(&g_dllRefs); }
void DllRelease() { InterlockedDecrement(&g_dllRefs); }

namespace {

class ClassFactory : public IClassFactory {
public:
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (riid == IID_IUnknown || riid == IID_IClassFactory) { *ppv = this; AddRef(); return S_OK; }
        *ppv = nullptr; return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { DllAddRef(); return 2; }
    STDMETHODIMP_(ULONG) Release() override { DllRelease(); return 1; }
    STDMETHODIMP CreateInstance(IUnknown* outer, REFIID riid, void** ppv) override {
        if (outer) return CLASS_E_NOAGGREGATION;
        TextService* ts = new (std::nothrow) TextService();
        if (!ts) return E_OUTOFMEMORY;
        HRESULT hr = ts->QueryInterface(riid, ppv);
        ts->Release();
        return hr;
    }
    STDMETHODIMP LockServer(BOOL lock) override { lock ? DllAddRef() : DllRelease(); return S_OK; }
};
ClassFactory g_factory;

std::wstring ClsidString() {
    wchar_t buf[64];
    StringFromGUID2(CLSID_DhammaKey, buf, 64);
    return buf;
}

std::wstring ModulePath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(g_hInst, path, MAX_PATH);
    return path;
}

bool RegisterComServer() {
    std::wstring key = L"Software\\Classes\\CLSID\\" + ClsidString();
    HKEY k;
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, key.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &k, nullptr) != ERROR_SUCCESS) return false;
    RegSetValueExW(k, nullptr, 0, REG_SZ, (const BYTE*)DK_DESCRIPTION, sizeof(DK_DESCRIPTION));
    HKEY ip;
    bool ok = RegCreateKeyExW(k, L"InprocServer32", 0, nullptr, 0, KEY_WRITE, nullptr, &ip, nullptr) == ERROR_SUCCESS;
    if (ok) {
        std::wstring p = ModulePath();
        RegSetValueExW(ip, nullptr, 0, REG_SZ, (const BYTE*)p.c_str(), (DWORD)((p.size() + 1) * sizeof(wchar_t)));
        const wchar_t tm[] = L"Apartment";
        RegSetValueExW(ip, L"ThreadingModel", 0, REG_SZ, (const BYTE*)tm, sizeof(tm));
        RegCloseKey(ip);
    }
    RegCloseKey(k);
    return ok;
}

void UnregisterComServer() {
    std::wstring key = L"Software\\Classes\\CLSID\\" + ClsidString();
    RegDeleteTreeW(HKEY_LOCAL_MACHINE, key.c_str());
}

bool RegisterProfile() {
    std::wstring icon = ModulePath();
    ITfInputProcessorProfileMgr* mgr = nullptr;
    // Windows 8 trở lên
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_ITfInputProcessorProfileMgr, (void**)&mgr))) {
        HRESULT hr = mgr->RegisterProfile(CLSID_DhammaKey, DK_LANGID, GUID_DhammaKeyProfile,
                                          DK_DESCRIPTION, (ULONG)wcslen(DK_DESCRIPTION),
                                          icon.c_str(), (ULONG)icon.size(), (UINT)-IDI_DHAMMAKEY,
                                          nullptr, 0, TRUE, 0);
        mgr->Release();
        if (SUCCEEDED(hr)) return true;
    }
    // Windows 7
    ITfInputProcessorProfiles* p = nullptr;
    if (FAILED(CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                IID_ITfInputProcessorProfiles, (void**)&p))) return false;
    HRESULT hr = p->Register(CLSID_DhammaKey);
    if (SUCCEEDED(hr))
        hr = p->AddLanguageProfile(CLSID_DhammaKey, DK_LANGID, GUID_DhammaKeyProfile,
                                   DK_DESCRIPTION, (ULONG)wcslen(DK_DESCRIPTION),
                                   icon.c_str(), (ULONG)icon.size(), (ULONG)-IDI_DHAMMAKEY);
    p->Release();
    return SUCCEEDED(hr);
}

void UnregisterProfile() {
    ITfInputProcessorProfileMgr* mgr = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_ITfInputProcessorProfileMgr, (void**)&mgr))) {
        mgr->UnregisterProfile(CLSID_DhammaKey, DK_LANGID, GUID_DhammaKeyProfile, 0);
        mgr->Release();
    }
    ITfInputProcessorProfiles* p = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_ITfInputProcessorProfiles, (void**)&p))) {
        p->Unregister(CLSID_DhammaKey);
        p->Release();
    }
}

const GUID* const kCategories[] = {
    &GUID_TFCAT_TIP_KEYBOARD,
    &GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT,  // ứng dụng Store / Windows 8+
    &GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT,
    &GUID_TFCAT_TIPCAP_UIELEMENTENABLED,
    &GUID_TFCAT_TIPCAP_SECUREMODE,
};

bool RegisterCategories(bool add) {
    ITfCategoryMgr* cm = nullptr;
    if (FAILED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr, (void**)&cm)))
        return false;
    for (const GUID* g : kCategories) {
        if (add) cm->RegisterCategory(CLSID_DhammaKey, *g, CLSID_DhammaKey);
        else cm->UnregisterCategory(CLSID_DhammaKey, *g, CLSID_DhammaKey);
    }
    cm->Release();
    return true;
}

// "042A:{CLSID}{Profile}" — cách Windows gọi tên một bộ gõ
std::wstring ProfileString() {
    wchar_t prof[64];
    StringFromGUID2(GUID_DhammaKeyProfile, prof, 64);
    return L"042A:" + ClsidString() + prof;
}

const wchar_t* kUserProfileKey = L"Control Panel\\International\\User Profile";

std::wstring ReadString(HKEY root, const wchar_t* key, const wchar_t* name) {
    wchar_t buf[512];
    DWORD sz = sizeof(buf);
    if (RegGetValueW(root, key, name, RRF_RT_REG_SZ, nullptr, buf, &sz) != ERROR_SUCCESS) return L"";
    return buf;
}
void WriteString(HKEY root, const wchar_t* key, const wchar_t* name, const std::wstring& v) {
    HKEY k;
    if (RegCreateKeyExW(root, key, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &k, nullptr) != ERROR_SUCCESS) return;
    RegSetValueExW(k, name, 0, REG_SZ, (const BYTE*)v.c_str(), (DWORD)((v.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(k);
}
std::wstring ReadOverride() { return ReadString(HKEY_CURRENT_USER, kUserProfileKey, L"InputMethodOverride"); }
void WriteOverride(const std::wstring& v) { WriteString(HKEY_CURRENT_USER, kUserProfileKey, L"InputMethodOverride", v); }
std::wstring ReadDkString(const wchar_t* name) { return ReadString(HKEY_CURRENT_USER, DK_REG_KEY, name); }
void WriteDkString(const wchar_t* name, const std::wstring& v) { WriteString(HKEY_CURRENT_USER, DK_REG_KEY, name, v); }

// Thêm DhammaKey vào danh sách bàn phím của người dùng đang đăng nhập (Windows 7+);
// makeDefault: đặt làm bàn phím mặc định khi mở máy.
void SetupForUser(bool makeDefault) {
    typedef BOOL(WINAPI * LayoutOrTipFn)(LPCWSTR, DWORD);
    std::wstring s = ProfileString();
    HMODULE h = LoadLibraryW(L"input.dll");
    if (h) {
        auto install = (LayoutOrTipFn)GetProcAddress(h, "InstallLayoutOrTip");
        if (install) install(s.c_str(), 0);
        if (makeDefault) {
            auto setDefault = (LayoutOrTipFn)GetProcAddress(h, "SetDefaultLayoutOrTip");
            if (setDefault) setDefault(s.c_str(), 0);
        }
        FreeLibrary(h);
    }
    if (makeDefault) {
        // Windows 8+: mục "Override for default input method" trong Settings.
        // Nhớ bộ gõ mặc định cũ (nếu không phải DhammaKey) để khi tắt công tắc thì trả lại.
        std::wstring old = ReadOverride();
        if (!old.empty() && _wcsicmp(old.c_str(), s.c_str()) != 0) WriteDkString(L"PrevInputMethodOverride", old);
        WriteOverride(s);
    }
}

// ---- Bật DhammaKey khi mở máy (công tắc "DefaultIme" trong Bảng điều khiển, bản 0.3.6) ----
// Windows đôi khi không giữ bộ gõ mặc định sau khi khởi động lại (cập nhật Windows, đổi ngôn ngữ, cài bản mới…),
// nên mỗi lần đăng nhập chạy lại: rundll32 "...\DhammaKey.dll",DkLogon  (khoá Run của người dùng).
const wchar_t* kRunKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

bool DefaultImeOn() {
    DWORD v = 1, sz = sizeof(v);  // chưa đặt = bật
    RegGetValueW(HKEY_CURRENT_USER, DK_REG_KEY, L"DefaultIme", RRF_RT_REG_DWORD, nullptr, &v, &sz);
    return v != 0;
}

// Chỉ ghi khoá Run từ DLL cùng bitness với Windows (x64 trên Windows 64-bit), vì explorer chạy
// rundll32 trong System32; DLL x86 trên Windows 64-bit thì để bản x64 ghi.
bool NativeBitness() {
#if defined(_WIN64)
    return true;
#else
    BOOL wow = FALSE;
    IsWow64Process(GetCurrentProcess(), &wow);
    return !wow;
#endif
}

void SetRunEntry(bool on) {
    if (!NativeBitness()) return;
    HKEY k;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRunKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &k, nullptr) != ERROR_SUCCESS)
        return;
    if (on) {
        wchar_t dll[MAX_PATH], sys[MAX_PATH];
        GetModuleFileNameW(g_hInst, dll, MAX_PATH);
        GetSystemDirectoryW(sys, MAX_PATH);
        std::wstring cmd = L"\"" + std::wstring(sys) + L"\\rundll32.exe\" \"" + dll + L"\",DkLogon";
        RegSetValueExW(k, L"DhammaKey", 0, REG_SZ, (const BYTE*)cmd.c_str(), (DWORD)((cmd.size() + 1) * sizeof(wchar_t)));
    } else {
        RegDeleteValueW(k, L"DhammaKey");
    }
    RegCloseKey(k);
}

// Tắt: bỏ DhammaKey khỏi vị trí mặc định, trả lại bộ gõ mặc định cũ (DhammaKey vẫn còn trong danh sách bàn phím).
void UnsetDefault() {
    std::wstring s = ProfileString();
    std::wstring cur = ReadOverride();
    if (!cur.empty() && _wcsicmp(cur.c_str(), s.c_str()) != 0) return;  // người dùng đã tự chọn bộ gõ khác
    std::wstring prev = ReadDkString(L"PrevInputMethodOverride");
    if (!prev.empty()) {
        WriteOverride(prev);
        typedef BOOL(WINAPI * LayoutOrTipFn)(LPCWSTR, DWORD);
        if (HMODULE h = LoadLibraryW(L"input.dll")) {
            if (auto setDefault = (LayoutOrTipFn)GetProcAddress(h, "SetDefaultLayoutOrTip")) setDefault(prev.c_str(), 0);
            FreeLibrary(h);
        }
    } else {
        HKEY k;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, kUserProfileKey, 0, KEY_SET_VALUE, &k) == ERROR_SUCCESS) {
            RegDeleteValueW(k, L"InputMethodOverride");  // Windows dùng ngôn ngữ đầu danh sách
            RegCloseKey(k);
        }
    }
}

} // namespace

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) { g_hInst = h; DisableThreadLibraryCalls(h); }
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void** ppv) {
    if (!IsEqualCLSID(clsid, CLSID_DhammaKey)) { *ppv = nullptr; return CLASS_E_CLASSNOTAVAILABLE; }
    return g_factory.QueryInterface(riid, ppv);
}

STDAPI DllCanUnloadNow() { return g_dllRefs <= 0 ? S_OK : S_FALSE; }

STDAPI DllRegisterServer() {
    HRESULT co = CoInitialize(nullptr);
    bool ok = RegisterComServer() && RegisterProfile() && RegisterCategories(true);
    if (ok) SetupForUser(false);
    if (SUCCEEDED(co)) CoUninitialize();
    if (!ok) { DllUnregisterServer(); return SELFREG_E_CLASS; }
    return S_OK;
}

STDAPI DllUnregisterServer() {
    HRESULT co = CoInitialize(nullptr);
    RegisterCategories(false);
    UnregisterProfile();
    UnregisterComServer();
    if (SUCCEEDED(co)) CoUninitialize();
    return S_OK;
}

// Bộ cài gọi sau khi cài xong, bằng tài khoản của người dùng (không phải SYSTEM):
//   rundll32.exe "C:\Program Files\DhammaKey\DhammaKey.dll",DkSetupUser
// → thêm DhammaKey vào danh sách bàn phím và đặt làm mặc định.
// Nếu máy có bộ gõ tiếng Việt khác (UniKey, EVKey, GoTiengViet…), hỏi người dùng có tắt không.
extern "C" void CALLBACK DkSetupUser(HWND, HINSTANCE, LPSTR, int) {
    HRESULT co = CoInitialize(nullptr);
    bool on = DefaultImeOn();  // cài bản mới không bật lại nếu người dùng đã tắt công tắc
    SetupForUser(on);
    SetRunEntry(on);
    if (SUCCEEDED(co)) CoUninitialize();

    std::wstring name = conflict::FindRunning();
    if (name.empty()) name = conflict::FindAutostart();
    if (name.empty()) return;

    std::wstring ask =
        L"Máy đang có bộ gõ " + name + L".\n"
        L"Dùng cùng lúc với DhammaKey sẽ làm chữ bị biến hai lần (gõ sai dấu).\n\n"
        L"Tắt " + name + L" và không cho nó tự chạy khi mở máy?\n\n"
        L"(" + name + L" không bị xóa. Muốn dùng lại: mở Task Manager → Startup → chọn " + name + L" → Enable.)";
    if (MessageBoxW(nullptr, ask.c_str(), L"DhammaKey", MB_YESNO | MB_ICONQUESTION | MB_TOPMOST | MB_SETFOREGROUND) != IDYES) {
        MessageBoxW(nullptr,
                    (L"Đã giữ nguyên " + name + L".\nKhi dùng DhammaKey, hãy thoát " + name +
                     L" hoặc chuyển nó sang chế độ E. DhammaKey sẽ nhắc nếu thấy " + name + L" đang chạy.").c_str(),
                    L"DhammaKey", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
        return;
    }
    conflict::Result r = conflict::DisableAndStop();
    if (r.failed || r.stopFailed) {
        MessageBoxW(nullptr,
                    (L"Chưa tắt được hết " + name + L" (cần quyền quản trị).\n\n"
                     L"Hãy tự làm: mở Task Manager → Startup → chọn " + name + L" → Disable, "
                     L"rồi thoát " + name + L" ở khay hệ thống.").c_str(),
                    L"DhammaKey", MB_OK | MB_ICONWARNING | MB_TOPMOST);
    } else {
        MessageBoxW(nullptr, (L"Đã tắt " + name + L". Bây giờ có thể gõ bằng DhammaKey.").c_str(),
                    L"DhammaKey", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
    }
}
#if defined(_M_IX86)
#pragma comment(linker, "/EXPORT:DkSetupUser=_DkSetupUser@16")
#else
#pragma comment(linker, "/EXPORT:DkSetupUser")
#endif

// Mỗi lần đăng nhập (khoá Run): đặt lại DhammaKey làm bộ gõ mặc định nếu công tắc đang bật. Không hiện hộp thoại.
extern "C" void CALLBACK DkLogon(HWND, HINSTANCE, LPSTR, int) {
    if (!DefaultImeOn()) { SetRunEntry(false); return; }
    HRESULT co = CoInitialize(nullptr);
    SetupForUser(true);
    if (SUCCEEDED(co)) CoUninitialize();
}

// Bảng điều khiển gọi khi bật/tắt công tắc "Bật DhammaKey khi mở máy":
//   rundll32 "...\DhammaKey.dll",DkSetDefault 1   (hoặc 0)
extern "C" void CALLBACK DkSetDefault(HWND, HINSTANCE, LPSTR arg, int) {
    bool on = !(arg && arg[0] == '0');
    HKEY k;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, DK_REG_KEY, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &k, nullptr) == ERROR_SUCCESS) {
        DWORD v = on ? 1 : 0;
        RegSetValueExW(k, L"DefaultIme", 0, REG_DWORD, (const BYTE*)&v, sizeof(v));
        RegCloseKey(k);
    }
    HRESULT co = CoInitialize(nullptr);
    if (on) SetupForUser(true); else UnsetDefault();
    SetRunEntry(on);
    if (SUCCEEDED(co)) CoUninitialize();
}
// Bộ cài gọi khi gỡ hẳn DhammaKey (không phải khi nâng cấp): trả lại bộ gõ mặc định cũ, xoá dòng tự chạy.
// Không ghi "DefaultIme", để lần cài lại sau vẫn theo lựa chọn cũ của người dùng.
extern "C" void CALLBACK DkUninstallUser(HWND, HINSTANCE, LPSTR, int) {
    HRESULT co = CoInitialize(nullptr);
    UnsetDefault();
    SetRunEntry(false);
    if (SUCCEEDED(co)) CoUninitialize();
}
#if defined(_M_IX86)
#pragma comment(linker, "/EXPORT:DkUninstallUser=_DkUninstallUser@16")
#pragma comment(linker, "/EXPORT:DkLogon=_DkLogon@16")
#pragma comment(linker, "/EXPORT:DkSetDefault=_DkSetDefault@16")
#else
#pragma comment(linker, "/EXPORT:DkUninstallUser")
#pragma comment(linker, "/EXPORT:DkLogon")
#pragma comment(linker, "/EXPORT:DkSetDefault")
#endif
