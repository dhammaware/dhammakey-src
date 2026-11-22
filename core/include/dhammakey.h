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

/*
 * DhammaKey — lõi bộ gõ (C API)
 * Dùng chung cho mọi lớp vỏ: Windows (TSF / hook), macOS, Linux, Android, iOS.
 *
 * Mô hình:
 *   Lớp vỏ gửi từng phím (ký tự Unicode mà phím sinh ra) vào dk_process_key().
 *   Lõi giữ "từ đang gõ" và trả về:
 *     - preedit    : toàn bộ từ đang gõ (cho kiểu composition của TSF/IMKit/IBus)
 *     - backspaces + insert : hiệu giữa màn hình cũ và mới (cho kiểu hook như Unikey)
 *   Khi gặp phím kết thúc từ (dấu cách, dấu câu, Enter…), lõi trả consumed = 0,
 *   commit = 1, preedit = từ cuối cùng; lớp vỏ chốt từ rồi cho phím đi tiếp.
 */
#ifndef DHAMMAKEY_H
#define DHAMMAKEY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) && defined(DK_BUILD_SHARED)
#  define DK_API __declspec(dllexport)
#else
#  define DK_API
#endif

typedef struct dk_engine dk_engine;

/* Kiểu gõ */
enum {
    DK_METHOD_OFF = 0,          /* tắt — gõ như bàn phím thường */
    DK_METHOD_TELEX = 1,
    DK_METHOD_VNI = 2,
    DK_METHOD_VIQR = 3,
    DK_METHOD_PALI_VELTHUIS = 4 /* Pali/Sanskrit La-tinh: aa→ā, .t→ṭ, "n→ṅ, ~n→ñ … */
};

/* Tuỳ chọn */
enum {
    DK_OPT_METHOD = 1,          /* một trong DK_METHOD_* */
    DK_OPT_TONE_NEW_STYLE = 2,  /* 0: òa, thủy (kiểu cũ — mặc định) | 1: oà, thuỷ */
    DK_OPT_SPELL_CHECK = 3,     /* 1: khôi phục phím khi từ không phải tiếng Việt (mặc định 1) */
    DK_OPT_W_TO_U = 4,          /* Telex: w đứng riêng → ư (mặc định 1) */
    DK_OPT_OUTPUT_COMBINING = 5,/* 0: Unicode dựng sẵn (mặc định) | 1: Unicode tổ hợp */
    DK_OPT_PALI_M_DOT_ABOVE = 6,/* 0: ṃ (VRI — mặc định) | 1: ṁ (PTS) */
    DK_OPT_CHARSET = 7          /* bảng mã xuất ra, một trong DK_CHARSET_* (0.3.20) */
};

/* Bảng mã xuất ra (0.3.20). Bảng mã 8-bit trả mỗi byte thành một ký tự theo Windows-1252
   (đúng cách phông .VnTime, VNI-Times… nhận chữ trong Windows). Chữ Pāli/Sanskrit không có
   trong bảng mã cũ thì vẫn ra Unicode. DK_CHARSET_UNICODE_COMBINING = DK_OPT_OUTPUT_COMBINING 1. */
enum {
    DK_CHARSET_UNICODE = 0,           /* Unicode dựng sẵn (mặc định) */
    DK_CHARSET_UNICODE_COMBINING = 1, /* Unicode tổ hợp */
    DK_CHARSET_TCVN3 = 2,             /* TCVN3 (ABC) — phông .VnTime; chữ hoa có dấu dùng phông .VnTimeH */
    DK_CHARSET_VNI = 3,               /* VNI Windows — phông VNI-Times… (chữ có dấu = chữ gốc + byte dấu) */
    DK_CHARSET_VPS = 4,               /* VPS */
    DK_CHARSET_VISCII = 5,            /* VISCII */
    DK_CHARSET_VIQR = 6,              /* VIQR: a' a` a? a~ a. a^ a( o+ u+ dd */
    DK_CHARSET_CP1258 = 7,            /* Windows CP 1258 */
    DK_CHARSET_NCR_DEC = 8,           /* NCR thập phân: &#7889; */
    DK_CHARSET_NCR_HEX = 9,           /* NCR thập lục: &#x1ED1; */
    DK_CHARSET_COUNT = 10
};

/* Cờ phím */
enum {
    DK_FLAG_ALTGR = 1           /* phím được bấm cùng AltGr (Alt phải) → ký tự Pali */
};

/* Phím đặc biệt */
#define DK_KEY_BACKSPACE 0x08

typedef struct {
    int consumed;        /* 1: lõi đã xử lý phím, lớp vỏ KHÔNG gửi phím gốc đi */
    int commit;          /* 1: từ đã kết thúc; preedit = từ cuối cùng cần chốt */
    int backspaces;      /* kiểu hook: số ký tự (code point) cần xoá */
    const char* insert;  /* kiểu hook: chuỗi UTF-8 cần chèn sau khi xoá */
    const char* preedit; /* kiểu composition: toàn bộ từ đang gõ (UTF-8) */
} dk_result;

DK_API dk_engine* dk_engine_new(void);
DK_API void       dk_engine_free(dk_engine* e);
DK_API int        dk_set_option(dk_engine* e, int option, int value);
DK_API int        dk_get_option(dk_engine* e, int option);

/* ch: ký tự Unicode phím sinh ra (đã tính Shift/CapsLock), hoặc DK_KEY_BACKSPACE */
DK_API int  dk_process_key(dk_engine* e, uint32_t ch, uint32_t flags, dk_result* out);
/* Chốt từ đang gõ (khi đổi cửa sổ, bấm chuột, phím mũi tên…) */
DK_API int  dk_commit(dk_engine* e, dk_result* out);
/* Huỷ từ đang gõ, không chốt */
DK_API void dk_reset(dk_engine* e);

DK_API const char* dk_version(void);

#ifdef __cplusplus
}
#endif
#endif /* DHAMMAKEY_H */
