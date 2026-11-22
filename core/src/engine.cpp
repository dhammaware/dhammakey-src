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

#include "engine.hpp"
#include "charset.hpp"
#include "pali.hpp"

namespace dk {

namespace {

bool hasVowel(const Word& w) {
    for (const Letter& l : w.L)
        if (isVowelBase(l.base)) return true;
    return false;
}

// Tìm cụm nguyên âm cuối [s, e), cho phép tối đa 2 phụ âm cuối phía sau.
bool lastCluster(const Word& w, int& s, int& e) {
    int i = (int)w.L.size() - 1, cons = 0;
    while (i >= 0 && !isVowelBase(w.L[i].base)) {
        if (++cons > 2) return false;
        --i;
    }
    if (i < 0) return false;
    e = i + 1;
    while (i >= 0 && isVowelBase(w.L[i].base)) --i;
    s = i + 1;
    // "qu", "gi": u / i thuộc phụ âm đầu
    if (s == 1 && e - s >= 2) {
        char32_t b0 = w.L[0].base, b1 = w.L[1].base;
        if ((b0 == 'q' && b1 == 'u') || (b0 == 'g' && b1 == 'i')) s = 2;
    }
    return true;
}

bool validWord(const Word& w) {
    std::u32string low;
    for (const Letter& l : w.L) low += modded(l.base, l.mod);
    return isValidPrefix(low, w.tone);
}

Letter letterOf(char32_t k) {
    Letter l;
    if (isAsciiLetter(k)) { l.base = asciiLower(k); l.upper = (k >= 'A' && k <= 'Z'); }
    else l.base = k;
    return l;
}

// Bật/tắt dấu phụ tại idx; bấm lần hai thì huỷ dấu và thêm phím gốc như chữ thường.
void toggleMod(Word& w, int idx, Mod m, char32_t literal, int keyIdx) {
    Letter& l = w.L[idx];
    if (l.mod == m) {
        l.mod = M_NONE;
        w.L.push_back(letterOf(literal));
        w.undoAt = keyIdx;
    } else {
        l.mod = m;
    }
    w.transformed = true;
}

int telexTone(char32_t lo) {
    switch (lo) { case 's': return 1; case 'f': return 2; case 'r': return 3; case 'x': return 4; case 'j': return 5; }
    return 0;
}

int viqrMark(char32_t c) {
    switch (c) {
        case '\'': return 1; case '`': return 2; case '?': return 3; case '~': return 4; case '.': return 5;
        case '^': return 6; case '+': return 7; case '(': return 8;
    }
    return -1;
}

// Phím w (Telex) / 7 (VNI) / + (VIQR): ơ ư ươ, và ă cho Telex
// Trả 0: không áp dụng được, 1: đã áp dụng/huỷ
int applyHorn(Word& w, char32_t literal, int keyIdx, bool allowBreve) {
    int s, e;
    if (!lastCluster(w, s, e)) return 0;
    // "uo" → "ươ"
    for (int j = s; j + 1 < e; ++j) {
        if (w.L[j].base == 'u' && w.L[j + 1].base == 'o') {
            bool quo = j == 1 && w.L[0].base == 'q';
            // "uo" là cả vần, không âm cuối → "uơ" (thuở, huơ); có âm cuối sẽ tự thành "ươ"
            if (quo || (e - s == 2 && e == (int)w.L.size() && w.L[j].mod == M_NONE)) {
                toggleMod(w, j + 1, M_HORN, literal, keyIdx);
                return 1;
            }
            if (w.L[j].mod == M_HORN && w.L[j + 1].mod == M_HORN) {
                w.L[j].mod = w.L[j + 1].mod = M_NONE;
                w.L.push_back(letterOf(literal));
                w.undoAt = keyIdx;
            } else {
                w.L[j].mod = w.L[j + 1].mod = M_HORN;
            }
            w.transformed = true;
            return 1;
        }
    }
    int lastO = -1, lastA = -1, firstU = -1;
    for (int j = s; j < e; ++j) {
        char32_t b = w.L[j].base;
        if (b == 'o') lastO = j;
        if (b == 'a') lastA = j;
        if (b == 'u' && firstU < 0) firstU = j;
    }
    bool noCoda = e == (int)w.L.size();
    if (lastO >= 0 && !(allowBreve && lastA > lastO)) { toggleMod(w, lastO, M_HORN, literal, keyIdx); return 1; }
    if (lastA >= 0 && allowBreve) {
        // "ua" (không âm cuối) + w → "ưa"
        if (firstU >= 0 && firstU + 1 == lastA && noCoda && w.L[lastA].mod == M_NONE) {
            toggleMod(w, firstU, M_HORN, literal, keyIdx); return 1;
        }
        toggleMod(w, lastA, M_BREVE, literal, keyIdx); return 1;
    }
    if (firstU >= 0) { toggleMod(w, firstU, M_HORN, literal, keyIdx); return 1; }
    return 0;
}

} // namespace

bool Engine::isWordKey(char32_t ch) const {
    if (isAsciiLetter(ch)) return true;
    switch (opt.method) {
        case DK_METHOD_VNI:  return ch >= '0' && ch <= '9' && !wordEmpty();
        case DK_METHOD_VIQR: return viqrMark(ch) >= 0 && !wordEmpty();
        case DK_METHOD_PALI_VELTHUIS:
            // Dấu chấm cũng mở đầu được chữ (.t → ṭ, .thaana → ṭhāna) — 0.3.30. Chấm đứng riêng vẫn ra chấm.
            return ch == '"' || ch == '~' || ch == '.';
    }
    return false;
}

void Engine::applyMark(Word& w, int mark, char32_t literal, int idx) const {
    auto pushLit = [&] { w.L.push_back(letterOf(literal)); };
    if (mark >= 1 && mark <= 5) {
        if (!hasVowel(w)) { pushLit(); return; }
        if (w.tone == mark) { w.tone = 0; pushLit(); w.undoAt = idx; }
        else { w.tone = mark; w.toneSet = true; }
        w.transformed = true;
        return;
    }
    if (mark == 0) {
        if (w.tone) { w.tone = 0; w.transformed = true; } else pushLit();
        return;
    }
    if (mark == 9) {
        if (!w.L.empty() && w.L[0].base == 'd') toggleMod(w, 0, M_BAR, literal, idx);
        else pushLit();
        return;
    }
    if (mark == 7) { if (!applyHorn(w, literal, idx, false)) pushLit(); return; }
    int s, e;
    if (!lastCluster(w, s, e)) { pushLit(); return; }
    int target = -1;
    for (int j = e - 1; j >= s && target < 0; --j) {
        char32_t b = w.L[j].base;
        if (mark == 6 && (b == 'a' || b == 'e' || b == 'o')) target = j;
        if (mark == 8 && b == 'a') target = j;
    }
    if (target < 0) { pushLit(); return; }
    toggleMod(w, target, mark == 6 ? M_HAT : M_BREVE, literal, idx);
}

void Engine::applyKey(Word& w, char32_t k, int idx) const {
    if (opt.method == DK_METHOD_VNI && k >= '0' && k <= '9') { applyMark(w, (int)(k - '0'), k, idx); return; }
    if (opt.method == DK_METHOD_VIQR) {
        int m = viqrMark(k);
        if (m >= 0) { applyMark(w, m, k, idx); return; }
    }
    if (!isAsciiLetter(k)) { w.L.push_back(letterOf(k)); return; }

    char32_t lo = asciiLower(k);

    // đ: "dd" (Telex, VIQR) — chỉ khi chữ đầu từ là d
    if (lo == 'd' && opt.method != DK_METHOD_VNI && !w.L.empty() && w.L[0].base == 'd') {
        toggleMod(w, 0, M_BAR, k, idx);
        return;
    }

    if (opt.method == DK_METHOD_TELEX) {
        int t = telexTone(lo);
        if (t && hasVowel(w)) { applyMark(w, t, k, idx); return; }
        if (lo == 'z' && w.tone) { w.tone = 0; w.transformed = true; return; }
        if ((lo == 'a' || lo == 'e' || lo == 'o') && !w.toneSet) {
            int s, e;
            if (lastCluster(w, s, e)) {
                for (int j = e - 1; j >= s; --j) {
                    if (w.L[j].base != lo) continue;
                    // Thử thêm dấu mũ; nếu ra âm tiết sai mà gõ thường lại đúng (ngoeo → ngoèo) thì gõ thường
                    Word t = w;
                    toggleMod(t, j, M_HAT, k, idx);
                    Word plain = w;
                    plain.L.push_back(letterOf(k));
                    if (!validWord(t) && validWord(plain)) break;
                    w = t;
                    return;
                }
            }
        }
        if (lo == 'w') {
            if (!w.L.empty() && w.L.back().fromW) {
                // "ww" → "w"
                w.L.back() = letterOf(k);
                w.undoAt = idx;
                w.transformed = true;
                return;
            }
            if (applyHorn(w, k, idx, true)) return;
            if (opt.wToU) {
                Letter l = letterOf(k);
                l.base = 'u'; l.mod = M_HORN; l.fromW = true;
                w.L.push_back(l);
                w.transformed = true;
                return;
            }
        }
    }
    w.L.push_back(letterOf(k));
}

std::u32string Engine::wordToString(const Word& w) const {
    std::u32string low;
    low.reserve(w.L.size());
    for (const Letter& l : w.L) low += modded(l.base, l.mod);
    int pos = w.tone ? tonePosition(low, opt.toneNewStyle) : -1;
    std::u32string out;
    out.reserve(w.L.size());
    for (size_t i = 0; i < w.L.size(); ++i) {
        const Letter& l = w.L[i];
        char32_t c = low[i];
        if ((int)i == pos) c = toned(c, w.tone, l.upper);
        else if (l.upper) c = upperOf(c);
        out += c;
    }
    return out;
}

std::u32string Engine::render() const {
    if (opt.method == DK_METHOD_PALI_VELTHUIS)
        return wordToString(base_) + velthuis(keys_, opt.mDotAbove);

    Word w = base_;
    w.undoAt = -1;
    for (size_t i = 0; i < keys_.size(); ++i) applyKey(w, keys_[i], (int)i);
    // "uơ" + âm cuối / i / u → "ươ" (tuơng → tương, nguơi → ngươi)
    for (size_t j = 0; j + 2 < w.L.size(); ++j) {
        if (w.L[j].base == 'u' && w.L[j].mod == M_NONE && w.L[j + 1].base == 'o' &&
            w.L[j + 1].mod == M_HORN && !(j == 1 && w.L[0].base == 'q'))
            w.L[j].mod = M_HORN;
    }
    std::u32string r = wordToString(w);
    if (opt.spellCheck && w.transformed) {
        std::u32string low;
        for (const Letter& l : w.L) low += modded(l.base, l.mod);
        if (!isValidPrefix(low, w.tone)) {
            // Phím cuối là phím huỷ dấu (ss, ff, aaa…) → giữ kết quả đã huỷ ("thiss" → "this")
            if (w.undoAt == (int)keys_.size() - 1) return r;
            // Ngược lại: không phải tiếng Việt → trả nguyên các phím đã gõ (dhamma, majjhima…)
            return wordToString(base_) + keys_;
        }
    }
    return r;
}

std::u32string Engine::toOutput(const std::u32string& logical) const {
    // Bảng mã cũ (0.3.20): đổi từ chữ dựng sẵn; xoá lùi/so khác biệt vẫn tính trên chuỗi đã đổi
    if (opt.charset > DK_CHARSET_UNICODE_COMBINING) return toCharset(logical, opt.charset);
    if (!opt.outputCombining && opt.charset != DK_CHARSET_UNICODE_COMBINING) return logical;
    std::u32string out;
    for (char32_t c : logical) {
        Letter l; int t;
        if (decomposeVn(c, l, t) && t > 0) {
            char32_t m = modded(l.base, l.mod);
            out += l.upper ? upperOf(m) : m;
            out += combiningTone(t);
        } else {
            out += c;
        }
    }
    return out;
}

KeyResult Engine::finish(const std::u32string& oldDisplay) {
    KeyResult r;
    r.consumed = true;
    std::u32string a = toOutput(oldDisplay), b = toOutput(display_);
    size_t cp = 0;
    while (cp < a.size() && cp < b.size() && a[cp] == b[cp]) ++cp;
    r.backspaces = (int)(a.size() - cp);
    r.insert = b.substr(cp);
    r.preedit = b;
    return r;
}

KeyResult Engine::backspace() {
    if (wordEmpty()) return {};
    std::u32string old = display_;
    std::u32string t = display_;
    if (!t.empty()) t.pop_back();
    base_ = Word{};
    keys_.clear();
    for (char32_t c : t) {
        Letter l; int tone = 0;
        if (opt.method != DK_METHOD_PALI_VELTHUIS && decomposeVn(c, l, tone)) {
            if (tone) { base_.tone = tone; base_.toneSet = true; }
            if (tone || l.mod) base_.transformed = true;
            base_.L.push_back(l);
        } else {
            base_.L.push_back(letterOf(c));
        }
    }
    if (render() != t) {
        // Không dựng lại được đúng như cũ → giữ nguyên chữ hiện tại
        base_ = Word{};
        for (char32_t c : t) { Letter l; l.base = c; base_.L.push_back(l); }
    }
    display_ = t;
    return finish(old);
}

KeyResult Engine::processKey(char32_t ch, uint32_t flags) {
    if (ch == DK_KEY_BACKSPACE) return backspace();
    if (opt.method == DK_METHOD_OFF) return {};

    char32_t k = ch;
    if (flags & DK_FLAG_ALTGR) {
        k = paliAltGr(ch, opt.mDotAbove);
        if (!k) return commit();
    } else if (!isWordKey(ch)) {
        return commit();
    }
    std::u32string old = display_;
    keys_.push_back(k);
    display_ = render();
    return finish(old);
}

KeyResult Engine::commit() {
    KeyResult r;
    if (!wordEmpty()) {
        r.commit = true;
        r.preedit = toOutput(display_);
    }
    reset();
    return r;
}

void Engine::reset() {
    base_ = Word{};
    keys_.clear();
    display_.clear();
}

} // namespace dk
