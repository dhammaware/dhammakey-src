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
/*
 * BẢNG PHÍM ALT PHẢI (AltGr) CHO PĀLI/SANSKRIT — NƠI DUY NHẤT ĐỊNH NGHĨA (0.3.31).
 *
 * Muốn đổi phím, thêm hay bớt chữ: chỉ sửa bảng này. Lõi (pali.cpp), bộ gõ Windows (TextService),
 * bảng trong Cài đặt đều đọc từ đây. Sau khi sửa nhớ cập nhật README.md, trang web (app.js) và
 * "Có gì mới". Tránh phím trùng phím tắt của hệ thống/ứng dụng (ví dụ Alt+G của Gemini).
 *
 *   key      : phím (chữ thường); Shift cho chữ hoa
 *   lower    : chữ ra (UTF-8). Riêng m: ṃ (chấm dưới) — khi chọn chấm trên thì ra ṁ
 *   sanskrit : 1 = chữ chỉ dùng cho Sanskrit
 */
typedef struct {
    char key;
    const char* lower;
    int sanskrit;
} dk_pali_alt_key;

static const dk_pali_alt_key kDkPaliAltKeys[] = {
    {'a', u8"ā", 0}, {'i', u8"ī", 0}, {'u', u8"ū", 0}, {'m', u8"ṃ", 0}, {'n', u8"ṇ", 0},
    {'k', u8"ṅ", 0}, {'j', u8"ñ", 0}, {'t', u8"ṭ", 0}, {'d', u8"ḍ", 0}, {'l', u8"ḷ", 0},
    {'s', u8"ṣ", 1}, {'c', u8"ś", 1}, {'r', u8"ṛ", 1}, {'h', u8"ḥ", 1},
};
#define DK_PALI_ALT_COUNT ((int)(sizeof(kDkPaliAltKeys) / sizeof(kDkPaliAltKeys[0])))
