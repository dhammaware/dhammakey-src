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

#pragma once
// Menu chuột phải ở khay (0.3.20, đã duyệt mẫu 15/11/2026): biểu tượng nét mảnh cho các mục,
// chân menu tự vẽ có logo DhammaKey kèm slogan, chấm xanh của chữ ṃ nảy như bóng tennis
// (nảy tại chỗ → bật cao, to dần → rơi ra khỏi phím xuống nền menu → nảy thấp dần → nhảy về chỗ cũ),
// dòng © 2026 Ujudhammo canh giữa câu slogan. Vẽ bằng GDI+ (có từ Windows XP), nền theo uxtheme.
#include <windows.h>

namespace menuart {

enum Icon { ICON_PALI, ICON_OFF, ICON_CHARSET, ICON_SETTINGS, ICON_ABOUT,
           ICON_CS_UNICODE, ICON_CS_COMBINING, ICON_CS_OLDFONT, ICON_CS_TEXT,   // danh sách Bảng mã (0.3.34)
           ICON_UPDATE };   // mũi tên vòng: Kiểm tra cập nhật (0.4.0)
constexpr UINT ID_FOOTER = 900;  // mục chân menu (tự vẽ, không bấm được)

bool Begin();                    // khởi động GDI+ — gọi trước khi dựng menu
void End();                      // tắt GDI+ — gọi sau khi menu đóng
HBITMAP MakeIcon(Icon kind);     // ảnh 32-bit có kênh alpha cho MENUITEMINFO::hbmpItem; người gọi DeleteObject
HWND CreateOwner(POINT pt);      // cửa sổ chủ của menu: đo, vẽ và chạy hoạt cảnh chân menu
void DestroyOwner(HWND owner);

} // namespace menuart
