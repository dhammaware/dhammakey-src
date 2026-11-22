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

#include "TextService.h"
#include "LangBar.h"
#include "Conflict.h"
#include "Guard.h"
#include <dhammakey_pali_keys.h>
#include <inputscope.h>
#include <memory>
#include <functional>

namespace {

// Edit session dùng lambda
class EditSession : public ITfEditSession {
public:
    explicit EditSession(std::function<HRESULT(TfEditCookie)> fn) : fn_(std::move(fn)) {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (riid == IID_IUnknown || riid == IID_ITfEditSession) { *ppv = this; AddRef(); return S_OK; }
        *ppv = nullptr; return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&refs_); }
    STDMETHODIMP_(ULONG) Release() override {
        LONG r = InterlockedDecrement(&refs_);
        if (!r) delete this;
        return r;
    }
    STDMETHODIMP DoEditSession(TfEditCookie ec) override { return fn_(ec); }
private:
    LONG refs_ = 1;
    std::function<HRESULT(TfEditCookie)> fn_;
};

std::wstring Widen(const char* s) {
    if (!s || !*s) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s, -1, &w[0], n);
    w.resize(n - 1);
    return w;
}

std::string Narrow(const std::wstring& w) {
    if (w.empty()) return std::string();
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, nullptr, nullptr);
    return s;
}

// Gửi lại một phím (Enter, Tab, mũi tên, Ctrl+…) cho ứng dụng sau khi đã chốt từ.
// Lúc gửi lại thì không còn đang soạn, nên OnTestKeyDown để phím đi thẳng vào ứng dụng.
// Alt phải + chữ → chữ Pāli/Sanskrit: đăng ký thêm làm "phím giữ trước" (preserved key) của TSF (0.3.30).
// Lý do: đã thử trên máy thật 0.3.29, Alt phải + chữ không ra chữ nào, máy kêu "boong" — tức phím Alt+chữ lọt vào
// ứng dụng như phím tắt menu, không qua OnTestKeyDown. Phím giữ trước được TSF xét TRƯỚC, kể cả ở
// ứng dụng cũ chạy qua lớp IMM (ở đó phím có Alt không được đưa cho bộ gõ).
// Chữ cái in hoa của các phím trong bảng core/include/dhammakey_pali_keys.h (nơi duy nhất định nghĩa)
static char PaliAltKey(int i) { return i < DK_PALI_ALT_COUNT ? (char)(kDkPaliAltKeys[i].key - 32) : 0; }
static GUID PaliAltGuid(int index, bool shift) {
    GUID g = {0x6D1F3A52, 0x8C41, 0x4E0B, {0x9A, 0x27, 0x31, 0x5C, 0x44, 0xD0, 0x10, 0x00}};
    g.Data4[7] = (unsigned char)(index * 2 + (shift ? 1 : 0));
    return g;
}
static const wchar_t kPaliAltDesc[] = L"DhammaKey: Alt phải + chữ (Pāli)";

static bool IsModifierVk(WPARAM wp) {
    switch (wp) {
        case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
        case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
        case VK_MENU: case VK_LMENU: case VK_RMENU:
        case VK_CAPITAL: case VK_LWIN: case VK_RWIN:
            return true;
    }
    return false;
}

// Dò lỗi phím (GuardDebug = 1): ghi chuỗi ngắn vào HKCU\Software\DhammaKey\KeyLast
static void KeyDebug(const std::wstring& line) {
    HKEY k;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, DK_REG_KEY, 0, KEY_READ | KEY_WRITE, &k) != ERROR_SUCCESS) return;
    DWORD on = 0, sz = sizeof(on), type = 0;
    if (RegQueryValueExW(k, L"GuardDebug", nullptr, &type, (BYTE*)&on, &sz) == ERROR_SUCCESS && type == REG_DWORD && on)
        RegSetValueExW(k, L"KeyLast", 0, REG_SZ, (const BYTE*)line.c_str(), (DWORD)((line.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(k);
}

void ReplayKey(WPARAM wp, LPARAM lp) {
    INPUT in = {};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = (WORD)wp;
    in.ki.wScan = (WORD)((lp >> 16) & 0xFF);
    if (lp & (1 << 24)) in.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    SendInput(1, &in, sizeof(INPUT));
}

DWORD RegGet(HKEY k, const wchar_t* name, DWORD def) {
    DWORD v = def, sz = sizeof(v), type = 0;
    if (RegQueryValueExW(k, name, nullptr, &type, (BYTE*)&v, &sz) != ERROR_SUCCESS || type != REG_DWORD) return def;
    return v;
}

// Lời nhắc khi thấy bộ gõ khác đang chạy. Hiện trên luồng riêng để không làm treo ứng dụng.
DWORD WINAPI ConflictNoticeThread(LPVOID p) {
    std::unique_ptr<std::wstring> name((std::wstring*)p);
    std::wstring text =
        *name + L" đang chạy cùng lúc với DhammaKey.\n"
        L"Hai bộ gõ cùng bật sẽ làm chữ bị biến hai lần (gõ sai dấu).\n\n"
        L"Cách sửa: thoát " + *name + L" (bấm phải vào biểu tượng của nó ở khay hệ thống → Thoát/Kết thúc), "
        L"hoặc chuyển " + *name + L" sang chế độ gõ tiếng Anh (E).\n\n"
        L"Có nhắc lại vào những ngày sau không?";
    int r = MessageBoxW(nullptr, text.c_str(), L"DhammaKey",
                        MB_YESNO | MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND);
    if (r == IDNO) {
        HKEY k;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, DK_REG_KEY, 0, nullptr, 0, KEY_WRITE, nullptr, &k, nullptr) == ERROR_SUCCESS) {
            DWORD one = 1;
            RegSetValueExW(k, L"ConflictNoticeOff", 0, REG_DWORD, (const BYTE*)&one, sizeof(one));
            RegCloseKey(k);
        }
    }
    // Giữ DLL trong bộ nhớ cho tới khi luồng này kết thúc hẳn
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCWSTR)&ConflictNoticeThread, &self);
    FreeLibraryAndExitThread(self, 0);
    return 0;
}

// Nhắc tối đa một lần mỗi ngày (dù có nhiều ứng dụng), trừ khi người dùng đã chọn không nhắc nữa
void MaybeShowConflictNotice(const std::wstring& name) {
    HKEY k;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, DK_REG_KEY, 0, nullptr, 0, KEY_READ | KEY_WRITE, nullptr, &k, nullptr) != ERROR_SUCCESS)
        return;
    SYSTEMTIME st;
    GetLocalTime(&st);
    DWORD today = st.wYear * 10000 + st.wMonth * 100 + st.wDay;
    bool show = RegGet(k, L"ConflictNoticeOff", 0) == 0 && RegGet(k, L"ConflictNoticeDay", 0) != today;
    if (show) RegSetValueExW(k, L"ConflictNoticeDay", 0, REG_DWORD, (const BYTE*)&today, sizeof(today));
    RegCloseKey(k);
    if (!show) return;

    HMODULE self = nullptr;  // tăng số tham chiếu DLL; luồng sẽ trả lại khi xong
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)&ConflictNoticeThread, &self)) return;
    std::wstring* arg = new std::wstring(name);
    HANDLE t = CreateThread(nullptr, 0, ConflictNoticeThread, arg, 0, nullptr);
    if (t) CloseHandle(t);
    else { delete arg; FreeLibrary(self); }
}

} // namespace

TextService::TextService() {
    DllAddRef();
    eng_ = dk_engine_new();
}

TextService::~TextService() {
    dk_engine_free(eng_);
    DllRelease();
}

STDMETHODIMP TextService::QueryInterface(REFIID riid, void** ppv) {
    if (!ppv) return E_INVALIDARG;
    *ppv = nullptr;
    if (riid == IID_IUnknown || riid == IID_ITfTextInputProcessor || riid == IID_ITfTextInputProcessorEx)
        *ppv = static_cast<ITfTextInputProcessorEx*>(this);
    else if (riid == IID_ITfKeyEventSink) *ppv = static_cast<ITfKeyEventSink*>(this);
    else if (riid == IID_ITfCompositionSink) *ppv = static_cast<ITfCompositionSink*>(this);
    else if (riid == IID_ITfThreadMgrEventSink) *ppv = static_cast<ITfThreadMgrEventSink*>(this);
    if (!*ppv) return E_NOINTERFACE;
    AddRef();
    return S_OK;
}
STDMETHODIMP_(ULONG) TextService::AddRef() { return InterlockedIncrement(&refs_); }
STDMETHODIMP_(ULONG) TextService::Release() {
    LONG r = InterlockedDecrement(&refs_);
    if (!r) delete this;
    return r;
}

// ---------------- Kích hoạt / hủy ----------------

STDMETHODIMP TextService::ActivateEx(ITfThreadMgr* tm, TfClientId cid, DWORD) {
    tm_ = tm; tm_->AddRef();
    cid_ = cid;
    LoadSettings();

    static const wchar_t kToggleDesc[] = L"DhammaKey: Việt / Tắt";
    static const wchar_t kPaliDesc[] = L"DhammaKey: Pāli";
    ITfKeystrokeMgr* km = nullptr;
    if (SUCCEEDED(tm_->QueryInterface(IID_ITfKeystrokeMgr, (void**)&km))) {
        km->AdviseKeyEventSink(cid_, static_cast<ITfKeyEventSink*>(this), TRUE);
        TF_PRESERVEDKEY toggle = {VK_OEM_3, TF_MOD_CONTROL};             // Ctrl + `
        TF_PRESERVEDKEY pali = {'P', TF_MOD_CONTROL | TF_MOD_SHIFT};     // Ctrl + Shift + P
        km->PreserveKey(cid_, GUID_PreservedKey_Toggle, &toggle, kToggleDesc, (ULONG)wcslen(kToggleDesc));
        km->PreserveKey(cid_, GUID_PreservedKey_Pali, &pali, kPaliDesc, (ULONG)wcslen(kPaliDesc));
        for (int i = 0; PaliAltKey(i); ++i)
            for (int s = 0; s < 2; ++s) {
                TF_PRESERVEDKEY pk = {(UINT)PaliAltKey(i), (UINT)(TF_MOD_RALT | (s ? TF_MOD_SHIFT : 0))};
                km->PreserveKey(cid_, PaliAltGuid(i, s != 0), &pk, kPaliAltDesc, (ULONG)wcslen(kPaliAltDesc));
            }
        km->Release();
    }
    ITfSource* src = nullptr;
    if (SUCCEEDED(tm_->QueryInterface(IID_ITfSource, (void**)&src))) {
        src->AdviseSink(IID_ITfThreadMgrEventSink, static_cast<ITfThreadMgrEventSink*>(this), &tmSinkCookie_);
        src->Release();
    }
    CheckConflict();
    // Biểu tượng ở khay hệ thống
    lb_ = new LangBarButton(this);
    ITfLangBarItemMgr* lbm = nullptr;
    if (SUCCEEDED(tm_->QueryInterface(IID_ITfLangBarItemMgr, (void**)&lbm))) {
        lbm->AddItem(lb_);
        lbm->Release();
    }
    return S_OK;
}

STDMETHODIMP TextService::Deactivate() {
    if (lb_) {
        ITfLangBarItemMgr* lbm = nullptr;
        if (tm_ && SUCCEEDED(tm_->QueryInterface(IID_ITfLangBarItemMgr, (void**)&lbm))) {
            lbm->RemoveItem(lb_);
            lbm->Release();
        }
        lb_->Detach();
        lb_->Release();
        lb_ = nullptr;
    }
    if (tm_) {
        ITfKeystrokeMgr* km = nullptr;
        if (SUCCEEDED(tm_->QueryInterface(IID_ITfKeystrokeMgr, (void**)&km))) {
            TF_PRESERVEDKEY toggle = {VK_OEM_3, TF_MOD_CONTROL};
            TF_PRESERVEDKEY pali = {'P', TF_MOD_CONTROL | TF_MOD_SHIFT};
            km->UnpreserveKey(GUID_PreservedKey_Toggle, &toggle);
            km->UnpreserveKey(GUID_PreservedKey_Pali, &pali);
            for (int i = 0; PaliAltKey(i); ++i)
                for (int s = 0; s < 2; ++s) {
                    TF_PRESERVEDKEY pk = {(UINT)PaliAltKey(i), (UINT)(TF_MOD_RALT | (s ? TF_MOD_SHIFT : 0))};
                    km->UnpreserveKey(PaliAltGuid(i, s != 0), &pk);
                }
            km->UnadviseKeyEventSink(cid_);
            km->Release();
        }
        ITfSource* src = nullptr;
        if (tmSinkCookie_ != TF_INVALID_COOKIE && SUCCEEDED(tm_->QueryInterface(IID_ITfSource, (void**)&src))) {
            src->UnadviseSink(tmSinkCookie_);
            src->Release();
            tmSinkCookie_ = TF_INVALID_COOKIE;
        }
        tm_->Release();
        tm_ = nullptr;
    }
    if (comp_) { comp_->Release(); comp_ = nullptr; }
    dk_reset(eng_);
    cid_ = TF_CLIENTID_NULL;
    return S_OK;
}

// ---------------- Cài đặt (HKCU\Software\DhammaKey) ----------------

void TextService::LoadSettings() {
    HKEY k;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, DK_REG_KEY, 0, KEY_READ, &k) != ERROR_SUCCESS) return;
    dk_set_option(eng_, DK_OPT_METHOD, (int)RegGet(k, L"Method", DK_METHOD_TELEX));
    dk_set_option(eng_, DK_OPT_TONE_NEW_STYLE, (int)RegGet(k, L"ToneNewStyle", 0));
    dk_set_option(eng_, DK_OPT_SPELL_CHECK, (int)RegGet(k, L"SpellCheck", 1));
    dk_set_option(eng_, DK_OPT_W_TO_U, (int)RegGet(k, L"WToU", 1));
    // Bảng mã xuất ra (0.3.20). Chưa có giá trị Charset thì theo công tắc "Unicode tổ hợp" cũ.
    dk_set_option(eng_, DK_OPT_CHARSET,
                  (int)RegGet(k, L"Charset", RegGet(k, L"OutputCombining", 0) ? DK_CHARSET_UNICODE_COMBINING
                                                                             : DK_CHARSET_UNICODE));
    dk_set_option(eng_, DK_OPT_PALI_M_DOT_ABOVE, (int)RegGet(k, L"PaliMDotAbove", 0));
    lastMethod_ = (int)RegGet(k, L"LastVietMethod", DK_METHOD_TELEX);
    // Tự tắt gõ dấu (0.3.23), mặc định bật cả bốn
    guardPassword_ = RegGet(k, L"PasswordGuard", 1) != 0;
    guardUrl_ = RegGet(k, L"UrlGuard", 1) != 0;
    guardConsole_ = RegGet(k, L"ConsoleGuard", 1) != 0;
    guardAscii_ = RegGet(k, L"AsciiFieldGuard", 1) != 0;
    RegCloseKey(k);
}

void TextService::SaveMethod() {
    HKEY k;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, DK_REG_KEY, 0, nullptr, 0, KEY_WRITE, nullptr, &k, nullptr) != ERROR_SUCCESS) return;
    DWORD m = (DWORD)dk_get_option(eng_, DK_OPT_METHOD), last = (DWORD)lastMethod_;
    RegSetValueExW(k, L"Method", 0, REG_DWORD, (const BYTE*)&m, sizeof(m));
    RegSetValueExW(k, L"LastVietMethod", 0, REG_DWORD, (const BYTE*)&last, sizeof(last));
    RegCloseKey(k);
}

// ---------------- Phím ----------------

bool TextService::TranslateKey(WPARAM wp, LPARAM lp, uint32_t& ch, uint32_t& flags) {
    flags = 0;
    if (wp == VK_BACK) { ch = DK_KEY_BACKSPACE; return true; }
    BYTE ks[256];
    if (!GetKeyboardState(ks)) return false;
    bool altGr = (GetKeyState(VK_RMENU) & 0x8000) != 0;
    bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    if (altGr) {
        flags = DK_FLAG_ALTGR;
        // Lấy chữ gốc của phím (không tính Ctrl/Alt) để tra bảng Pali
        ks[VK_CONTROL] = ks[VK_LCONTROL] = ks[VK_RCONTROL] = 0;
        ks[VK_MENU] = ks[VK_LMENU] = ks[VK_RMENU] = 0;
    } else if (ctrl || alt) {
        return false; // phím tắt của ứng dụng (Ctrl+C …)
    }
    WCHAR buf[4] = {};
    // 0x4: không làm thay đổi trạng thái dead key (Windows 10 1607+; Windows 7 bỏ qua cờ này)
    int n = ToUnicodeEx((UINT)wp, (lp >> 16) & 0xFF, ks, buf, 4, 0x4, GetKeyboardLayout(0));
    if (n != 1) return false;
    ch = buf[0];
    return true;
}

STDMETHODIMP TextService::OnTestKeyDown(ITfContext* ctx, WPARAM wp, LPARAM lp, BOOL* eaten) {
    *eaten = FALSE;
    // Phím bổ trợ đứng riêng (Shift, Ctrl, Alt, Caps, Win) không bao giờ thuộc bộ gõ. Trước 0.3.30, đang soạn
    // dở mà nhấn Alt phải thì từ bị chốt và phím Alt bị gửi lại lần nữa — hỏng việc gõ ā giữa từ ("nibb" + Alt phải).
    if (IsModifierVk(wp)) return S_OK;
    if (comp_) { *eaten = TRUE; return S_OK; } // đang soạn: mọi phím đều qua OnKeyDown để chốt đúng lúc
    if (dk_get_option(eng_, DK_OPT_METHOD) == DK_METHOD_OFF) return S_OK;
    uint32_t ch, fl;
    bool ok = TranslateKey(wp, lp, ch, fl);
    if (GetKeyState(VK_RMENU) & 0x8000)
        KeyDebug(L"test wp=" + std::to_wstring((int)wp) + L" ok=" + std::to_wstring((int)ok) +
                 L" ch=" + std::to_wstring(ok ? (int)ch : -1) + L" fl=" + std::to_wstring(ok ? (int)fl : -1));
    if (!ok || ch == DK_KEY_BACKSPACE) return S_OK;
    bool letter = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
    // Velthuis: dấu " ~ và dấu chấm mở đầu chữ (.t → ṭ ngay đầu từ, 0.3.30)
    bool paliMark = dk_get_option(eng_, DK_OPT_METHOD) == DK_METHOD_PALI_VELTHUIS && (ch == '"' || ch == '~' || ch == '.');
    *eaten = letter || paliMark || (fl & DK_FLAG_ALTGR);
    // Ô mật khẩu, thanh địa chỉ, dòng lệnh, ô email/số: phím nào ra chữ nấy (0.3.23).
    // Chỉ hỏi loại ô khi bộ gõ định nhận phím — không đụng tới Ctrl+A/C/V, phím mũi tên… (0.3.24)
    if (*eaten && UpdateGuard(ctx, false) != guard::NONE) *eaten = FALSE;
    return S_OK;
}

// Lưu ý: TSF chỉ gọi OnKeyDown khi OnTestKeyDown đã nhận phím (eaten = TRUE).
// Khi đó phím gốc đã bị giữ lại, trả eaten = FALSE ở đây cũng KHÔNG đưa phím
// vào ứng dụng được — đó là lỗi "phải nhấn Space hai lần" của bản 0.1.1.
// Vì vậy ở đây phím nào cũng phải được xử lý trọn:
//   - phím bộ gõ dùng            → cập nhật chữ đang soạn
//   - ký tự in được (cách, dấu…) → chốt từ + chèn luôn ký tự đó
//   - phím điều khiển (Enter, mũi tên, Ctrl+…) → chốt từ rồi gửi lại phím
STDMETHODIMP TextService::OnKeyDown(ITfContext* ctx, WPARAM wp, LPARAM lp, BOOL* eaten) {
    if (IsModifierVk(wp)) { *eaten = FALSE; return S_OK; }
    *eaten = TRUE;
    uint32_t ch, fl;
    // Một số ứng dụng (cửa sổ dòng lệnh) gọi thẳng OnKeyDown, bỏ qua OnTestKeyDown:
    // kiểm tra loại ô cả ở đây khi chưa soạn dở (0.3.25)
    if (!comp_ && TranslateKey(wp, lp, ch, fl) && ch != DK_KEY_BACKSPACE &&
        UpdateGuard(ctx, false) != guard::NONE) {
        *eaten = FALSE;
        return S_OK;
    }
    if (!TranslateKey(wp, lp, ch, fl)) {
        bool composing = comp_ != nullptr;
        CommitIfComposing(ctx);
        if (composing) ReplayKey(wp, lp);
        else *eaten = FALSE;
        return S_OK;
    }
    dk_result r;
    dk_process_key(eng_, ch, fl, &r);
    if (r.consumed) {
        bool empty = !r.preedit || !*r.preedit;
        Update(ctx, r.preedit, empty);
        return S_OK;
    }

    // Phím không thuộc bộ gõ: lấy từ đang soạn để chốt
    std::wstring word;
    if (r.commit) {
        word = Widen(r.preedit);
    } else if (comp_) {
        dk_result c;
        dk_commit(eng_, &c);
        word = Widen(c.preedit);
    }

    bool printable = ch != DK_KEY_BACKSPACE && ch >= 0x20 && ch != 0x7F;
    if (printable) {
        word.push_back((wchar_t)ch);           // chốt từ + dấu cách/dấu câu trong một lần
        Update(ctx, Narrow(word).c_str(), true);
    } else {
        bool composing = comp_ != nullptr;
        if (composing) Update(ctx, Narrow(word).c_str(), true);
        dk_reset(eng_);
        if (composing) ReplayKey(wp, lp);      // Enter, Tab, Esc, Backspace…
        else *eaten = FALSE;
    }
    return S_OK;
}

STDMETHODIMP TextService::OnPreservedKey(ITfContext* ctx, REFGUID guid, BOOL* eaten) {
    *eaten = FALSE;
    int m = Method();
    // Alt phải + chữ (0.3.30)
    for (int i = 0; PaliAltKey(i); ++i)
        for (int s = 0; s < 2; ++s) {
            if (!IsEqualGUID(guid, PaliAltGuid(i, s != 0))) continue;
            if (m == DK_METHOD_OFF || !ctx) return S_OK;
            if (UpdateGuard(ctx, false) != guard::NONE) return S_OK;   // ô mật khẩu…: để phím đi thẳng
            bool upper = (s != 0) != ((GetKeyState(VK_CAPITAL) & 1) != 0);
            uint32_t ch = upper ? (uint32_t)PaliAltKey(i) : (uint32_t)(PaliAltKey(i) + 32);
            dk_result r;
            dk_process_key(eng_, ch, DK_FLAG_ALTGR, &r);
            KeyDebug(L"preserved ch=" + std::to_wstring((int)ch) + L" consumed=" + std::to_wstring((int)r.consumed));
            if (r.consumed) {
                bool empty = !r.preedit || !*r.preedit;
                Update(ctx, r.preedit, empty);
                *eaten = TRUE;
            }
            return S_OK;
        }
    int next;
    if (IsEqualGUID(guid, GUID_PreservedKey_Toggle)) {
        next = (m == DK_METHOD_OFF) ? lastMethod_ : DK_METHOD_OFF;
    } else if (IsEqualGUID(guid, GUID_PreservedKey_Pali)) {
        next = (m == DK_METHOD_PALI_VELTHUIS) ? lastMethod_ : DK_METHOD_PALI_VELTHUIS;
    } else {
        return S_OK;
    }
    CommitIfComposing(ctx);
    ApplyMethod(next);
    MessageBeep(next == DK_METHOD_OFF ? MB_ICONASTERISK : MB_OK);
    *eaten = TRUE;
    return S_OK;
}

// ---------------- Kiểu gõ (phím tắt, biểu tượng khay hệ thống) ----------------

int TextService::Method() { return dk_get_option(eng_, DK_OPT_METHOD); }

static bool IsVietMethod(int m) { return m != DK_METHOD_OFF && m != DK_METHOD_PALI_VELTHUIS; }

void TextService::ApplyMethod(int next) {
    int m = Method();
    if (IsVietMethod(m)) lastMethod_ = m;       // nhớ kiểu gõ Việt cuối cùng để bật lại
    if (IsVietMethod(next)) lastMethod_ = next;
    dk_set_option(eng_, DK_OPT_METHOD, next);
    SaveMethod();
    if (lb_) lb_->Refresh();
}

void TextService::ChooseMethod(int next) {
    ITfContext* ctx = FocusContext();
    if (ctx) { CommitFromUI(ctx); ctx->Release(); }
    else dk_reset(eng_);
    ApplyMethod(next);
}

int TextService::Charset() { return dk_get_option(eng_, DK_OPT_CHARSET); }

void TextService::ChooseCharset(int cs) {
    if (cs < 0 || cs >= DK_CHARSET_COUNT) return;
    ITfContext* ctx = FocusContext();
    if (ctx) { CommitFromUI(ctx); ctx->Release(); }   // chốt chữ đang gõ dở theo bảng mã cũ
    else dk_reset(eng_);
    dk_set_option(eng_, DK_OPT_CHARSET, cs);
    HKEY k;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, DK_REG_KEY, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &k, nullptr) == ERROR_SUCCESS) {
        DWORD v = (DWORD)cs, comb = cs == DK_CHARSET_UNICODE_COMBINING ? 1 : 0;
        RegSetValueExW(k, L"Charset", 0, REG_DWORD, (const BYTE*)&v, sizeof(v));
        RegSetValueExW(k, L"OutputCombining", 0, REG_DWORD, (const BYTE*)&comb, sizeof(comb));
        RegCloseKey(k);
    }
}

void TextService::ToggleOnOff() {
    ChooseMethod(Method() == DK_METHOD_OFF ? lastMethod_ : DK_METHOD_OFF);
}

ITfContext* TextService::FocusContext() {
    ITfDocumentMgr* dm = nullptr;
    ITfContext* ctx = nullptr;
    if (tm_ && SUCCEEDED(tm_->GetFocus(&dm)) && dm) {
        dm->GetTop(&ctx);
        dm->Release();
    }
    return ctx;
}

// Mỗi ứng dụng có một bản DhammaKey riêng: khi đổi cửa sổ thì đọc lại kiểu gõ đã lưu
// (đọc cả các tùy chọn khác, để thay đổi trong cửa sổ cài đặt có hiệu lực ngay khi quay lại gõ)
void TextService::SyncMethodFromRegistry() {
    LoadSettings();
    if (lb_) lb_->Refresh();
}

// ---------------- Composition ----------------

void TextService::CommitIfComposing(ITfContext* ctx) {
    if (!comp_) { dk_reset(eng_); return; }
    dk_result r;
    dk_commit(eng_, &r);
    Update(ctx, r.preedit, true);
}

// Chốt từ khi bấm biểu tượng: ngoài lúc xử lý phím, Windows có thể chưa cho sửa
// văn bản ngay, nên xin phiên sửa "không đồng bộ" (chạy ngay nếu được, nếu không thì chạy sau).
void TextService::CommitFromUI(ITfContext* ctx) {
    if (!comp_) { dk_reset(eng_); return; }
    dk_result r;
    dk_commit(eng_, &r);
    std::wstring text = Widen(r.preedit);
    ctx->AddRef();
    EditSession* es = new EditSession([this, ctx, text](TfEditCookie ec) {
        HRESULT hr = comp_ ? DoUpdate(ec, ctx, text, true) : S_OK;
        ctx->Release();
        return hr;
    });
    HRESULT hr;
    if (FAILED(ctx->RequestEditSession(cid_, es, TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, &hr)))
        ctx->Release();
    es->Release();
}

void TextService::Update(ITfContext* ctx, const char* utf8, bool finish) {
    if (!ctx) return;
    std::wstring text = Widen(utf8);
    EditSession* es = new EditSession([this, ctx, text, finish](TfEditCookie ec) {
        return DoUpdate(ec, ctx, text, finish);
    });
    HRESULT hr;
    ctx->RequestEditSession(cid_, es, TF_ES_SYNC | TF_ES_READWRITE, &hr);
    es->Release();
}

// GUID_PROP_LANGID (msctf.h) — ghi thẳng giá trị để không phụ thuộc thư viện liên kết
static const GUID kPropLangId = {0x3280CE20, 0x8032, 0x11D2, {0xB6, 0x03, 0x00, 0x10, 0x5A, 0x27, 0x99, 0xB5}};

HRESULT TextService::DoUpdate(TfEditCookie ec, ITfContext* ctx, const std::wstring& text, bool finish) {
    if (!comp_) {
        if (text.empty()) return S_OK;
        ITfInsertAtSelection* ias = nullptr;
        ITfContextComposition* cc = nullptr;
        ITfRange* range = nullptr;
        if (SUCCEEDED(ctx->QueryInterface(IID_ITfInsertAtSelection, (void**)&ias)) &&
            SUCCEEDED(ias->InsertTextAtSelection(ec, TF_IAS_QUERYONLY, nullptr, 0, &range)) &&
            SUCCEEDED(ctx->QueryInterface(IID_ITfContextComposition, (void**)&cc))) {
            cc->StartComposition(ec, range, static_cast<ITfCompositionSink*>(this), &comp_);
        }
        if (range) range->Release();
        if (cc) cc->Release();
        if (ias) ias->Release();
        if (!comp_) return E_FAIL;
    }
    ITfRange* range = nullptr;
    if (SUCCEEDED(comp_->GetRange(&range))) {
        range->SetText(ec, 0, text.c_str(), (LONG)text.size());
        // Bảng mã cũ (0.3.20): gắn ngôn ngữ tiếng Anh cho đoạn chữ. Hồ sơ bộ gõ là tiếng Việt nên Word
        // tưởng phông .VnTime, VNI-Times (chỉ có bộ chữ phương Tây) không gõ được tiếng Việt và tự đổi
        // sang phông Unicode (Calibri) — gặp trên máy thật 15/11/2026. Chữ bảng mã cũ thực ra là ký tự phương Tây.
        if (dk_get_option(eng_, DK_OPT_CHARSET) >= DK_CHARSET_TCVN3) {
            ITfProperty* lang = nullptr;
            if (SUCCEEDED(ctx->GetProperty(kPropLangId, &lang)) && lang) {
                VARIANT v;
                VariantInit(&v);
                v.vt = VT_I4;
                v.lVal = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
                lang->SetValue(ec, range, &v);
                lang->Release();
            }
        }
        ITfRange* caret = nullptr;
        if (SUCCEEDED(range->Clone(&caret))) {
            caret->Collapse(ec, TF_ANCHOR_END);
            TF_SELECTION sel = {};
            sel.range = caret;
            sel.style.ase = TF_AE_NONE;
            sel.style.fInterimChar = FALSE;
            ctx->SetSelection(ec, 1, &sel);
            caret->Release();
        }
        range->Release();
    }
    if (finish) {
        comp_->EndComposition(ec);
        comp_->Release();
        comp_ = nullptr;
    }
    return S_OK;
}

STDMETHODIMP TextService::OnCompositionTerminated(TfEditCookie, ITfComposition*) {
    // Ứng dụng tự kết thúc (bấm chuột, đổi ô nhập…) — chữ đã nằm trong văn bản
    if (comp_) { comp_->Release(); comp_ = nullptr; }
    dk_reset(eng_);
    return S_OK;
}

STDMETHODIMP TextService::OnSetFocus(ITfDocumentMgr*, ITfDocumentMgr* prev) {
    if (comp_ && prev) {
        ITfContext* ctx = nullptr;
        if (SUCCEEDED(prev->GetTop(&ctx)) && ctx) {
            CommitIfComposing(ctx);
            ctx->Release();
        }
    }
    dk_reset(eng_);
    SyncMethodFromRegistry();
    CheckConflict();
    guardCtx_ = nullptr;   // ô mới: hỏi lại ở phím đầu tiên
    if (guard_ != guard::NONE) { guard_ = guard::NONE; if (lb_) lb_->Refresh(); }
    return S_OK;
}

// ---------------- Tự tắt gõ dấu (0.3.23) ----------------

static const GUID kPropInputScope = {0x1713DD5A, 0x68E7, 0x4A5B, {0x9A, 0xF6, 0x59, 0x2A, 0x59, 0x5C, 0x77, 0x8D}};
static const IID kIID_ITfInputScope = {0xFDE1EAEE, 0x6924, 0x4CDF, {0x91, 0xE7, 0xDA, 0x38, 0xCF, 0xF5, 0x55, 0x9D}};

// Đọc InputScope ứng dụng gắn cho ô đang gõ (Chrome/Edge/Firefox: IS_URL ở thanh địa chỉ,
// IS_PASSWORD ở ô mật khẩu; ô UWP/WinUI cũng báo như vậy). Đọc đồng bộ, chỉ đọc —
// TSF cho phép trong lúc xử lý phím.
int TextService::ReadInputScopeGuard(ITfContext* ctx) {
    if (!ctx) return guard::NONE;
    int kind = guard::NONE;
    std::wstring seen;   // các InputScope đọc được, ghi ra GuardDebug khi cần dò lỗi
    bool sawPrivate = false;   // IS_PRIVATE (61): ô mật khẩu Chrome/Edge HOẶC ô thường ở cửa sổ ẩn danh
    auto* es = new EditSession([&](TfEditCookie ec) -> HRESULT {
        ITfRange* range = nullptr;
        TF_SELECTION sel = {};
        ULONG got = 0;
        if (SUCCEEDED(ctx->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &sel, &got)) && got) range = sel.range;
        else ctx->GetStart(ec, &range);
        if (!range) return S_OK;
        auto scan = [&](const VARIANT& v, const wchar_t* src) {
            if (v.vt != VT_UNKNOWN || !v.punkVal) return;
            ITfInputScope* is = nullptr;
            if (FAILED(v.punkVal->QueryInterface(kIID_ITfInputScope, (void**)&is)) || !is) return;
            InputScope* scopes = nullptr;
            UINT count = 0;
            if (SUCCEEDED(is->GetInputScopes(&scopes, &count)) && scopes) {
                for (UINT i = 0; i < count; ++i) {
                    seen += src; seen += L":"; seen += std::to_wstring((int)scopes[i]); seen += L" ";
                    if ((int)scopes[i] == 61) sawPrivate = true;
                    if (kind == guard::NONE) kind = guard::FromInputScope((int)scopes[i]);
                }
                CoTaskMemFree(scopes);
            }
            is->Release();
        };
        // 1) Thuộc tính ứng dụng (app property): Chrome, Edge, Firefox, ô Win32 đặt bằng SetInputScope
        //    báo InputScope theo đường này (ITextStoreACP::RetrieveRequestedAttrs) — 0.3.24.
        ITfReadOnlyProperty* app = nullptr;
        if (SUCCEEDED(ctx->GetAppProperty(kPropInputScope, &app)) && app) {
            VARIANT v; VariantInit(&v);
            if (SUCCEEDED(app->GetValue(ec, range, &v))) scan(v, L"app");
            VariantClear(&v);
            app->Release();
        }
        // 2) Thuộc tính TSF thường (một số ứng dụng UWP / WinUI)
        if (kind == guard::NONE) {
            ITfProperty* prop = nullptr;
            if (SUCCEEDED(ctx->GetProperty(kPropInputScope, &prop)) && prop) {
                VARIANT v; VariantInit(&v);
                if (SUCCEEDED(prop->GetValue(ec, range, &v))) scan(v, L"tsf");
                VariantClear(&v);
                prop->Release();
            }
        }
        range->Release();
        return S_OK;
    });
    HRESULT hr = S_OK;
    HRESULT rq = ctx->RequestEditSession(cid_, es, TF_ES_READ | TF_ES_SYNC, &hr);
    es->Release();
    // Chỉ có IS_PRIVATE: hỏi thêm ứng dụng ô này có phải ô được bảo vệ (mật khẩu) không (0.3.26)
    // Chrome dựng thông tin trợ năng sau lần hỏi đầu (không đồng bộ): chưa biết thì hỏi lại ngay ở phím sau
    guardUnsure_ = false;
    if (kind == guard::NONE && sawPrivate) {
        std::wstring info;
        int p = guard::FocusIsProtected(info);
        seen += info;
        // Chưa biết chắc thì cứ tạm tắt gõ dấu: thà mấy chữ đầu ở ô ẩn danh không có dấu,
        // còn hơn làm sai mật khẩu mà người gõ không nhìn thấy (0.3.28). Phím sau hỏi lại.
        if (p == 1) kind = guard::PASSWORD;
        else if (p == 2) kind = guard::URL;      // thanh địa chỉ trong cửa sổ ẩn danh (0.3.34)
        else if (p < 0) { kind = guard::PASSWORD; guardUnsure_ = true; }
    }
    // Dò lỗi: đặt HKCU\Software\DhammaKey\GuardDebug = 1 thì ghi lại lần đọc gần nhất vào GuardLast
    HKEY k;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, DK_REG_KEY, 0, KEY_READ | KEY_WRITE, &k) == ERROR_SUCCESS) {
        if (RegGet(k, L"GuardDebug", 0)) {
            wchar_t exe[MAX_PATH] = {};
            GetModuleFileNameW(nullptr, exe, MAX_PATH);
            const wchar_t* name = wcsrchr(exe, L'\\');
            // 0.3.34: thanh địa chỉ cửa sổ ẩn danh Chrome báo InputScope mặc định nên chưa nhận ra được.
            // Khi dò lỗi, ở trình duyệt mà chưa nhận ra ô nào thì ghi thêm cây trợ năng (tên, loại ô)
            // để lần sau viết luật nhận thanh địa chỉ cho đúng. Không đổi cách gõ.
            if (kind == guard::NONE && !sawPrivate && name) {
                static const wchar_t* const kBrowsers[] = {L"chrome.exe", L"msedge.exe", L"firefox.exe", L"brave.exe",
                                                           L"opera.exe", L"vivaldi.exe", L"coccoc.exe"};
                for (const wchar_t* b : kBrowsers) {
                    if (_wcsicmp(name + 1, b) == 0) {
                        std::wstring info;
                        guard::FocusIsProtected(info);
                        seen += L"[dò trợ năng] " + info;
                        break;
                    }
                }
            }
            std::wstring line = std::wstring(name ? name + 1 : exe) + L" | req=" + std::to_wstring((long)rq) +
                                L" hr=" + std::to_wstring((long)hr) + L" | " + (seen.empty() ? L"(không có InputScope)" : seen) +
                                L"| kind=" + std::to_wstring(kind);
            RegSetValueExW(k, L"GuardLast", 0, REG_SZ, (const BYTE*)line.c_str(), (DWORD)((line.size() + 1) * sizeof(wchar_t)));
        }
        RegCloseKey(k);
    }
    return kind;
}

// Ô hiện tại có cần tạm tắt gõ dấu không? Nhớ kết quả cho cùng một ô trong 0,5 giây
// để không hỏi lại ở từng phím; đổi ô (OnSetFocus) thì hỏi lại ngay.
int TextService::UpdateGuard(ITfContext* ctx, bool force) {
    ULONGLONG now = GetTickCount64();
    if (!force && ctx == guardCtx_ && now - guardTick_ < 500) return guard_;
    guardCtx_ = ctx;
    guardTick_ = now;
    guardUnsure_ = false;
    int kind = guard::FromFocusWindow();
    if (kind == guard::NONE) kind = ReadInputScopeGuard(ctx);
    if (guardUnsure_) guardTick_ = 0;   // 0.3.27: chưa biết chắc → phím sau hỏi lại
    if ((kind == guard::PASSWORD && !guardPassword_) || (kind == guard::URL && !guardUrl_) ||
        (kind == guard::CONSOLE && !guardConsole_) || (kind == guard::ASCII && !guardAscii_))
        kind = guard::NONE;
    if (kind != guard_) {
        guard_ = kind;
        if (lb_) lb_->Refresh();
    }
    return kind;
}

// Mỗi tuần một lần: nhờ DhammaKeySettings.exe kiểm tra bản mới (bộ gõ không tự dùng mạng)
static void MaybeCheckUpdate() {
    HKEY k;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, DK_REG_KEY, 0, nullptr, 0, KEY_READ | KEY_WRITE, nullptr, &k, nullptr) != ERROR_SUCCESS)
        return;
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    DWORD today = (DWORD)((((ULONGLONG)ft.dwHighDateTime << 32) | ft.dwLowDateTime) / 864000000000ULL);
    bool due = RegGet(k, L"AutoUpdateCheck", 1) != 0 && today - RegGet(k, L"LastUpdateCheck", 0) >= 7;
    if (due) RegSetValueExW(k, L"LastUpdateCheck", 0, REG_DWORD, (const BYTE*)&today, sizeof(today));
    RegCloseKey(k);
    if (due) LaunchDhammaKeySettings(L"--check-update");
}

// Có bộ gõ tiếng Việt khác (UniKey, EVKey, GoTiengViet…) đang chạy không? Kiểm tra mỗi 15 giây là đủ.
void TextService::CheckConflict() {
    ULONGLONG now = GetTickCount64();
    if (lastConflictCheck_ && now - lastConflictCheck_ < 15000) return;
    lastConflictCheck_ = now;
    MaybeCheckUpdate();
    std::wstring name = conflict::FindRunning();
    if (name != conflictName_) {
        conflictName_ = name;
        if (lb_) lb_->Refresh();
    }
    if (!name.empty() && IsVietMethod(Method())) MaybeShowConflictNotice(name);
}
