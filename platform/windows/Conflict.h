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
#include <windows.h>
#include <string>

// Các bộ gõ tiếng Việt chạy bằng "hook" bàn phím (UniKey, EVKey, GoTiengViet, OpenKey,
// Vietkey, Laban Key, VPSKeys, WinVNKey…) nằm trên mọi bàn phím của Windows, kể cả DhammaKey.
// Nếu cùng bật, chữ bị biến hai lần → gõ sai.
// DhammaKey không ra lệnh được cho chúng, chỉ nhận ra và (khi người dùng đồng ý) tắt chúng.
namespace conflict {

// Tên bộ gõ đang chạy (ví dụ "UniKey"), hoặc "" nếu không có
std::wstring FindRunning();

// Tên bộ gõ được đặt tự chạy khi mở máy (và chưa bị tắt), hoặc ""
std::wstring FindAutostart();

struct Result {
    int disabled = 0;    // số mục tự chạy đã tắt
    int failed = 0;      // số mục tự chạy không tắt được (cần quyền quản trị)
    int stopped = 0;     // số tiến trình đã dừng
    int stopFailed = 0;  // số tiến trình không dừng được
};

// Tắt tự chạy (giống Task Manager → Startup → Disable, bật lại được) và dừng tiến trình đang chạy.
// Không xóa chương trình.
Result DisableAndStop();

} // namespace conflict
