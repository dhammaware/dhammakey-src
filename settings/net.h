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
#include <QByteArray>
#include <QString>
#include <functional>

// Gửi/nhận qua mạng. Windows dùng WinHTTP (có sẵn HTTPS, không cần OpenSSL).
namespace net {

struct Response {
    int status = 0;       // mã HTTP (200 = thành công), 0 = không kết nối được
    QByteArray body;
    QString error;        // mô tả lỗi cho người dùng, rỗng nếu ổn
    bool ok() const { return error.isEmpty() && status >= 200 && status < 300; }
};

// progress(đã nhận, tổng số byte hoặc -1); trả false để hủy
using Progress = std::function<bool(qint64, qint64)>;

Response get(const QString& url, const Progress& progress = {});
Response postForm(const QString& url, const QByteArray& formData);

} // namespace net
