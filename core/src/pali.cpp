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

// Pali / Sanskrit La-tinh: phím AltGr và kiểu gõ Velthuis
#include "pali.hpp"
#include "vn.hpp"
#include <dhammakey_pali_keys.h>

namespace dk {

// AltGr + phím → ký tự Pali. Bảng phím nằm ở core/include/dhammakey_pali_keys.h (nơi duy nhất để sửa).
char32_t paliAltGr(char32_t key, bool mDotAbove) {
    bool up = key >= 'A' && key <= 'Z';
    char32_t lk = asciiLower(key);
    for (int i = 0; i < DK_PALI_ALT_COUNT; ++i) {
        if ((char32_t)kDkPaliAltKeys[i].key != lk) continue;
        if (lk == 'm') { char32_t r = mDotAbove ? U'ṁ' : U'ṃ'; return up ? upperOf(r) : r; }
        const unsigned char* u = (const unsigned char*)kDkPaliAltKeys[i].lower;   // giải mã UTF-8 một ký tự
        char32_t r = u[0] < 0x80 ? u[0] : u[0] < 0xE0 ? ((u[0] & 0x1F) << 6) | (u[1] & 0x3F)
                                                      : ((u[0] & 0x0F) << 12) | ((u[1] & 0x3F) << 6) | (u[2] & 0x3F);
        return up ? upperOf(r) : r;
    }
    return 0;
}

// Velthuis: aa ā, ii ī, uu ū, .t ṭ, .d ḍ, .n ṇ, .l ḷ, .ll ḹ, .m ṃ, .r ṛ, .rr ṝ, .s ṣ, .h ḥ,
//           "n ṅ, "s ś, ~n ñ. Chữ hoa: AA/Aa → Ā, .T → Ṭ …
std::u32string velthuis(const std::u32string& in, bool mDotAbove) {
    std::u32string out;
    size_t n = in.size();
    auto withCase = [](char32_t r, char32_t src) { return (src >= 'A' && src <= 'Z') ? upperOf(r) : r; };
    for (size_t i = 0; i < n;) {
        char32_t c = in[i];
        char32_t nx = i + 1 < n ? in[i + 1] : 0;
        char32_t nl = asciiLower(nx);
        if (c == '.' && nx) {
            char32_t r = 0; size_t len = 2;
            switch (nl) {
                case 't': r = U'ṭ'; break; case 'd': r = U'ḍ'; break; case 'n': r = U'ṇ'; break;
                case 'm': r = mDotAbove ? U'ṁ' : U'ṃ'; break; case 's': r = U'ṣ'; break;
                case 'h': r = U'ḥ'; break;
                case 'r': r = U'ṛ'; if (i + 2 < n && asciiLower(in[i + 2]) == 'r') { r = U'ṝ'; len = 3; } break;
                case 'l': r = U'ḷ'; if (i + 2 < n && asciiLower(in[i + 2]) == 'l') { r = U'ḹ'; len = 3; } break;
            }
            if (r) { out += withCase(r, nx); i += len; continue; }
        } else if (c == '"' && (nl == 'n' || nl == 's')) {
            out += withCase(nl == 'n' ? U'ṅ' : U'ś', nx); i += 2; continue;
        } else if (c == '~' && nl == 'n') {
            out += withCase(U'ñ', nx); i += 2; continue;
        } else {
            char32_t cl = asciiLower(c);
            if ((cl == 'a' || cl == 'i' || cl == 'u') && nl == cl) {
                char32_t r = cl == 'a' ? U'ā' : cl == 'i' ? U'ī' : U'ū';
                out += withCase(r, c); i += 2; continue;
            }
        }
        out += c; ++i;
    }
    return out;
}

} // namespace dk
