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

// Kiểm thử lõi DhammaKey: mô phỏng gõ phím qua C API theo cả hai kiểu lớp vỏ
// (hook như Unikey và composition như TSF) — hai kiểu phải cho cùng kết quả.
//
// Ký hiệu trong chuỗi gõ:  {x} = AltGr + x     < = Backspace
#include "../include/dhammakey.h"
#include <cstdio>
#include <string>
#include <vector>

static std::u32string fromUtf8(const std::string& s) {
    std::u32string o;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = s[i];
        char32_t cp; int n;
        if (c < 0x80) { cp = c; n = 1; }
        else if ((c >> 5) == 6) { cp = c & 0x1F; n = 2; }
        else if ((c >> 4) == 14) { cp = c & 0x0F; n = 3; }
        else { cp = c & 0x07; n = 4; }
        for (int k = 1; k < n; ++k) cp = (cp << 6) | (s[i + k] & 0x3F);
        o += cp; i += n;
    }
    return o;
}
static std::string toUtf8(const std::u32string& s) {
    std::string o;
    for (char32_t c : s) {
        if (c < 0x80) o += (char)c;
        else if (c < 0x800) { o += (char)(0xC0 | (c >> 6)); o += (char)(0x80 | (c & 0x3F)); }
        else if (c < 0x10000) { o += (char)(0xE0 | (c >> 12)); o += (char)(0x80 | ((c >> 6) & 0x3F)); o += (char)(0x80 | (c & 0x3F)); }
        else { o += (char)(0xF0 | (c >> 18)); o += (char)(0x80 | ((c >> 12) & 0x3F)); o += (char)(0x80 | ((c >> 6) & 0x3F)); o += (char)(0x80 | (c & 0x3F)); }
    }
    return o;
}

struct Key { char32_t ch; uint32_t flags; };
static std::vector<Key> parseKeys(const std::string& s) {
    std::u32string u = fromUtf8(s);
    std::vector<Key> keys;
    for (size_t i = 0; i < u.size(); ++i) {
        if (u[i] == '{' && i + 2 < u.size() && u[i + 2] == '}') { keys.push_back({u[i + 1], DK_FLAG_ALTGR}); i += 2; }
        else if (u[i] == '<') keys.push_back({DK_KEY_BACKSPACE, 0});
        else keys.push_back({u[i], 0});
    }
    return keys;
}

// Kiểu hook: màn hình = buffer; lõi trả số phím xoá + chuỗi chèn
static std::string typeHook(dk_engine* e, const std::vector<Key>& keys) {
    std::u32string buf;
    dk_result r;
    for (const Key& k : keys) {
        dk_process_key(e, k.ch, k.flags, &r);
        if (r.consumed) {
            for (int i = 0; i < r.backspaces && !buf.empty(); ++i) buf.pop_back();
            buf += fromUtf8(r.insert);
        } else if (k.ch == DK_KEY_BACKSPACE) {
            if (!buf.empty()) buf.pop_back();
        } else {
            buf += k.ch;
        }
    }
    dk_commit(e, &r);
    return toUtf8(buf);
}

// Kiểu composition (TSF/IMKit/IBus): văn bản đã chốt + chuỗi đang soạn
static std::string typeCompose(dk_engine* e, const std::vector<Key>& keys) {
    std::u32string committed, pre;
    dk_result r;
    for (const Key& k : keys) {
        dk_process_key(e, k.ch, k.flags, &r);
        if (r.consumed) { pre = fromUtf8(r.preedit); continue; }
        if (r.commit) { committed += fromUtf8(r.preedit); pre.clear(); }
        if (k.ch == DK_KEY_BACKSPACE) { if (!committed.empty()) committed.pop_back(); }
        else committed += k.ch;
    }
    dk_commit(e, &r);
    if (r.commit) committed += fromUtf8(r.preedit);
    return toUtf8(committed);
}

static int g_fail = 0, g_total = 0;

static void check(int method, const std::string& keys, const std::string& expect,
                  std::initializer_list<std::pair<int, int>> opts = {}) {
    for (int mode = 0; mode < 2; ++mode) {
        dk_engine* e = dk_engine_new();
        dk_set_option(e, DK_OPT_METHOD, method);
        for (auto& o : opts) dk_set_option(e, o.first, o.second);
        auto k = parseKeys(keys);
        std::string got = mode == 0 ? typeHook(e, k) : typeCompose(e, k);
        ++g_total;
        if (got != expect) {
            ++g_fail;
            std::printf("  SAI [%s] gõ \"%s\" → \"%s\" (cần \"%s\") len %zu/%zu\n", mode == 0 ? "hook" : "compose",
                        keys.c_str(), got.c_str(), expect.c_str(), got.size(), expect.size());
        }
        dk_engine_free(e);
    }
}

int main() {
    const int T = DK_METHOD_TELEX, V = DK_METHOD_VNI, Q = DK_METHOD_VIQR, P = DK_METHOD_PALI_VELTHUIS;

    std::printf("== Telex ==\n");
    check(T, "tieengs vieetj", "tiếng việt");
    check(T, "Vieetj Nam", "Việt Nam");
    check(T, "VIEETJ NAM", "VIỆT NAM");
    check(T, "nguwowif", "người");
    check(T, "truowngf", "trường");
    check(T, "truongwf", "trường");
    check(T, "dduwowngf", "đường");
    check(T, "DDUWOWNGF", "ĐƯỜNG");
    check(T, "ddi", "đi");
    check(T, "ddd", "dd");
    check(T, "hoaf", "hòa");
    check(T, "hoaf", "hoà", {{DK_OPT_TONE_NEW_STYLE, 1}});
    check(T, "thuyr", "thủy");
    check(T, "thuyr", "thuỷ", {{DK_OPT_TONE_NEW_STYLE, 1}});
    check(T, "quas", "quá");
    check(T, "gias", "giá");
    check(T, "gif", "gì");
    check(T, "giuwax", "giữa");
    check(T, "ngoaif", "ngoài");
    check(T, "muaf", "mùa");
    check(T, "muwa", "mưa");
    check(T, "muaw", "mưa");
    check(T, "hoawcj", "hoặc");
    check(T, "khuyeens", "khuyến");
    check(T, "tuyeetj", "tuyệt");
    check(T, "xooong", "xoong");
    check(T, "ngoeof", "ngoèo");
    check(T, "aaa", "aa");
    check(T, "w", "ư");
    check(T, "ww", "w");
    check(T, "tw", "tư");
    check(T, "thiss", "this");
    check(T, "tieengsz", "tiêng");
    check(T, "chafo", "chào");     // gõ dấu trước
    check(T, "phaatj phaps", "phật pháp");
    check(T, "Phaatj giaso", "Phật giáo");
    check(T, "tieengs<", "tiến");  // xoá lùi giữ dấu
    check(T, "tieengs<<g", "tiếg");
    check(T, "tieens<ng", "tiếng");
    check(T, "thuowr", "thuở");
    check(T, "tuowng", "tương");
    check(T, "nguoiwf", "người");

    std::printf("== Telex + Pali không xung đột ==\n");
    check(T, "buddha", "buddha");
    check(T, "dhamma", "dhamma");
    check(T, "majjhima", "majjhima");
    check(T, "vipassana", "vipassana");
    check(T, "dasa", "dasa");
    check(T, "sutta", "sutta");
    check(T, "sangha", "sangha");
    check(T, "bhante", "bhante");
    check(T, "missing", "missing");
    check(T, "were", "were");
    check(T, "nibb{a}na", "nibbāna");
    check(T, "sa{k}gha", "saṅgha");
    check(T, "pu{n}{n}a", "puṇṇa");
    check(T, "pu{j}{j}a", "puñña");
    check(T, "satipa{t}{t}h{a}na", "satipaṭṭhāna");
    check(T, "{A}nanda", "Ānanda");
    check(T, "eva{m}", "evaṃ");
    check(T, "eva{m}", "evaṁ", {{DK_OPT_PALI_M_DOT_ABOVE, 1}});
    check(T, "{j}{a}{n}a", "ñāṇa");
    check(T, "Namo tassa bhagavato arahato sammasambuddhassa",
             "Namo tassa bhagavato arahato sammasambuddhassa");
    check(T, "Namo tassa bhagavato arahato samm{a}sambuddhassa",
             "Namo tassa bhagavato arahato sammāsambuddhassa");
    check(T, "Buddhaf ddoongf", "Buddhaf đồng"); // Pali giữ nguyên, chữ Việt vẫn gõ được

    std::printf("== Unicode tổ hợp ==\n");
    check(T, "as", "a\u0301", {{DK_OPT_OUTPUT_COMBINING, 1}});
    check(T, "vieetj", "vi\u00EA\u0323t", {{DK_OPT_OUTPUT_COMBINING, 1}});

    std::printf("== VNI ==\n");
    check(V, "tie61ng vie65t", "tiếng việt");
    check(V, "d9uo7ng2", "đường");
    check(V, "hoa2", "hòa");
    check(V, "a8n", "ăn");
    check(V, "2024", "2024");
    check(V, "nam 2024", "nam 2024");
    check(V, "a11", "a1");
    check(V, "buddha", "buddha");

    std::printf("== VIQR ==\n");
    check(Q, "vie^.t", "việt");
    check(Q, "ddu+o+`ng", "đường");
    check(Q, "a('n", "ắn");

    std::printf("== Pali Velthuis ==\n");
    check(P, "nibbaana", "nibbāna");
    check(P, "sa\"ngha", "saṅgha");
    check(P, "eva.m", "evaṃ");
    check(P, "~naa.na", "ñāṇa");
    check(P, "pu.n.na", "puṇṇa");
    check(P, "Aananda", "Ānanda");
    check(P, "AAnanda", "Ānanda");
    check(P, "satipa.t.thaana", "satipaṭṭhāna");
    check(P, "dhamma. buddha", "dhamma. buddha");
    // 0.3.30: dấu chấm mở đầu chữ (đã thử trên máy thật 0.3.29: ".t" đầu từ không ra ṭ)
    check(P, ".t", "ṭ");
    check(P, ".thaana", "ṭhāna");
    check(P, ".Thaana .daaha", "Ṭhāna ḍāha");
    check(P, "a . b", "a . b");
    check(P, "... .", "... .");
    check(P, "3.5 .n", "3.5 ṇ");
    check(P, "k.r.s.na", "kṛṣṇa");
    check(P, "\"siila", "śīla");
    check(P, "kaaya<<ya", "kāya");


    std::printf("== Bảng mã cũ (0.3.20) ==\n");
    // Mã mong đợi lấy độc lập: TCVN3, VISCII theo glibc iconv; VNI theo chữ "Vieät/ñöôøng" quen thấy;
    // byte hiện thành ký tự Windows-1252 (trừ 80 8E 9E B7 — xem charset.cpp).
    check(T, "vieejt", "viÖt", {{DK_OPT_CHARSET, DK_CHARSET_TCVN3}});
    check(T, "ddwowfng", "®\u00ADêng", {{DK_OPT_CHARSET, DK_CHARSET_TCVN3}});
    check(T, "tieesng vieejt", "tiÕng viÖt", {{DK_OPT_CHARSET, DK_CHARSET_TCVN3}});
    check(T, "vieejt", "vi®t", {{DK_OPT_CHARSET, DK_CHARSET_VISCII}});
    check(T, "ddwowfng", "ðß¶ng", {{DK_OPT_CHARSET, DK_CHARSET_VISCII}});
    check(T, "Vieejt Nam", "Vieät Nam", {{DK_OPT_CHARSET, DK_CHARSET_VNI}});
    check(T, "ddwowfng", "ñöôøng", {{DK_OPT_CHARSET, DK_CHARSET_VNI}});
    check(T, "vieejt", "vi\u00EA\u0323t", {{DK_OPT_CHARSET, DK_CHARSET_CP1258}});   // 1258: ê + dấu nặng tổ hợp
    check(T, "ddax", "®\u00B7", {{DK_OPT_CHARSET, DK_CHARSET_TCVN3}});   // ã = byte B7 → U+00B7 (.VnTime và .VnArialH đều nhận; đã thử trên máy thật Word 15/11)
    check(T, "Af", "\u0080", {{DK_OPT_CHARSET, DK_CHARSET_VPS}});       // VPS À = byte 80 → U+0080 (không phải €)
    check(T, "vieejt", "vie^.t", {{DK_OPT_CHARSET, DK_CHARSET_VIQR}});
    check(T, "ddwowfng", "ddu+o+`ng", {{DK_OPT_CHARSET, DK_CHARSET_VIQR}});
    check(T, "vieejt", "vi&#7879;t", {{DK_OPT_CHARSET, DK_CHARSET_NCR_DEC}});
    check(T, "vieejt", "vi&#x1EC7;t", {{DK_OPT_CHARSET, DK_CHARSET_NCR_HEX}});
    check(T, "vieejt", "vi\u00EA\u0323t", {{DK_OPT_CHARSET, DK_CHARSET_UNICODE_COMBINING}});
    check(T, "vieejt<<", "vi", {{DK_OPT_CHARSET, DK_CHARSET_VNI}});
    check(T, "buddha", "buddha", {{DK_OPT_CHARSET, DK_CHARSET_TCVN3}});
    check(T, "{a}nanda", "ānanda", {{DK_OPT_CHARSET, DK_CHARSET_TCVN3}});   // Pāli: giữ Unicode
    check(P, "nibbaana", "nibbāna", {{DK_OPT_CHARSET, DK_CHARSET_VNI}});

    std::printf("\nKết quả: %d/%d đạt\n", g_total - g_fail, g_total);
    return g_fail ? 1 : 0;
}
