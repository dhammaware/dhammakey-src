#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Ujudhammo
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
# DhammaKey - a DhammaWare project - https://dhammaware.org
# Copyright (C) 2026 Ujudhammo. Full terms: LICENSE at the project root.
#
# Kiểm tra độc lập bảng core/src/charset_tables.inc (chạy trên Linux có glibc iconv):
#   TCVN3 chữ thường == iconv TCVN5712-1 · VISCII (trừ chữ hoa ở vùng mã điều khiển) == iconv VISCII
#   CP1258: giải mã bằng Python cp1258 rồi NFC phải ra đúng chữ · VNI: chữ gốc (ASCII hoặc Ơ ơ Ư ư) + byte dấu
# Cách chạy: python3 tools/charset/verify_tables.py core/src/charset_tables.inc
import re, subprocess, sys, unicodedata

def iconv(enc, ch):
    return subprocess.run(['iconv', '-f', 'UTF-8', '-t', enc], input=ch.encode(), capture_output=True).stdout

rows = re.findall(r'\{0x([0-9A-F]+), 0x([0-9A-F]+), 0x([0-9A-F]+), 0x([0-9A-F]+), 0x([0-9A-F]+), 0x([0-9A-F]+), "', open(sys.argv[1], encoding='utf-8').read())
bad = 0
for u, tc, vp, vi, vn, cp in rows:
    ch = chr(int(u, 16)); tc, vi, vn, cp = int(tc, 16), int(vi, 16), int(vn, 16), int(cp, 16)
    if ch.islower() and bytes([tc]) != iconv('TCVN5712-1', ch): print('TCVN3 sai', ch); bad += 1
    ref = iconv('VISCII', ch)
    if ref and ref[0] >= 0x20 and ref[0] != 0xA0 and bytes([vi]) != ref: print('VISCII sai', ch); bad += 1
    b = bytes([cp & 0xFF]) + (bytes([cp >> 8]) if cp >> 8 else b'')
    if unicodedata.normalize('NFC', b.decode('cp1258')) != ch: print('CP1258 sai', ch); bad += 1
    if (vn >> 8) and not ((vn & 0xFF) < 0x80 or (vn & 0xFF) in (0xD4, 0xF4, 0xD6, 0xF6)): print('VNI sai', ch); bad += 1  # gốc ASCII hoặc Ơ ơ Ư ư
print(len(rows), 'chữ;', 'đạt' if not bad else f'{bad} lỗi')
sys.exit(1 if bad else 0)
