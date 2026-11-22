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
// ======================================================================
//  Cấu hình mạng của DhammaKey — sửa ở đây khi có trang web / Google Form
// ======================================================================

// Tệp cho biết bản mới nhất, đặt trên trang web DhammaWare (địa chỉ người dùng chốt 14/11/2026).
// Địa chỉ này nằm cứng trong mọi bản đã phát hành: phía website KHÔNG được đổi đường dẫn.
// Chỉ HTTPS; tải qua WinHTTP của Windows (không cần OpenSSL), giữ kiểm tra chứng chỉ.
// Đây là tệp CHUNG cho mọi ứng dụng DhammaWare (đã chốt 15/11/2026): mỗi ứng dụng một mục theo
// tên khoá; DhammaKey đọc mục DK_UPDATE_APP_KEY (vẫn nhận dạng phẳng cũ, các trường nằm ở gốc).
// Các trường ứng dụng đọc (tên trường phải đúng như dưới):
// {
//   "dhammakey": {
//     "version": "0.4.0",                       // số phiên bản x.y.z, so với DK_VERSION
//     "url":     "https://dhammaware.org/download/dhammakey/0.4.0/...",  // bộ cài (HTTPS)
//     "sha256":  "…64 ký tự hex…",              // mã băm bộ cài; khác mã thì không chạy bộ cài
//     "notes":   "Thêm gõ Sanskrit; sửa lỗi …"  // ghi chú ngắn, hiện cho người dùng
//   }
// }
// Chưa có tệp (404), tệp không phải JSON hoặc không có mục "dhammakey": trang Cập nhật báo
// "Chưa có thông tin bản mới trên máy chủ" (không phải lỗi); kiểm tra hằng tuần im lặng.
#define DK_UPDATE_URL "https://dhammaware.org/update/latest.json"
#define DK_UPDATE_APP_KEY "dhammakey"

// Google Form nhận góp ý. Cách lấy các mã: xem hướng dẫn trong docs/gop-y-google-form.md
//   DK_FEEDBACK_FORM_ID : đoạn giữa ".../forms/d/e/" và "/viewform" trong đường dẫn chia sẻ biểu mẫu
//   DK_FEEDBACK_ENTRY_* : mã "entry.123456789" của từng câu hỏi
#define DK_FEEDBACK_FORM_ID       "FORM_ID"
#define DK_FEEDBACK_ENTRY_KIND    "entry.1000000001"   // Loại: Góp ý / Báo lỗi / Đề nghị tính năng
#define DK_FEEDBACK_ENTRY_TEXT    "entry.1000000002"   // Nội dung
#define DK_FEEDBACK_ENTRY_CONTACT "entry.1000000003"   // Email / cách liên hệ
#define DK_FEEDBACK_ENTRY_SYSTEM  "entry.1000000004"   // Phiên bản DhammaKey, Windows
