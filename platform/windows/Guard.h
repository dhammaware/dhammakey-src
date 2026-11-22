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

// Tự tắt gõ dấu ở ô mật khẩu, thanh địa chỉ web, dòng lệnh (0.3.23).
// Chỉ đọc, không đổi trạng thái kiểu gõ đã lưu: rời ô là gõ dấu lại như cũ.
namespace guard {

enum Kind { NONE = 0, PASSWORD = 1, URL = 2, CONSOLE = 3, ASCII = 4 };

// Tên ngắn hiện ở chú thích biểu tượng khay
const wchar_t* KindName(int kind);

// Tiến trình này là cửa sổ dòng lệnh (cmd, PowerShell, Windows Terminal, conhost, mintty…)?
// Tính một lần cho cả tiến trình.
bool IsConsoleProcess();

// Ô đang có con trỏ là ô EDIT Win32 kiểu ES_PASSWORD, hoặc nằm trong cửa sổ dòng lệnh?
int FromFocusWindow();

// Ô đang có con trỏ được ứng dụng đánh dấu "được bảo vệ" (mật khẩu) qua MSAA
// (STATE_SYSTEM_PROTECTED). Chrome/Edge báo ô mật khẩu chỉ bằng IS_PRIVATE (61) — giống hệt
// ô thường trong cửa sổ ẩn danh — nên phải hỏi thêm bằng cách này (0.3.26).
// Trả về 1 = được bảo vệ, 2 = thanh địa chỉ của trình duyệt (0.3.34), 0 = không, -1 = chưa biết (ứng dụng chưa dựng xong thông tin trợ năng).
// info: chuỗi ngắn ghi lại từng bước, để dò lỗi (GuardLast).
int FocusIsProtected(std::wstring& info);

// Kiểu ô theo InputScope mà ứng dụng báo qua TSF (IS_PASSWORD, IS_URL…)
int FromInputScope(int scope);

} // namespace guard
