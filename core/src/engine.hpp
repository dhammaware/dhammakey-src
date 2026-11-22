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
#include <string>
#include <vector>
#include "vn.hpp"
#include "../include/dhammakey.h"

namespace dk {

struct Options {
    int method = DK_METHOD_TELEX;
    bool toneNewStyle = false;
    bool spellCheck = true;
    bool wToU = true;
    bool outputCombining = false;
    bool mDotAbove = false;
    int charset = DK_CHARSET_UNICODE;  // DK_CHARSET_* (0.3.20)
};

struct Word {
    std::vector<Letter> L;
    int tone = 0;
    bool toneSet = false;     // đã gõ phím thanh (để chữ nguyên âm sau đó không thành â/ê/ô)
    bool transformed = false; // đã có biến đổi (dấu phụ / thanh / huỷ)
    int undoAt = -1;          // vị trí phím cuối cùng gây "huỷ dấu" (ss → s)
};

struct KeyResult {
    bool consumed = false;
    bool commit = false;
    int backspaces = 0;
    std::u32string insert;
    std::u32string preedit;
};

class Engine {
public:
    Options opt;
    KeyResult processKey(char32_t ch, uint32_t flags);
    KeyResult commit();
    void reset();
    const std::u32string& display() const { return display_; }

private:
    Word base_;               // phần từ đã có sẵn (sau khi xoá lùi)
    std::u32string keys_;     // các phím gõ tiếp theo sau base_
    std::u32string display_;  // chữ đang hiện (dạng dựng sẵn)

    bool wordEmpty() const { return base_.L.empty() && keys_.empty(); }
    bool isWordKey(char32_t ch) const;
    std::u32string render() const;
    void applyKey(Word& w, char32_t k, int idx) const;
    void applyMark(Word& w, int mark, char32_t literal, int idx) const; // VNI/VIQR
    std::u32string wordToString(const Word& w) const;
    std::u32string toOutput(const std::u32string& logical) const;
    KeyResult finish(const std::u32string& oldDisplay);
    KeyResult backspace();
};

} // namespace dk
