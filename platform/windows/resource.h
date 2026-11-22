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
// Mã tài nguyên dùng chung cho DhammaKey.rc và mã C++ (chỉ #define, để rc.exe đọc được)

#define IDI_DHAMMAKEY 101
#define IDR_MENU_LOGO 300   // logo DhammaKey kèm slogan, không chấm — chân menu khay (0.3.20)

// Tray icon: IDI_TRAY_BASE + bộ chữ * 8 + chế độ * 2 + nền
//   bộ chữ: 0 = Latinh (V P E), 1 = bản địa (Vi Pā EN)   — registry TrayGlyph
//   chế độ: 0 = Việt, 1 = Pali, 2 = tắt, 3 = xung đột bộ gõ khác
//   nền:    0 = thanh tác vụ sáng (phím măng cụt), 1 = thanh tác vụ tối (phím cơm trắng)
#define IDI_TRAY_BASE 200
#define DK_TRAY_ID(glyph, mode, dark) (IDI_TRAY_BASE + (glyph) * 8 + (mode) * 2 + (dark))
