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
#include <msctf.h>

// CLSID của bộ gõ và GUID hồ sơ ngôn ngữ — KHÔNG đổi sau khi phát hành
// {83DBF8F6-B142-4729-B119-E97BB941BB71}
static const CLSID CLSID_DhammaKey =
    {0x83dbf8f6, 0xb142, 0x4729, {0xb1, 0x19, 0xe9, 0x7b, 0xb9, 0x41, 0xbb, 0x71}};
// {ABFBCCD1-F519-4F01-BC62-56C47C8D6C51}
static const GUID GUID_DhammaKeyProfile =
    {0xabfbccd1, 0xf519, 0x4f01, {0xbc, 0x62, 0x56, 0xc4, 0x7c, 0x8d, 0x6c, 0x51}};

// Phím tắt: Ctrl+` bật/tắt tiếng Việt, Ctrl+Shift+P bật/tắt Pali Velthuis
// {5C0A2E1B-6F3D-4C8A-9E21-7B4D0C9F1A01}
static const GUID GUID_PreservedKey_Toggle =
    {0x5c0a2e1b, 0x6f3d, 0x4c8a, {0x9e, 0x21, 0x7b, 0x4d, 0x0c, 0x9f, 0x1a, 0x01}};
// {5C0A2E1B-6F3D-4C8A-9E21-7B4D0C9F1A02}
static const GUID GUID_PreservedKey_Pali =
    {0x5c0a2e1b, 0x6f3d, 0x4c8a, {0x9e, 0x21, 0x7b, 0x4d, 0x0c, 0x9f, 0x1a, 0x02}};

#define DK_LANGID MAKELANGID(LANG_VIETNAMESE, SUBLANG_VIETNAMESE_VIETNAM) // 0x042A
#define DK_DESCRIPTION L"DhammaKey"
#define DK_REG_KEY L"Software\\DhammaKey"
#include "resource.h"   // IDI_DHAMMAKEY, DK_TRAY_ID

extern HINSTANCE g_hInst;
extern LONG g_dllRefs;
void DllAddRef();
void DllRelease();
