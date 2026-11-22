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
// Bảng mã tiếng Việt cũ (0.3.20): đổi chữ Unicode dựng sẵn sang cách ghi của từng bảng mã.
#include <string>

namespace dk {

// Đổi một chuỗi Unicode dựng sẵn (NFC) sang bảng mã cs (DK_CHARSET_*).
// Bảng mã 8-bit (TCVN3, VNI, VPS, VISCII) trả về mỗi byte thành ký tự mà phông .VnTime, VNI-Times, VPS…
// gắn với byte đó (gần như Windows-1252, xem legacyChar). CP1258 trả về chữ Unicode của bảng mã 1258.
// Chữ không có trong bảng mã (Pāli ā ṃ…) giữ nguyên Unicode, trừ NCR (mọi chữ ngoài ASCII thành &#…;).
std::u32string toCharset(const std::u32string& nfc, int cs);

} // namespace dk
