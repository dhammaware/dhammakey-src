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

// Dữ liệu và quy tắc tiếng Việt: bảng nguyên âm có dấu, vị trí đặt dấu, kiểm tra âm tiết.
#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace dk {

enum Mod : uint8_t { M_NONE = 0, M_HAT, M_BREVE, M_HORN, M_BAR };

// Thanh điệu: 0 không dấu, 1 sắc, 2 huyền, 3 hỏi, 4 ngã, 5 nặng
struct Letter {
    char32_t base = 0;   // chữ thường a–z, hoặc ký tự bất kỳ (giữ nguyên)
    bool upper = false;
    Mod mod = M_NONE;
    bool fromW = false;  // Telex: 'ư' sinh ra từ phím w đứng riêng
};

bool isAsciiLetter(char32_t c);
char32_t asciiLower(char32_t c);
bool isVowelBase(char32_t b);                 // a e i o u y
bool isVowelModded(char32_t c);               // a ă â e ê i o ô ơ u ư y
char32_t modded(char32_t base, Mod m);        // a+HAT → â …
char32_t upperOf(char32_t c);                 // chữ hoa cho mọi ký tự tiếng Việt/Pali dùng ở đây
char32_t toned(char32_t moddedLower, int tone, bool upper);
// Tách một ký tự (có thể có dấu) về Letter + thanh. Trả false nếu không phải chữ tiếng Việt.
bool decomposeVn(char32_t c, Letter& out, int& tone);
// Dấu thanh dạng tổ hợp (U+0301 …)
char32_t combiningTone(int tone);

// s: các chữ thường đã gắn dấu phụ (không thanh). Trả vị trí đặt thanh, hoặc -1.
int tonePosition(const std::u32string& s, bool newStyle);
// s có thể là phần đầu của một âm tiết tiếng Việt hợp lệ (với thanh tone) không?
bool isValidPrefix(const std::u32string& s, int tone);

} // namespace dk
