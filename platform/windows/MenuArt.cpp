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

#include "MenuArt.h"
#include "resource.h"
#include <algorithm>
#include <objidl.h>
#ifdef NOMINMAX
namespace Gdiplus { using std::min; using std::max; }  // GDI+ cần min/max khi có NOMINMAX
#endif
#include <gdiplus.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <cmath>
#include <vector>

extern HINSTANCE g_hInst;

namespace menuart {
namespace {

using namespace Gdiplus;

ULONG_PTR g_token = 0;
int g_dpi = 96;
float Px(float v) { return v * g_dpi / 96.0f; }

const Color kInk(255, 40, 40, 40);
const Color kGray(255, 110, 110, 110);
const Color kDot(255, 0x8C, 0xB2, 0x5A);      // Cuống xanh (chấm ṃ)
const Color kDotRim(255, 0x4B, 0x1E, 0x3F);   // Măng cụt

// ---------- biểu tượng ----------
void DrawGlyph(Graphics& g, const wchar_t* text, float s) {
    FontFamily fam(L"Segoe UI");
    Font font(&fam, s * 0.95f, FontStyleRegular, UnitPixel);
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);
    SolidBrush b(kInk);
    g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    g.DrawString(text, -1, &font, RectF(-s * 0.2f, -s * 0.12f, s * 1.4f, s * 1.2f), &sf, &b);
}

HBITMAP NewDib(int w, int h, void** bits) {
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;  // từ trên xuống
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    return CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, bits, nullptr, 0);
}

// ---------- logo ----------
Bitmap* g_logo = nullptr;  // logo kèm slogan, không có chấm (res/menu-logo.png, cao 240)

Bitmap* LoadLogo() {
    if (g_logo) return g_logo;
    HRSRC r = FindResourceW(g_hInst, MAKEINTRESOURCEW(IDR_MENU_LOGO), RT_RCDATA);
    if (!r) return nullptr;
    DWORD n = SizeofResource(g_hInst, r);
    const void* data = LockResource(LoadResource(g_hInst, r));
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, n);
    if (!data || !mem) return nullptr;
    memcpy(GlobalLock(mem), data, n);
    GlobalUnlock(mem);
    IStream* st = nullptr;
    if (FAILED(CreateStreamOnHGlobal(mem, TRUE, &st))) { GlobalFree(mem); return nullptr; }
    g_logo = Bitmap::FromStream(st);
    st->Release();
    if (g_logo && g_logo->GetLastStatus() != Ok) { delete g_logo; g_logo = nullptr; }
    return g_logo;
}

// ---------- hoạt cảnh (toạ độ logo: logo cao 100 đơn vị, rộng 423) ----------
// Giống mẫu đã duyệt (DhammaKey-0.3.20-xem-truoc-menu-khay-logo-2.mp4, tools: anim2.py).
struct Frame { float x, yb, scale, squash, ground; };
const float kRestX = 48.8f, kRestY = 67.8f, kR = 4.2f;
const float kKeyFloor = kRestY + kR;   // chân chấm khi nằm trên phím
const float kFloor = 126.0f;           // nền menu dưới đáy phím (đáy phím ở 96)
std::vector<Frame> g_track;

void Rest(int n, float x = kRestX, float g = kKeyFloor) { for (int i = 0; i < n; ++i) g_track.push_back({x, g, 1, 1, g}); }
void Squash(float x, float g, std::initializer_list<float> k) { for (float q : k) g_track.push_back({x, g, 1, q, g}); }
void Hop(float x0, float g0, float x1, float g1, float h, int n, float smid = 1.0f, float sw = -1.0f) {
    float top = (g0 < g1 ? g0 : g1) - h;
    float b = top - std::sqrt(std::fmax(0.0f, (g0 - top) * (g1 - top)));  // điểm điều khiển Bezier
    for (int i = 1; i <= n; ++i) {
        float t = (float)i / n;
        float x = x0 + (x1 - x0) * t;
        float y = (1 - t) * (1 - t) * g0 + 2 * t * (1 - t) * b + t * t * g1;
        float sc = 1.0f + (smid - 1.0f) * 4 * t * (1 - t);  // to dần khi bay cao (lại gần mắt)
        float g = (sw < 0 || t < sw) ? g0 : g1;
        g_track.push_back({x, y, sc, 1, g});
    }
}

void BuildTrack() {
    if (!g_track.empty()) return;
    const float X = kRestX;
    Rest(36);
    const struct { float h; int n; } hops[] = {{16, 14}, {9, 11}, {4, 8}};
    // (không đặt tên "small": windows.h có #define small char)
    for (auto& s : hops) { Hop(X, kKeyFloor, X, kKeyFloor, s.h, s.n); Squash(X, kKeyFloor, {0.75f, 0.92f}); }
    Rest(10);
    Squash(X, kKeyFloor, {0.7f, 0.6f, 0.7f, 0.9f});                         // nhún lấy đà
    Hop(X, kKeyFloor, 74, kFloor, 30, 24, 1.7f, 0.5f);                       // bật cao, ra khỏi phím
    Squash(74, kFloor, {0.55f, 0.78f, 0.95f});
    Hop(74, kFloor, 90, kFloor, 12, 12, 1.15f); Squash(90, kFloor, {0.7f, 0.9f});
    Hop(90, kFloor, 98, kFloor, 5, 8); Squash(98, kFloor, {0.85f});
    Rest(14, 98, kFloor);
    Squash(98, kFloor, {0.75f, 0.65f, 0.8f});
    Hop(98, kFloor, X, kKeyFloor, 40, 26, 1.45f, 0.55f);                     // nhảy về phím
    Squash(X, kKeyFloor, {0.6f, 0.8f, 0.95f});
    Hop(X, kKeyFloor, X, kKeyFloor, 6, 8); Squash(X, kKeyFloor, {0.9f});
    Rest(40);
}

// ---------- chân menu ----------
const float kLogoH = 60.0f;            // logo cao 60 px ở 96 dpi
float FootW() { return Px(300); }
float FootH() { return Px(std::floor(kLogoH * 1.45f) + 14); }

struct Owner {
    HWND menuWnd = nullptr;
    RECT item = {};
    size_t frame = 0;
    bool timer = false;
};

void PaintFooter(HDC dc, const RECT& rc, HWND menuWnd, size_t frame) {
    int w = rc.right - rc.left, h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0) return;
    HDC mem = CreateCompatibleDC(dc);
    void* bits = nullptr;
    HBITMAP bmp = NewDib(w, h, &bits);
    HGDIOBJ old = SelectObject(mem, bmp);
    RECT r = {0, 0, w, h};
    HTHEME th = menuWnd ? OpenThemeData(menuWnd, L"MENU") : nullptr;
    if (th) { DrawThemeBackground(th, mem, MENU_POPUPBACKGROUND, 0, &r, nullptr); CloseThemeData(th); }
    else FillRect(mem, &r, GetSysColorBrush(COLOR_MENU));
    {
        Graphics g(mem);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        g.SetPixelOffsetMode(PixelOffsetModeHalf);
        float lh = Px(kLogoH), u = lh / 100.0f;
        Bitmap* logo = LoadLogo();
        float lw = logo ? lh * logo->GetWidth() / logo->GetHeight() : lh * 4.23f;
        float lx = (w - lw) / 2.0f, ly = Px(6);
        if (logo) g.DrawImage(logo, RectF(lx, ly, lw, lh));
        // © canh giữa câu slogan (slogan từ mép chữ D 131,5 tới 419,6 → giữa 275,5)
        FontFamily fam(L"Segoe UI");
        Font font(&fam, Px(12), FontStyleRegular, UnitPixel);
        StringFormat sf;
        sf.SetAlignment(StringAlignmentCenter);
        sf.SetLineAlignment(StringAlignmentCenter);
        SolidBrush gray(kGray);
        g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
        g.DrawString(L"© 2026 Ujudhammo", -1, &font, PointF(lx + 275.5f * u, ly + 112.0f * u), &sf, &gray);
        // chấm xanh
        const Frame& f = g_track[frame % g_track.size()];
        float r0 = kR * f.scale;
        float lift = f.ground - f.yb;
        if (lift < 0) lift = 0;
        if (f.ground == kFloor || lift > 0.5f) {   // bóng đổ: bay càng cao càng nhạt, nhỏ
            float a = 90.0f * std::fmax(0.15f, 1.0f - lift / 40.0f);
            float sw = r0 * 1.2f * std::fmax(0.5f, 1.0f - lift / 60.0f);
            SolidBrush sh(f.ground == kFloor ? Color((BYTE)a, 0, 0, 0) : Color((BYTE)a, 20, 5, 15));
            g.FillEllipse(&sh, RectF(lx + (f.x - sw) * u, ly + (f.ground - 0.9f) * u, 2 * sw * u, 1.8f * u));
        }
        float rx = r0 / std::sqrt(f.squash), ry = r0 * f.squash;
        RectF dot(lx + (f.x - rx) * u, ly + (f.yb - 2 * ry) * u, 2 * rx * u, 2 * ry * u);
        SolidBrush fill(kDot);
        Pen rim(kDotRim, std::fmax(1.0f, 1.4f * u));
        g.FillEllipse(&fill, dot);
        g.DrawEllipse(&rim, dot);
    }
    BitBlt(dc, rc.left, rc.top, w, h, mem, 0, 0, SRCCOPY);
    SelectObject(mem, old);
    DeleteObject(bmp);
    DeleteDC(mem);
}

const wchar_t kClass[] = L"DhammaKeyMenuOwner";

LRESULT CALLBACK OwnerProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    Owner* o = (Owner*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    switch (msg) {
    case WM_MEASUREITEM: {
        auto* mi = (MEASUREITEMSTRUCT*)lp;
        if (mi->CtlType == ODT_MENU && mi->itemID == ID_FOOTER) {
            mi->itemWidth = (UINT)FootW();
            mi->itemHeight = (UINT)FootH();
            return TRUE;
        }
        break;
    }
    case WM_DRAWITEM: {
        auto* di = (DRAWITEMSTRUCT*)lp;
        if (o && di->CtlType == ODT_MENU && di->itemID == ID_FOOTER) {
            o->menuWnd = WindowFromDC(di->hDC);
            o->item = di->rcItem;
            PaintFooter(di->hDC, di->rcItem, o->menuWnd, o->frame);
            if (!o->timer) { SetTimer(hwnd, 1, 33, nullptr); o->timer = true; }   // ~30 hình/giây
            return TRUE;
        }
        break;
    }
    case WM_TIMER:
        if (o && wp == 1 && o->menuWnd && IsWindow(o->menuWnd)) {
            ++o->frame;
            HDC dc = GetDC(o->menuWnd);   // vẽ thẳng, không chờ WM_PAINT của menu
            if (dc) { PaintFooter(dc, o->item, o->menuWnd, o->frame); ReleaseDC(o->menuWnd, dc); }
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

} // namespace

bool Begin() {
    HDC sc = GetDC(nullptr);
    g_dpi = sc ? GetDeviceCaps(sc, LOGPIXELSY) : 96;
    if (sc) ReleaseDC(nullptr, sc);
    if (g_token) return true;
    GdiplusStartupInput in;
    if (GdiplusStartup(&g_token, &in, nullptr) != Ok) { g_token = 0; return false; }
    BuildTrack();
    return true;
}

void End() {
    delete g_logo;
    g_logo = nullptr;
    if (g_token) { GdiplusShutdown(g_token); g_token = 0; }
}

HBITMAP MakeIcon(Icon kind) {
    if (!g_token) return nullptr;
    int s = (int)std::lround(Px(16));
    // 0.3.33 (đã duyệt mẫu 16/11/2026): đệm vùng trong suốt bên phải để chữ dời ra, cách biểu tượng ~12 px
    // (trước đó ~4 px, sát chữ). Hình vẽ vẫn ở góc trái, kích thước s x s.
    int w = s + (int)std::lround(Px(8));
    void* bits = nullptr;
    HBITMAP bmp = NewDib(w, s, &bits);
    if (!bmp) return nullptr;
    memset(bits, 0, (size_t)w * s * 4);
    {
        Bitmap canvas(w, s, w * 4, PixelFormat32bppPARGB, (BYTE*)bits);
        Graphics g(&canvas);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetPixelOffsetMode(PixelOffsetModeHalf);
        float k = s / 16.0f;
        Pen pen(kInk, std::fmax(1.0f, 1.3f * k));
        pen.SetLineCap(LineCapRound, LineCapRound, DashCapRound);
        pen.SetLineJoin(LineJoinRound);
        SolidBrush ink(kInk);
        switch (kind) {
        case ICON_PALI: DrawGlyph(g, L"ā", (float)s); break;      // ā
        case ICON_CHARSET: DrawGlyph(g, L"ẫ", (float)s); break;   // ẫ
        case ICON_OFF: {  // bàn phím gạch chéo
            GraphicsPath p;
            RectF r(1 * k, 4 * k, 14 * k, 8.5f * k);
            float rr = 2 * k;
            p.AddArc(r.X, r.Y, rr * 2, rr * 2, 180, 90);
            p.AddArc(r.GetRight() - rr * 2, r.Y, rr * 2, rr * 2, 270, 90);
            p.AddArc(r.GetRight() - rr * 2, r.GetBottom() - rr * 2, rr * 2, rr * 2, 0, 90);
            p.AddArc(r.X, r.GetBottom() - rr * 2, rr * 2, rr * 2, 90, 90);
            p.CloseFigure();
            g.DrawPath(&pen, &p);
            for (int i = 0; i < 4; ++i) g.FillEllipse(&ink, (3.6f + i * 2.6f) * k - 0.8f * k, 7 * k - 0.8f * k, 1.6f * k, 1.6f * k);
            g.DrawLine(&pen, 5 * k, 10 * k, 11 * k, 10 * k);
            g.DrawLine(&pen, 1.5f * k, 1.5f * k, 14.5f * k, 14.5f * k);
            break;
        }
        case ICON_SETTINGS: {  // ba thanh trượt
            const float rows[3][2] = {{4, 10}, {8, 5}, {12, 9}};
            SolidBrush clear(Color(0, 0, 0, 0));
            for (auto& rw : rows) {
                g.DrawLine(&pen, 1.5f * k, rw[0] * k, 14.5f * k, rw[0] * k);
                g.SetCompositingMode(CompositingModeSourceCopy);   // khoét lỗ dưới núm cho rõ
                g.FillEllipse(&clear, (rw[1] - 1.8f) * k, (rw[0] - 1.8f) * k, 3.6f * k, 3.6f * k);
                g.SetCompositingMode(CompositingModeSourceOver);
                g.DrawEllipse(&pen, (rw[1] - 1.8f) * k, (rw[0] - 1.8f) * k, 3.6f * k, 3.6f * k);
            }
            break;
        }
        case ICON_CS_UNICODE:  // quả cầu: Unicode, chữ dùng chung mọi ngôn ngữ (0.3.34)
            g.DrawEllipse(&pen, 1.5f * k, 1.5f * k, 13.0f * k, 13.0f * k);
            g.DrawEllipse(&pen, 5.0f * k, 1.5f * k, 6.0f * k, 13.0f * k);
            g.DrawLine(&pen, 1.5f * k, 8.0f * k, 14.5f * k, 8.0f * k);
            break;
        case ICON_CS_COMBINING:  // chữ a kèm dấu rời: Unicode tổ hợp
            g.TranslateTransform(-2.0f * k, 2.0f * k);
            DrawGlyph(g, L"a", (float)s);
            g.ResetTransform();
            g.DrawLine(&pen, 10.0f * k, 3.0f * k, 13.0f * k, 1.5f * k);
            g.DrawLine(&pen, 8.5f * k, 5.0f * k, 11.0f * k, 3.5f * k);
            break;
        case ICON_CS_OLDFONT: {  // tờ giấy gập góc: bảng mã theo phông cũ
            PointF poly[5] = {PointF(3 * k, 1.5f * k), PointF(10 * k, 1.5f * k), PointF(13.5f * k, 5 * k),
                              PointF(13.5f * k, 14.5f * k), PointF(3 * k, 14.5f * k)};
            g.DrawPolygon(&pen, poly, 5);
            g.DrawLine(&pen, 10 * k, 1.5f * k, 10 * k, 5 * k);
            g.DrawLine(&pen, 10 * k, 5 * k, 13.5f * k, 5 * k);
            g.DrawLine(&pen, 5.5f * k, 9 * k, 11 * k, 9 * k);
            g.DrawLine(&pen, 5.5f * k, 11.8f * k, 11 * k, 11.8f * k);
            break;
        }
        case ICON_CS_TEXT:  // </>: bảng mã viết bằng chữ thường (VIQR, NCR)
            g.DrawLine(&pen, 5.5f * k, 4 * k, 1.5f * k, 8 * k);
            g.DrawLine(&pen, 1.5f * k, 8 * k, 5.5f * k, 12 * k);
            g.DrawLine(&pen, 10.5f * k, 4 * k, 14.5f * k, 8 * k);
            g.DrawLine(&pen, 14.5f * k, 8 * k, 10.5f * k, 12 * k);
            g.DrawLine(&pen, 9.2f * k, 2.5f * k, 6.8f * k, 13.5f * k);
            break;
        case ICON_UPDATE: {  // hai cung tròn có mũi tên: kiểm tra cập nhật (0.4.0)
            const float cx = 8 * k, cy = 8 * k, r = 6 * k;
            g.DrawArc(&pen, cx - r, cy - r, 2 * r, 2 * r, 200, 140);
            g.DrawArc(&pen, cx - r, cy - r, 2 * r, 2 * r, 20, 140);
            auto head = [&](float deg) {
                float a = deg * 3.14159265f / 180.0f;
                float px = cx + r * std::cos(a), py = cy + r * std::sin(a);
                float bx = std::sin(a), by = -std::cos(a);   // lùi ngược chiều cung
                for (float turn : {-0.6f, 0.6f}) {
                    float cs = std::cos(turn), sn = std::sin(turn);
                    float dx = bx * cs - by * sn, dy = bx * sn + by * cs;
                    g.DrawLine(&pen, px, py, px + 3.2f * k * dx, py + 3.2f * k * dy);
                }
            };
            head(340);
            head(160);
            break;
        }
        case ICON_ABOUT:  // chữ i trong vòng tròn
            g.DrawEllipse(&pen, 1.2f * k, 1.2f * k, 13.6f * k, 13.6f * k);
            g.DrawLine(&pen, 8 * k, 7.2f * k, 8 * k, 11.6f * k);
            g.FillEllipse(&ink, 8 * k - 1.1f * k, 4.6f * k - 1.1f * k, 2.2f * k, 2.2f * k);
            break;
        }
    }
    return bmp;
}

HWND CreateOwner(POINT pt) {
    // Lớp cửa sổ đăng ký mỗi lần mở menu và gỡ khi đóng: DLL bộ gõ có thể bị gỡ khỏi tiến trình
    // bất cứ lúc nào, để lại lớp trỏ vào mã đã mất là ứng dụng sẽ đổ.
    bool registered = false;
    {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = OwnerProc;
        wc.hInstance = g_hInst;
        wc.lpszClassName = kClass;
        registered = RegisterClassW(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
    }
    if (!registered) return nullptr;
    HWND w = CreateWindowExW(WS_EX_TOOLWINDOW, kClass, L"", WS_POPUP, pt.x, pt.y, 0, 0,
                             nullptr, nullptr, g_hInst, nullptr);
    if (w) SetWindowLongPtrW(w, GWLP_USERDATA, (LONG_PTR)new Owner());
    return w;
}

void DestroyOwner(HWND owner) {
    if (!owner) return;
    KillTimer(owner, 1);
    delete (Owner*)GetWindowLongPtrW(owner, GWLP_USERDATA);
    SetWindowLongPtrW(owner, GWLP_USERDATA, 0);
    DestroyWindow(owner);
    UnregisterClassW(kClass, g_hInst);
}

} // namespace menuart
