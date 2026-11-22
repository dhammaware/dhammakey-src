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

#include "LangBar.h"
#include "Guard.h"
#include "TextService.h"
#include "MenuArt.h"
#include <olectl.h>
#include <shellapi.h>
#include <string>
#include <vector>

namespace {

// GUID_LBI_INPUTMODE: Windows 8+ chỉ hiện ở khay hệ thống mục mang GUID này
const GUID kLbiInputMode = {0x2C77A81E, 0x41CC, 0x4178, {0xA3, 0xA7, 0x5F, 0x8A, 0x98, 0x75, 0x68, 0xE6}};
const DWORD kSinkCookie = 0x444B4C42; // "DKLB"

enum { ID_TELEX = 1 + DK_METHOD_TELEX, ID_VNI = 1 + DK_METHOD_VNI, ID_VIQR = 1 + DK_METHOD_VIQR,
       ID_PALI = 1 + DK_METHOD_PALI_VELTHUIS, ID_OFF = 1 + DK_METHOD_OFF, ID_SETTINGS = 100, ID_ABOUT = 101, ID_UPDATE = 102,
       ID_CHARSET = 200 /* + DK_CHARSET_* (bảng mã, 0.3.20) */ };

// Bảng mã trong menu khay (0.3.20, đã duyệt mẫu 15/11/2026): tên đầy đủ trong danh sách, tên ngắn
// trên dòng "Bảng mã: …"; nhóm = vạch ngăn trước mục đó.
// icon, tag: biểu tượng theo nhóm và nhãn nhóm bên phải (0.3.34, đã duyệt mẫu gộp A+B 16/11/2026).
struct CharsetItem { int id; const wchar_t* full; const wchar_t* shortName; bool sepBefore; int icon; const wchar_t* tag; };
static const CharsetItem kCharsets[] = {
    {DK_CHARSET_UNICODE, L"Unicode dựng sẵn", L"Unicode", false, menuart::ICON_CS_UNICODE, L"Mặc định"},
    {DK_CHARSET_UNICODE_COMBINING, L"Unicode tổ hợp", L"Unicode tổ hợp", false, menuart::ICON_CS_COMBINING, nullptr},
    {DK_CHARSET_TCVN3, L"TCVN3 (ABC) · .VnTime", L"TCVN3 (ABC)", true, menuart::ICON_CS_OLDFONT, L"Phông cũ"},
    {DK_CHARSET_VNI, L"VNI Windows · VNI-Times", L"VNI Windows", false, menuart::ICON_CS_OLDFONT, L"Phông cũ"},
    {DK_CHARSET_VPS, L"VPS", L"VPS", false, menuart::ICON_CS_OLDFONT, L"Phông cũ"},
    {DK_CHARSET_VISCII, L"VISCII", L"VISCII", false, menuart::ICON_CS_OLDFONT, L"Phông cũ"},
    {DK_CHARSET_CP1258, L"Windows CP 1258", L"CP 1258", false, menuart::ICON_CS_OLDFONT, L"Phông cũ"},
    {DK_CHARSET_VIQR, L"VIQR", L"VIQR", true, menuart::ICON_CS_TEXT, L"Chữ thường"},
    {DK_CHARSET_NCR_DEC, L"NCR thập phân · &#7879;", L"NCR thập phân", false, menuart::ICON_CS_TEXT, L"Chữ thường"},
    {DK_CHARSET_NCR_HEX, L"NCR thập lục · &#x1EC7;", L"NCR thập lục", false, menuart::ICON_CS_TEXT, L"Chữ thường"},
};

// Dòng trong danh sách Bảng mã: "&" phải viết đôi (trong menu "&" đánh dấu phím tắt, trước 0.3.34 chữ & của
// "&#7879;" biến mất); nhãn nhóm đứng sau dấu Tab nên căn sang phải như cột phím tắt.
static std::wstring CharsetMenuText(const CharsetItem& c, bool withTag) {
    std::wstring t;
    for (const wchar_t* p = c.full; *p; ++p) { if (*p == L'&') t += L'&'; t += *p; }
    if (withTag && c.tag) { t += L'\t'; t += c.tag; }
    return t;
}

static std::wstring CharsetLabel(int cs, bool en) {
    for (const auto& c : kCharsets)
        if (c.id == cs) return std::wstring(en ? L"Charset: " : L"Bảng mã: ") + c.shortName;
    return en ? L"Charset" : L"Bảng mã";
}

// Mở cửa sổ cài đặt: DhammaKeySettings.exe nằm cạnh DLL 64-bit (C:\Program Files\DhammaKey).
// DLL 32-bit nằm ở Program Files (x86) nên phải tìm sang thư mục 64-bit.
void LaunchSettings(const wchar_t* args = nullptr) {
    wchar_t path[MAX_PATH] = {};
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCWSTR)&LaunchSettings, &self);
    GetModuleFileNameW(self, path, MAX_PATH);
    std::wstring exe = path;
    size_t slash = exe.find_last_of(L'\\');
    exe = (slash == std::wstring::npos ? std::wstring() : exe.substr(0, slash + 1)) + L"DhammaKeySettings.exe";
    if (GetFileAttributesW(exe.c_str()) == INVALID_FILE_ATTRIBUTES) {
        wchar_t alt[MAX_PATH] = {};
        if (ExpandEnvironmentStringsW(L"%ProgramW6432%\\DhammaKey\\DhammaKeySettings.exe", alt, MAX_PATH))
            exe = alt;
    }
    ShellExecuteW(nullptr, L"open", exe.c_str(), args, nullptr, SW_SHOWNORMAL);
}

const wchar_t* MethodName(int m) {
    switch (m) {
        case DK_METHOD_TELEX: return L"Telex";
        case DK_METHOD_VNI: return L"VNI";
        case DK_METHOD_VIQR: return L"VIQR";
        case DK_METHOD_PALI_VELTHUIS: return L"Pāli";
    }
    return L"Tắt";
}

// Dự phòng khi không nạp được tray icon từ tài nguyên: vẽ phím vuông màu có một chữ trắng
HICON MakeIcon(wchar_t letter, COLORREF bg) {
    int sz = GetSystemMetrics(SM_CXSMICON);
    if (sz <= 0) sz = 16;
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = sz;
    bi.bmiHeader.biHeight = -sz;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HDC screen = GetDC(nullptr);
    HBITMAP color = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!color) { ReleaseDC(nullptr, screen); return nullptr; }
    HDC dc = CreateCompatibleDC(screen);
    HGDIOBJ oldBmp = SelectObject(dc, color);

    RECT rc = {0, 0, sz, sz};
    HBRUSH br = CreateSolidBrush(bg);
    FillRect(dc, &rc, br);
    DeleteObject(br);

    HFONT font = CreateFontW(-(sz * 13 / 16), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                             DEFAULT_PITCH, L"Segoe UI");
    HGDIOBJ oldFont = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    wchar_t s[2] = {letter, 0};
    DrawTextW(dc, s, 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, oldFont);
    DeleteObject(font);
    SelectObject(dc, oldBmp);
    DeleteDC(dc);
    ReleaseDC(nullptr, screen);
    GdiFlush();

    // GDI không ghi kênh alpha: đặt đục toàn bộ, chừa 4 góc trong suốt cho mềm
    DWORD* px = (DWORD*)bits;
    for (int y = 0; y < sz; ++y)
        for (int x = 0; x < sz; ++x) {
            bool corner = (x == 0 || x == sz - 1) && (y == 0 || y == sz - 1);
            px[y * sz + x] = corner ? 0 : (px[y * sz + x] | 0xFF000000);
        }

    std::vector<BYTE> zeros(((sz + 15) / 16) * 2 * sz, 0);
    HBITMAP mask = CreateBitmap(sz, sz, 1, 1, zeros.data());
    ICONINFO ii = {};
    ii.fIcon = TRUE;
    ii.hbmMask = mask;
    ii.hbmColor = color;
    HICON icon = CreateIconIndirect(&ii);
    DeleteObject(mask);
    DeleteObject(color);
    return icon;
}

DWORD ReadDword(HKEY root, const wchar_t* key, const wchar_t* name, DWORD def) {
    HKEY k;
    if (RegOpenKeyExW(root, key, 0, KEY_READ, &k) != ERROR_SUCCESS) return def;
    DWORD v = def, sz = sizeof(v), type = 0;
    if (RegQueryValueExW(k, name, nullptr, &type, (BYTE*)&v, &sz) != ERROR_SUCCESS || type != REG_DWORD) v = def;
    RegCloseKey(k);
    return v;
}

// Thanh tác vụ sáng? (Windows 10 1903+ có giá trị SystemUsesLightTheme; máy cũ hơn: thanh tác vụ tối)
bool TaskbarIsLight() {
    return ReadDword(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                     L"SystemUsesLightTheme", 0) != 0;
}

// Ngôn ngữ giao diện (0.4.0, bước 2): cùng registry UiLang với Cài đặt. 0 = Tiếng Việt, 1 = English.
bool UiEnglish() { return ReadDword(HKEY_CURRENT_USER, DK_REG_KEY, L"UiLang", 0) == 1; }

// Bộ chữ trên tray icon (registry TrayGlyph): 0 = Latinh, 1 = bản địa.
// Chưa chọn: máy ở Việt Nam dùng chữ Latinh, nước khác dùng chữ bản địa. Bảng điều khiển dùng cùng quy tắc.
int TrayGlyph() {
    const DWORD kVietnamGeoId = 251;
    DWORD def = GetUserGeoID(GEOCLASS_NATION) == (GEOID)kVietnamGeoId ? 0 : 1;
    return ReadDword(HKEY_CURRENT_USER, DK_REG_KEY, L"TrayGlyph", def) ? 1 : 0;
}

// Nạp tray icon theo kiểu gõ, bộ chữ và màu thanh tác vụ; tệp .ico có sẵn 16/20/24/32 px
// nên ở 100/125/150/200% Windows lấy đúng cỡ, không phải co giãn.
HICON LoadTrayIcon(int mode) {
    int sz = GetSystemMetrics(SM_CXSMICON);
    if (sz <= 0) sz = 16;
    int id = DK_TRAY_ID(TrayGlyph(), mode, TaskbarIsLight() ? 0 : 1);
    return (HICON)LoadImageW(g_hInst, MAKEINTRESOURCEW(id), IMAGE_ICON, sz, sz, LR_DEFAULTCOLOR);
}

} // namespace

void LaunchDhammaKeySettings(const wchar_t* args) { LaunchSettings(args); }

LangBarButton::LangBarButton(TextService* ts) : ts_(ts) {}

LangBarButton::~LangBarButton() {
    if (sink_) sink_->Release();
}

STDMETHODIMP LangBarButton::QueryInterface(REFIID riid, void** ppv) {
    if (!ppv) return E_INVALIDARG;
    *ppv = nullptr;
    if (riid == IID_IUnknown || riid == IID_ITfLangBarItem || riid == IID_ITfLangBarItemButton)
        *ppv = static_cast<ITfLangBarItemButton*>(this);
    else if (riid == IID_ITfSource)
        *ppv = static_cast<ITfSource*>(this);
    if (!*ppv) return E_NOINTERFACE;
    AddRef();
    return S_OK;
}
STDMETHODIMP_(ULONG) LangBarButton::AddRef() { return InterlockedIncrement(&refs_); }
STDMETHODIMP_(ULONG) LangBarButton::Release() {
    LONG r = InterlockedDecrement(&refs_);
    if (!r) delete this;
    return r;
}

STDMETHODIMP LangBarButton::GetInfo(TF_LANGBARITEMINFO* info) {
    if (!info) return E_INVALIDARG;
    info->clsidService = CLSID_DhammaKey;
    info->guidItem = kLbiInputMode;
    info->dwStyle = TF_LBI_STYLE_BTN_BUTTON | TF_LBI_STYLE_SHOWNINTRAY;
    info->ulSort = 0;
    lstrcpynW(info->szDescription, L"DhammaKey", TF_LBI_DESC_MAXLEN);
    return S_OK;
}

STDMETHODIMP LangBarButton::GetStatus(DWORD* status) {
    if (!status) return E_INVALIDARG;
    *status = 0;
    return S_OK;
}

STDMETHODIMP LangBarButton::GetTooltipString(BSTR* tip) {
    if (!tip) return E_INVALIDARG;
    int m = ts_ ? ts_->Method() : DK_METHOD_OFF;
    std::wstring s = L"DhammaKey – ";
    s += MethodName(m);
    if (ts_ && ts_->Guard() && m != DK_METHOD_OFF)
        s += std::wstring(L"\n") + guard::KindName(ts_->Guard()) + L" — tạm tắt gõ dấu";
    else if (ts_ && ts_->HasConflict() && m != DK_METHOD_OFF)
        s += L"\nCảnh báo: " + ts_->ConflictName() + L" đang chạy, có thể gõ sai. Hãy thoát " + ts_->ConflictName() + L".";
    else
        s += UiEnglish() ? L" (right-click to choose a mode)" : L" (bấm chuột phải để chọn kiểu gõ)";
    *tip = SysAllocString(s.c_str());
    return *tip ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP LangBarButton::GetText(BSTR* text) {
    if (!text) return E_INVALIDARG;
    *text = SysAllocString(MethodName(ts_ ? ts_->Method() : DK_METHOD_OFF));
    return *text ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP LangBarButton::GetIcon(HICON* icon) {
    if (!icon) return E_INVALIDARG;
    int m = ts_ ? ts_->Method() : DK_METHOD_OFF;
    // Chế độ trong DK_TRAY_ID: 0 Việt · 1 Pali · 2 tắt · 3 xung đột (V đỏ)
    int mode = 0;
    if (m == DK_METHOD_OFF || (ts_ && ts_->Guard())) mode = 2;   // ô mật khẩu… hiện như tắt (0.3.23)
    else if (m == DK_METHOD_PALI_VELTHUIS) mode = 1;
    else if (ts_ && ts_->HasConflict()) mode = 3;
    *icon = LoadTrayIcon(mode);
    if (!*icon) {
        static const wchar_t letters[] = {L'V', L'P', L'E', L'V'};
        static const COLORREF colors[] = {RGB(0x4B, 0x1E, 0x3F), RGB(0x4B, 0x1E, 0x3F), RGB(110, 110, 110),
                                          RGB(0xB3, 0x26, 0x1E)};
        *icon = MakeIcon(letters[mode], colors[mode]);
    }
    return *icon ? S_OK : E_FAIL;
}

STDMETHODIMP LangBarButton::OnClick(TfLBIClick click, POINT pt, const RECT*) {
    if (!ts_) return S_OK;
    if (click == TF_LBI_CLK_LEFT) ts_->ToggleOnOff();
    else ShowMenu(pt);
    return S_OK;
}

// Menu kiểu thanh ngôn ngữ (Windows 7)
STDMETHODIMP LangBarButton::InitMenu(ITfMenu* menu) {
    if (!menu || !ts_) return S_OK;
    int m = ts_->Method();
    const bool en = UiEnglish();
    struct { UINT id; const wchar_t* text; } items[] = {
        {ID_TELEX, L"Telex"}, {ID_VNI, L"VNI"}, {ID_VIQR, L"VIQR"},
        {ID_PALI, L"Pāli (Velthuis)"}, {ID_OFF, en ? L"Off – type English" : L"Tắt – gõ tiếng Anh"},
    };
    for (auto& it : items) {
        DWORD fl = ((int)it.id - 1 == m) ? TF_LBMENUF_CHECKED : 0;
        menu->AddMenuItem(it.id, fl, nullptr, nullptr, it.text, (ULONG)wcslen(it.text), nullptr);
    }
    menu->AddMenuItem(0, TF_LBMENUF_SEPARATOR, nullptr, nullptr, nullptr, 0, nullptr);
    {   // Bảng mã (0.3.20)
        std::wstring label = CharsetLabel(ts_->Charset(), en);
        ITfMenu* sub = nullptr;
        menu->AddMenuItem(0, TF_LBMENUF_SUBMENU, nullptr, nullptr, label.c_str(), (ULONG)label.size(), &sub);
        if (sub) {
            int cur = ts_->Charset();
            for (const auto& c : kCharsets) {
                if (c.sepBefore) sub->AddMenuItem(0, TF_LBMENUF_SEPARATOR, nullptr, nullptr, nullptr, 0, nullptr);
                std::wstring txt = CharsetMenuText(c, false);
                sub->AddMenuItem(ID_CHARSET + c.id, c.id == cur ? TF_LBMENUF_CHECKED : 0, nullptr, nullptr,
                                 txt.c_str(), (ULONG)txt.size(), nullptr);
            }
            sub->Release();
        }
        menu->AddMenuItem(0, TF_LBMENUF_SEPARATOR, nullptr, nullptr, nullptr, 0, nullptr);
    }
    std::wstring settings = en ? L"DhammaKey control panel…" : L"Bảng điều khiển DhammaKey…";
    menu->AddMenuItem(ID_SETTINGS, 0, nullptr, nullptr, settings.c_str(), (ULONG)settings.size(), nullptr);
    std::wstring update = en ? L"Check for updates" : L"Kiểm tra cập nhật";
    menu->AddMenuItem(ID_UPDATE, 0, nullptr, nullptr, update.c_str(), (ULONG)update.size(), nullptr);
    menu->AddMenuItem(0, TF_LBMENUF_SEPARATOR, nullptr, nullptr, nullptr, 0, nullptr);  // ngăn "Về chúng tôi" (0.3.15)
    std::wstring about = en ? L"About DhammaKey\u2122" : L"Về DhammaKey\u2122";  // đổi từ "Về chúng tôi" (theo yêu cầu 15/11/2026, bản 0.3.19)
    menu->AddMenuItem(ID_ABOUT, 0, nullptr, nullptr, about.c_str(), (ULONG)about.size(), nullptr);
    return S_OK;
}

STDMETHODIMP LangBarButton::OnMenuSelect(UINT id) {
    if (id == ID_SETTINGS) { LaunchSettings(); return S_OK; }
    if (id == ID_UPDATE) { LaunchSettings(L"--check-update"); return S_OK; }
    if (id == ID_ABOUT) { LaunchSettings(L"--about"); return S_OK; }
    if (ts_ && id >= ID_CHARSET && id < ID_CHARSET + DK_CHARSET_COUNT) { ts_->ChooseCharset((int)id - ID_CHARSET); return S_OK; }
    if (ts_ && id >= 1 && id <= 5) ts_->ChooseMethod((int)id - 1);
    return S_OK;
}

void LangBarButton::ShowMenu(POINT pt) {
    // Menu khay (0.3.20, đã duyệt mẫu DhammaKey-0.3.20-xem-truoc-menu-khay*.png/mp4 15/11/2026):
    // biểu tượng nét mảnh, mục "Bảng mã: …" mở danh sách bên cạnh, chân menu có logo kèm slogan
    // với chấm xanh nảy (MenuArt.cpp) và © 2026 Ujudhammo.
    int m = ts_->Method();
    const bool en = UiEnglish();
    HMENU menu = CreatePopupMenu();
    HMENU csMenu = CreatePopupMenu();
    if (!menu || !csMenu) { if (menu) DestroyMenu(menu); if (csMenu) DestroyMenu(csMenu); return; }
    bool art = menuart::Begin();
    std::vector<HBITMAP> bitmaps;
    auto setIcon = [&](UINT id, menuart::Icon kind, bool byPos = false) {
        if (!art) return;
        HBITMAP b = menuart::MakeIcon(kind);
        if (!b) return;
        bitmaps.push_back(b);
        MENUITEMINFOW mi = {};
        mi.cbSize = sizeof(mi);
        mi.fMask = MIIM_BITMAP;
        mi.hbmpItem = b;
        SetMenuItemInfoW(menu, id, byPos ? TRUE : FALSE, &mi);
    };
    auto add = [&](UINT id, const wchar_t* text) {
        AppendMenuW(menu, MF_STRING | (((int)id - 1 == m) ? MF_CHECKED : 0), id, text);
    };
    add(ID_TELEX, L"Telex");
    add(ID_VNI, L"VNI");
    add(ID_VIQR, L"VIQR");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    add(ID_PALI, L"Pāli (Velthuis)\tCtrl+Shift+P");
    setIcon(ID_PALI, menuart::ICON_PALI);
    add(ID_OFF, en ? L"Off – type English\tCtrl+`" : L"Tắt – gõ tiếng Anh\tCtrl+`");
    setIcon(ID_OFF, menuart::ICON_OFF);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    int cur = ts_->Charset();
    for (const auto& c : kCharsets) {
        if (c.sepBefore) AppendMenuW(csMenu, MF_SEPARATOR, 0, nullptr);
        std::wstring txt = CharsetMenuText(c, true);
        AppendMenuW(csMenu, MF_STRING | (c.id == cur ? MF_CHECKED : 0), ID_CHARSET + c.id, txt.c_str());
        if (art) {
            HBITMAP b = menuart::MakeIcon((menuart::Icon)c.icon);
            if (b) {
                bitmaps.push_back(b);
                MENUITEMINFOW mi = {};
                mi.cbSize = sizeof(mi);
                mi.fMask = MIIM_BITMAP;
                mi.hbmpItem = b;
                SetMenuItemInfoW(csMenu, ID_CHARSET + c.id, FALSE, &mi);
            }
        }
    }
    std::wstring csLabel = CharsetLabel(cur, en);
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)csMenu, csLabel.c_str());
    setIcon(GetMenuItemCount(menu) - 1, menuart::ICON_CHARSET, true);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, ID_SETTINGS, en ? L"DhammaKey control panel…" : L"Bảng điều khiển DhammaKey…");
    setIcon(ID_SETTINGS, menuart::ICON_SETTINGS);
    AppendMenuW(menu, MF_STRING, ID_UPDATE, en ? L"Check for updates" : L"Kiểm tra cập nhật");
    setIcon(ID_UPDATE, menuart::ICON_UPDATE);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);  // ngăn "Về DhammaKey™" với Bảng điều khiển (0.3.15)
    AppendMenuW(menu, MF_STRING, ID_ABOUT, en ? L"About DhammaKey\u2122" : L"Về DhammaKey\u2122");  // đổi từ "Về chúng tôi" (0.3.19)
    setIcon(ID_ABOUT, menuart::ICON_ABOUT);
    if (art) {   // chân menu: logo + chấm nảy + ©
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_OWNERDRAW | MF_DISABLED, menuart::ID_FOOTER, nullptr);
    }

    // Menu cần một cửa sổ chủ; cửa sổ ẩn này còn vẽ chân menu và chạy hoạt cảnh
    HWND prev = GetForegroundWindow();
    HWND owner = art ? menuart::CreateOwner(pt)
                     : CreateWindowExW(WS_EX_TOOLWINDOW, L"STATIC", L"", WS_POPUP,
                                       pt.x, pt.y, 0, 0, nullptr, nullptr, nullptr, nullptr);
    if (owner) SetForegroundWindow(owner);
    // Có chân menu tự vẽ thì không dùng TPM_NONOTIFY, để chắc cửa sổ chủ nhận WM_MEASUREITEM/WM_DRAWITEM
    UINT flags = TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | (art ? 0 : TPM_NONOTIFY);
    UINT cmd = (UINT)TrackPopupMenuEx(menu, flags,
                                      pt.x, pt.y, owner, nullptr);
    if (owner) { if (art) menuart::DestroyOwner(owner); else DestroyWindow(owner); }
    DestroyMenu(menu);   // huỷ luôn menu con csMenu
    for (HBITMAP b : bitmaps) DeleteObject(b);
    if (art) menuart::End();
    if (prev) SetForegroundWindow(prev);
    if (cmd == ID_SETTINGS) LaunchSettings();
    else if (cmd == ID_UPDATE) LaunchSettings(L"--check-update");
    else if (cmd == ID_ABOUT) LaunchSettings(L"--about");
    else if (cmd >= ID_CHARSET && cmd < (UINT)(ID_CHARSET + DK_CHARSET_COUNT) && ts_) ts_->ChooseCharset((int)cmd - ID_CHARSET);
    else if (cmd && cmd <= 5 && ts_) ts_->ChooseMethod((int)cmd - 1);
}

STDMETHODIMP LangBarButton::AdviseSink(REFIID riid, IUnknown* punk, DWORD* cookie) {
    if (!punk || !cookie) return E_INVALIDARG;
    if (riid != IID_ITfLangBarItemSink) return CONNECT_E_CANNOTCONNECT;
    if (sink_) return CONNECT_E_ADVISELIMIT;
    if (FAILED(punk->QueryInterface(IID_ITfLangBarItemSink, (void**)&sink_))) { sink_ = nullptr; return E_NOINTERFACE; }
    *cookie = kSinkCookie;
    return S_OK;
}

STDMETHODIMP LangBarButton::UnadviseSink(DWORD cookie) {
    if (cookie != kSinkCookie || !sink_) return CONNECT_E_NOCONNECTION;
    sink_->Release();
    sink_ = nullptr;
    return S_OK;
}

void LangBarButton::Refresh() {
    if (sink_) sink_->OnUpdate(TF_LBI_ICON | TF_LBI_TEXT | TF_LBI_TOOLTIP);
}

void LangBarButton::Detach() {
    ts_ = nullptr;
}
