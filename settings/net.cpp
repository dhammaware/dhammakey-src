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

#include "net.h"

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#include <string>

namespace net {
namespace {

#ifndef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 0x00000800
#endif

struct Handle {
    HINTERNET h = nullptr;
    ~Handle() { if (h) WinHttpCloseHandle(h); }
};

Response send(const QString& url, const wchar_t* verb, const QByteArray& body, const wchar_t* headers,
              const Progress& progress) {
    Response r;
    std::wstring w = url.toStdWString();
    wchar_t host[256] = {}, path[4096] = {}, extra[4096] = {};
    URL_COMPONENTS uc = {};
    uc.dwStructSize = sizeof(uc);
    uc.lpszHostName = host;  uc.dwHostNameLength = 256;
    uc.lpszUrlPath = path;   uc.dwUrlPathLength = 4096;
    uc.lpszExtraInfo = extra; uc.dwExtraInfoLength = 4096;
    if (!WinHttpCrackUrl(w.c_str(), 0, 0, &uc)) { r.error = QStringLiteral("Địa chỉ không hợp lệ."); return r; }

    Handle session, connect, request;
    session.h = WinHttpOpen(L"DhammaKey", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                            WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session.h) { r.error = QStringLiteral("Không mở được kết nối mạng."); return r; }
    // Windows 7 mặc định chưa bật TLS 1.2
    DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
    WinHttpSetOption(session.h, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));
    WinHttpSetTimeouts(session.h, 10000, 10000, 15000, 30000);

    connect.h = WinHttpConnect(session.h, host, uc.nPort, 0);
    std::wstring object = std::wstring(path) + extra;
    if (connect.h)
        request.h = WinHttpOpenRequest(connect.h, verb, object.c_str(), nullptr, WINHTTP_NO_REFERER,
                                       WINHTTP_DEFAULT_ACCEPT_TYPES,
                                       uc.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
    if (!request.h) { r.error = QStringLiteral("Không kết nối được tới máy chủ."); return r; }

    BOOL sent = WinHttpSendRequest(request.h, headers ? headers : WINHTTP_NO_ADDITIONAL_HEADERS,
                                   headers ? (DWORD)-1L : 0,
                                   body.isEmpty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)body.constData(),
                                   (DWORD)body.size(), (DWORD)body.size(), 0);
    if (!sent || !WinHttpReceiveResponse(request.h, nullptr)) {
        r.error = QStringLiteral("Không kết nối được. Hãy kiểm tra kết nối Internet.");
        return r;
    }
    DWORD status = 0, len = sizeof(status);
    WinHttpQueryHeaders(request.h, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &len, WINHTTP_NO_HEADER_INDEX);
    r.status = (int)status;
    qint64 total = -1;
    DWORD cl = 0;
    len = sizeof(cl);
    if (WinHttpQueryHeaders(request.h, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &cl, &len, WINHTTP_NO_HEADER_INDEX))
        total = cl;

    for (;;) {
        DWORD avail = 0;
        if (!WinHttpQueryDataAvailable(request.h, &avail)) { r.error = QStringLiteral("Mất kết nối khi đang tải."); break; }
        if (avail == 0) break;
        QByteArray chunk((int)avail, Qt::Uninitialized);
        DWORD read = 0;
        if (!WinHttpReadData(request.h, chunk.data(), avail, &read)) { r.error = QStringLiteral("Mất kết nối khi đang tải."); break; }
        r.body.append(chunk.constData(), (int)read);
        if (progress && !progress(r.body.size(), total)) { r.error = QStringLiteral("Đã hủy."); break; }
    }
    if (r.error.isEmpty() && (r.status < 200 || r.status >= 300))
        r.error = QStringLiteral("Máy chủ trả lỗi %1.").arg(r.status);
    return r;
}

} // namespace

Response get(const QString& url, const Progress& progress) {
    return send(url, L"GET", QByteArray(), nullptr, progress);
}

Response postForm(const QString& url, const QByteArray& formData) {
    return send(url, L"POST", formData, L"Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n", {});
}

} // namespace net

#else  // macOS / Linux: làm sau

namespace net {
Response get(const QString&, const Progress&) { Response r; r.error = QStringLiteral("Chưa hỗ trợ."); return r; }
Response postForm(const QString&, const QByteArray&) { Response r; r.error = QStringLiteral("Chưa hỗ trợ."); return r; }
} // namespace net

#endif
