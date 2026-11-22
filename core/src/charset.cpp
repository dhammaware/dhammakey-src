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
#include "charset.hpp"
#include "../include/dhammakey.h"
#include <cstdio>

namespace dk {
namespace {
#include "charset_tables.inc"

const VnLegacy* find(char32_t c) {
    for (const VnLegacy& v : kVnLegacy)
        if (v.uni == c) return &v;
    return nullptr;
}

// Byte → ký tự Unicode đưa vào ứng dụng, để Windows tìm đúng nét chữ trong phông .VnTime, VNI-Times, VPS…
// Theo Windows-1252, TRỪ ba byte mà các phông tiếng Việt cũ (dựng từ phông Mac) không gắn theo 1252
// — đã soát bảng cmap của VNI-Aptima, VPS Thanh Hoa (15/11/2026, đã gửi phông):
//   0x80 → U+0080 (không phải €), 0x8E → U+008E (không phải Ž), 0x9E → U+009E (không phải ž).
// 0xB7 (ã trong TCVN3) giữ U+00B7 như 1252 — đã thử trên máy thật trong Word 15/11/2026: .VnTime nhận cả U+00B7 lẫn
// U+2219, .VnArialH chỉ nhận U+00B7 (U+2219 ra dấu chấm "Đ·"). Tệp VHTIME.TTF đã gửi (chỉ có U+2219)
// không cài trên máy thử, nên không lấy làm căn cứ.
// Năm ô trống của 1252 (81 8D 8F 90 9D) giữ nguyên giá trị như Windows làm.
char32_t legacyChar(unsigned char b) {
    static const char16_t hi[32] = {
        0x0080, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x008E, 0x008F,
        0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x009E, 0x0178};
    return (b >= 0x80 && b < 0xA0) ? (char32_t)hi[b - 0x80] : (char32_t)b;
}

// Windows CP 1258 là bảng mã thật của Windows: đưa vào ứng dụng đúng chữ Unicode của byte đó
// (chữ gốc + dấu thanh tổ hợp U+0300…), không qua phông riêng.
char32_t cp1258Char(unsigned char b) {
    static const char16_t hi[128] = {
        0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x008A, 0x2039, 0x0152, 0x008D, 0x008E, 0x008F,
        0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x009A, 0x203A, 0x0153, 0x009D, 0x009E, 0x0178,
        0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7, 0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
        0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
        0x00C0, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x00C5, 0x00C6, 0x00C7, 0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x0300, 0x00CD, 0x00CE, 0x00CF,
        0x0110, 0x00D1, 0x0309, 0x00D3, 0x00D4, 0x01A0, 0x00D6, 0x00D7, 0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x01AF, 0x0303, 0x00DF,
        0x00E0, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x00E5, 0x00E6, 0x00E7, 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x0301, 0x00ED, 0x00EE, 0x00EF,
        0x0111, 0x00F1, 0x0323, 0x00F3, 0x00F4, 0x01A1, 0x00F6, 0x00F7, 0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x01B0, 0x20AB, 0x00FF};
    return b < 0x80 ? (char32_t)b : (char32_t)hi[b - 0x80];
}

void putBytes(std::u32string& out, unsigned v, char32_t (*conv)(unsigned char)) {
    out += conv((unsigned char)(v & 0xFF));
    if (v >> 8) out += conv((unsigned char)(v >> 8));
}

void putNcr(std::u32string& out, char32_t c, bool hex) {
    char buf[16];
    std::snprintf(buf, sizeof buf, hex ? "&#x%X;" : "&#%u;", (unsigned)c);
    for (const char* p = buf; *p; ++p) out += (char32_t)*p;
}
} // namespace

std::u32string toCharset(const std::u32string& s, int cs) {
    if (cs == DK_CHARSET_UNICODE || cs == DK_CHARSET_UNICODE_COMBINING) return s;  // tổ hợp: engine lo
    std::u32string out;
    for (char32_t c : s) {
        if (c < 0x80) { out += c; continue; }
        if (cs == DK_CHARSET_NCR_DEC || cs == DK_CHARSET_NCR_HEX) { putNcr(out, c, cs == DK_CHARSET_NCR_HEX); continue; }
        const VnLegacy* v = find(c);
        if (!v) { out += c; continue; }   // chữ Pāli, ký hiệu…: giữ Unicode
        switch (cs) {
            case DK_CHARSET_TCVN3:  out += legacyChar(v->tcvn3); break;
            case DK_CHARSET_VPS:    out += legacyChar(v->vps); break;
            case DK_CHARSET_VISCII: out += legacyChar(v->viscii); break;
            case DK_CHARSET_VNI:    putBytes(out, v->vni, legacyChar); break;
            case DK_CHARSET_CP1258: putBytes(out, v->cp1258, cp1258Char); break;
            case DK_CHARSET_VIQR:   for (const char* p = v->viqr; *p; ++p) out += (char32_t)(unsigned char)*p; break;
            default:                out += c;
        }
    }
    return out;
}

} // namespace dk
