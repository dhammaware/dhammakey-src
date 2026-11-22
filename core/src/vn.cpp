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

#include "vn.hpp"
#include <unordered_set>
#include <unordered_map>
#include <mutex>

namespace dk {

namespace {
// Thứ tự cột: không dấu, sắc, huyền, hỏi, ngã, nặng
const char32_t* ROWS_LO[12] = {
    U"aáàảãạ", U"ăắằẳẵặ", U"âấầẩẫậ", U"eéèẻẽẹ", U"êếềểễệ", U"iíìỉĩị",
    U"oóòỏõọ", U"ôốồổỗộ", U"ơớờởỡợ", U"uúùủũụ", U"ưứừửữự", U"yýỳỷỹỵ"};
const char32_t* ROWS_UP[12] = {
    U"AÁÀẢÃẠ", U"ĂẮẰẲẴẶ", U"ÂẤẦẨẪẬ", U"EÉÈẺẼẸ", U"ÊẾỀỂỄỆ", U"IÍÌỈĨỊ",
    U"OÓÒỎÕỌ", U"ÔỐỒỔỖỘ", U"ƠỚỜỞỠỢ", U"UÚÙỦŨỤ", U"ƯỨỪỬỮỰ", U"YÝỲỶỸỴ"};
const char32_t VOWELS_MODDED[] = U"aăâeêioôơuưy";

int rowOf(char32_t m) {
    for (int i = 0; i < 12; ++i)
        if (VOWELS_MODDED[i] == m) return i;
    return -1;
}

struct Decomp { Letter l; int tone; };

const std::unordered_map<char32_t, Decomp>& decompTable() {
    static std::unordered_map<char32_t, Decomp> t;
    static std::once_flag once;
    std::call_once(once, [] {
        auto baseMod = [](char32_t m, char32_t& b, Mod& md) {
            switch (m) {
                case U'ă': b = 'a'; md = M_BREVE; break;
                case U'â': b = 'a'; md = M_HAT; break;
                case U'ê': b = 'e'; md = M_HAT; break;
                case U'ô': b = 'o'; md = M_HAT; break;
                case U'ơ': b = 'o'; md = M_HORN; break;
                case U'ư': b = 'u'; md = M_HORN; break;
                default: b = m; md = M_NONE;
            }
        };
        for (int r = 0; r < 12; ++r) {
            char32_t b; Mod md;
            baseMod(VOWELS_MODDED[r], b, md);
            for (int tn = 0; tn < 6; ++tn) {
                t[ROWS_LO[r][tn]] = {Letter{b, false, md, false}, tn};
                t[ROWS_UP[r][tn]] = {Letter{b, true, md, false}, tn};
            }
        }
        t[U'đ'] = {Letter{'d', false, M_BAR, false}, 0};
        t[U'Đ'] = {Letter{'d', true, M_BAR, false}, 0};
        for (char32_t c = 'a'; c <= 'z'; ++c) {
            if (!t.count(c)) t[c] = {Letter{c, false, M_NONE, false}, 0};
            char32_t u = c - 32;
            if (!t.count(u)) t[u] = {Letter{c, true, M_NONE, false}, 0};
        }
    });
    return t;
}

// ---- Bảng âm tiết tiếng Việt (để kiểm tra chính tả) ----
struct Nucleus { const char32_t* v; const char32_t* codas; }; // codas: danh sách cách bởi '|', "" = được phép không có âm cuối

const char32_t* ONSETS[] = {U"", U"b", U"c", U"ch", U"d", U"đ", U"g", U"gh", U"gi", U"h", U"k", U"kh",
                            U"l", U"m", U"n", U"ng", U"ngh", U"nh", U"p", U"ph", U"qu", U"r", U"s",
                            U"t", U"th", U"tr", U"v", U"x"};

const Nucleus NUCLEI[] = {
    {U"a", U"|c|ch|m|n|ng|nh|p|t"}, {U"ă", U"c|m|n|ng|p|t"}, {U"â", U"c|m|n|ng|p|t"},
    {U"e", U"|c|ch|m|n|ng|nh|p|t"}, {U"ê", U"|ch|m|n|ng|nh|p|t"},
    {U"i", U"|c|ch|m|n|ng|nh|p|t"}, {U"o", U"|c|m|n|ng|p|t"}, {U"ô", U"|c|m|n|ng|p|t"},
    {U"ơ", U"|m|n|p|t"}, {U"u", U"|c|m|n|ng|p|t"}, {U"ư", U"|c|m|n|ng|t"}, {U"y", U""},
    {U"oo", U"c|ng"}, {U"oa", U"|c|ch|m|n|ng|nh|p|t"}, {U"oă", U"c|m|n|ng|t"}, {U"oe", U"|n|t"},
    {U"uâ", U"n|ng|t"}, {U"uê", U"|ch|n|nh"}, {U"uy", U"|ch|n|nh|p|t"}, {U"uơ", U""},
    {U"uô", U"c|m|n|ng|t"}, {U"ươ", U"c|m|n|ng|p|t"}, {U"iê", U"c|m|n|ng|p|t"},
    {U"yê", U"c|m|n|ng|p|t"}, {U"uyê", U"n|t"},
    {U"ua", U""}, {U"ưa", U""}, {U"ia", U""},
    {U"ai", U""}, {U"ao", U""}, {U"au", U""}, {U"ay", U""}, {U"âu", U""}, {U"ây", U""},
    {U"eo", U""}, {U"êu", U""}, {U"iu", U""}, {U"oi", U""}, {U"ôi", U""}, {U"ơi", U""},
    {U"ui", U""}, {U"ưi", U""}, {U"ưu", U""}, {U"iêu", U""}, {U"yêu", U""}, {U"oai", U""},
    {U"oay", U""}, {U"oeo", U""}, {U"uây", U""}, {U"uôi", U""}, {U"ươi", U""}, {U"ươu", U""},
    {U"uya", U""}, {U"uyu", U""},
};

struct U32Hash { size_t operator()(const std::u32string& s) const noexcept {
    size_t h = 1469598103934665603ull;
    for (char32_t c : s) { h ^= c; h *= 1099511628211ull; }
    return h;
} };

const std::unordered_set<std::u32string, U32Hash>& prefixSet() {
    static std::unordered_set<std::u32string, U32Hash> set;
    static std::once_flag once;
    std::call_once(once, [] {
        for (const char32_t* on : ONSETS) {
            std::u32string onset(on);
            for (const Nucleus& n : NUCLEI) {
                std::u32string nuc(n.v);
                if (onset == U"qu" && nuc[0] == U'u') continue;
                if (onset == U"gi" && nuc[0] == U'i') continue;
                std::u32string codas(n.codas);
                size_t start = 0;
                while (true) {
                    size_t bar = codas.find(U'|', start);
                    std::u32string coda = codas.substr(start, bar == std::u32string::npos ? std::u32string::npos : bar - start);
                    std::u32string syl = onset + nuc + coda;
                    for (size_t k = 1; k <= syl.size(); ++k) set.insert(syl.substr(0, k));
                    if (bar == std::u32string::npos) break;
                    start = bar + 1;
                }
            }
        }
    });
    return set;
}

size_t onsetLength(const std::u32string& s) {
    auto at = [&](size_t i) -> char32_t { return i < s.size() ? s[i] : 0; };
    auto starts = [&](const char32_t* p) {
        size_t i = 0;
        for (; p[i]; ++i) if (at(i) != p[i]) return false;
        return true;
    };
    if (starts(U"ngh")) return 3;
    for (const char32_t* p : {U"ng", U"nh", U"ch", U"gh", U"kh", U"ph", U"th", U"tr"})
        if (starts(p)) return 2;
    if ((starts(U"gi") || starts(U"qu")) && s.size() > 2 && isVowelModded(s[2])) return 2;
    if (!s.empty() && !isVowelModded(s[0])) return 1;
    return 0;
}
} // namespace

bool isAsciiLetter(char32_t c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
char32_t asciiLower(char32_t c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }
bool isVowelBase(char32_t b) { return b == 'a' || b == 'e' || b == 'i' || b == 'o' || b == 'u' || b == 'y'; }
bool isVowelModded(char32_t c) { return rowOf(c) >= 0; }

char32_t modded(char32_t b, Mod m) {
    switch (m) {
        case M_HAT:   return b == 'a' ? U'â' : b == 'e' ? U'ê' : b == 'o' ? U'ô' : b;
        case M_BREVE: return b == 'a' ? U'ă' : b;
        case M_HORN:  return b == 'o' ? U'ơ' : b == 'u' ? U'ư' : b;
        case M_BAR:   return b == 'd' ? U'đ' : b;
        default:      return b;
    }
}

char32_t upperOf(char32_t c) {
    if (c >= 'a' && c <= 'z') return c - 32;
    int r = rowOf(c);
    if (r >= 0) return ROWS_UP[r][0];
    switch (c) {
        case U'đ': return U'Đ';
        // Pali / Sanskrit
        case U'ā': return U'Ā'; case U'ī': return U'Ī'; case U'ū': return U'Ū';
        case U'ṃ': return U'Ṃ'; case U'ṁ': return U'Ṁ'; case U'ṅ': return U'Ṅ';
        case U'ñ': return U'Ñ'; case U'ṭ': return U'Ṭ'; case U'ḍ': return U'Ḍ';
        case U'ṇ': return U'Ṇ'; case U'ḷ': return U'Ḷ'; case U'ḹ': return U'Ḹ';
        case U'ś': return U'Ś'; case U'ṣ': return U'Ṣ'; case U'ṛ': return U'Ṛ';
        case U'ṝ': return U'Ṝ'; case U'ḥ': return U'Ḥ';
    }
    return c;
}

char32_t toned(char32_t m, int tone, bool upper) {
    int r = rowOf(m);
    if (r < 0) return upper ? upperOf(m) : m;
    return (upper ? ROWS_UP : ROWS_LO)[r][tone];
}

bool decomposeVn(char32_t c, Letter& out, int& tone) {
    const auto& t = decompTable();
    auto it = t.find(c);
    if (it == t.end()) return false;
    out = it->second.l;
    tone = it->second.tone;
    return true;
}

char32_t combiningTone(int tone) {
    switch (tone) {
        case 1: return 0x0301; case 2: return 0x0300; case 3: return 0x0309;
        case 4: return 0x0303; case 5: return 0x0323;
    }
    return 0;
}

int tonePosition(const std::u32string& s, bool newStyle) {
    size_t on = onsetLength(s);
    size_t e = on;
    while (e < s.size() && isVowelModded(s[e])) ++e;
    size_t n = e - on;
    if (n == 0) return -1;
    bool hasCoda = e < s.size();
    std::u32string nuc = s.substr(on, n);
    // 1) Có nguyên âm mang dấu phụ (ă â ê ô ơ ư) → đặt vào nguyên âm đó (ươ → ơ)
    for (int i = (int)n - 1; i >= 0; --i) {
        char32_t c = nuc[i];
        if (c == U'ă' || c == U'â' || c == U'ê' || c == U'ô' || c == U'ơ' || c == U'ư')
            return (int)on + i;
    }
    if (n == 1) return (int)on;
    // 2) Có âm cuối → nguyên âm cuối của vần
    if (hasCoda) return (int)(on + n - 1);
    // 3) Vần mở
    if (n == 3) return (int)on + 1;
    if (nuc == U"oa" || nuc == U"oe" || nuc == U"uy") return (int)on + (newStyle ? 1 : 0);
    return (int)on;
}

bool isValidPrefix(const std::u32string& s, int tone) {
    if (s.empty()) return true;
    if (!prefixSet().count(s)) return false;
    // Âm cuối tắc (c, ch, p, t) chỉ đi với sắc hoặc nặng
    if (tone == 2 || tone == 3 || tone == 4) {
        size_t i = s.size();
        while (i > 0 && !isVowelModded(s[i - 1])) --i;
        if (i > 0) {
            std::u32string coda = s.substr(i);
            if (coda == U"c" || coda == U"ch" || coda == U"p" || coda == U"t") return false;
        }
    }
    return true;
}

} // namespace dk
