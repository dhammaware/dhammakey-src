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

#include "../include/dhammakey.h"
#include "engine.hpp"
#include <string>

struct dk_engine {
    dk::Engine eng;
    std::string insert, preedit;
};

namespace {
std::string utf8(const std::u32string& s) {
    std::string o;
    for (char32_t c : s) {
        if (c < 0x80) o += (char)c;
        else if (c < 0x800) { o += (char)(0xC0 | (c >> 6)); o += (char)(0x80 | (c & 0x3F)); }
        else if (c < 0x10000) { o += (char)(0xE0 | (c >> 12)); o += (char)(0x80 | ((c >> 6) & 0x3F)); o += (char)(0x80 | (c & 0x3F)); }
        else { o += (char)(0xF0 | (c >> 18)); o += (char)(0x80 | ((c >> 12) & 0x3F)); o += (char)(0x80 | ((c >> 6) & 0x3F)); o += (char)(0x80 | (c & 0x3F)); }
    }
    return o;
}

void fill(dk_engine* e, const dk::KeyResult& r, dk_result* out) {
    e->insert = utf8(r.insert);
    e->preedit = utf8(r.preedit);
    if (!out) return;
    out->consumed = r.consumed;
    out->commit = r.commit;
    out->backspaces = r.backspaces;
    out->insert = e->insert.c_str();
    out->preedit = e->preedit.c_str();
}
} // namespace

extern "C" {

dk_engine* dk_engine_new(void) { return new dk_engine(); }
void dk_engine_free(dk_engine* e) { delete e; }

int dk_set_option(dk_engine* e, int option, int value) {
    auto& o = e->eng.opt;
    switch (option) {
        case DK_OPT_METHOD:
            if (value < DK_METHOD_OFF || value > DK_METHOD_PALI_VELTHUIS) return 0;
            e->eng.reset();
            o.method = value; return 1;
        case DK_OPT_TONE_NEW_STYLE: o.toneNewStyle = value != 0; return 1;
        case DK_OPT_SPELL_CHECK: o.spellCheck = value != 0; return 1;
        case DK_OPT_W_TO_U: o.wToU = value != 0; return 1;
        case DK_OPT_OUTPUT_COMBINING: o.outputCombining = value != 0; return 1;
        case DK_OPT_PALI_M_DOT_ABOVE: o.mDotAbove = value != 0; return 1;
        case DK_OPT_CHARSET:
            if (value < 0 || value >= DK_CHARSET_COUNT) return 0;
            o.charset = value;
            o.outputCombining = value == DK_CHARSET_UNICODE_COMBINING;
            return 1;
    }
    return 0;
}

int dk_get_option(dk_engine* e, int option) {
    const auto& o = e->eng.opt;
    switch (option) {
        case DK_OPT_METHOD: return o.method;
        case DK_OPT_TONE_NEW_STYLE: return o.toneNewStyle;
        case DK_OPT_SPELL_CHECK: return o.spellCheck;
        case DK_OPT_W_TO_U: return o.wToU;
        case DK_OPT_OUTPUT_COMBINING: return o.outputCombining;
        case DK_OPT_PALI_M_DOT_ABOVE: return o.mDotAbove;
        case DK_OPT_CHARSET: return o.charset;
    }
    return -1;
}

int dk_process_key(dk_engine* e, uint32_t ch, uint32_t flags, dk_result* out) {
    dk::KeyResult r = e->eng.processKey((char32_t)ch, flags);
    fill(e, r, out);
    return r.consumed;
}

int dk_commit(dk_engine* e, dk_result* out) {
    dk::KeyResult r = e->eng.commit();
    fill(e, r, out);
    return r.commit;
}

void dk_reset(dk_engine* e) { e->eng.reset(); }

const char* dk_version(void) { return "0.1.0"; }

} // extern "C"
