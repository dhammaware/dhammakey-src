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

// Gõ thử lõi DhammaKey trên dòng lệnh.
//   dkdemo [telex|vni|viqr|pali] "chuỗi phím"
// Ký hiệu: {x} = AltGr + x, < = Backspace
#include "dhammakey.h"
#include <cstdio>
#include <cstring>
#include <string>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("Cách dùng: dkdemo telex|vni|viqr|pali \"chuoi phim\"\n");
        return 1;
    }
    int m = DK_METHOD_TELEX;
    if (!std::strcmp(argv[1], "vni")) m = DK_METHOD_VNI;
    else if (!std::strcmp(argv[1], "viqr")) m = DK_METHOD_VIQR;
    else if (!std::strcmp(argv[1], "pali")) m = DK_METHOD_PALI_VELTHUIS;

    dk_engine* e = dk_engine_new();
    dk_set_option(e, DK_OPT_METHOD, m);
    std::string out, in = argv[2];
    dk_result r;
    for (size_t i = 0; i < in.size(); ++i) {
        uint32_t ch = (unsigned char)in[i], fl = 0;
        if (ch == '{' && i + 2 < in.size() && in[i + 2] == '}') { ch = (unsigned char)in[i + 1]; fl = DK_FLAG_ALTGR; i += 2; }
        else if (ch == '<') ch = DK_KEY_BACKSPACE;
        dk_process_key(e, ch, fl, &r);
        if (!r.consumed) {
            if (r.commit) out += r.preedit;
            if (ch == DK_KEY_BACKSPACE) { if (!out.empty()) out.pop_back(); }
            else out += (char)ch;
        }
    }
    if (dk_commit(e, &r)) out += r.preedit;
    std::printf("%s\n", out.c_str());
    dk_engine_free(e);
    return 0;
}
