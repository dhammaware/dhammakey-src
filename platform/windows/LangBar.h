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
#include "Globals.h"

class TextService;

// Mở DhammaKeySettings.exe (args: nullptr hoặc L"--check-update")
void LaunchDhammaKeySettings(const wchar_t* args);

// Biểu tượng DhammaKey ở khay hệ thống (Windows 8+) / thanh ngôn ngữ (Windows 7).
//   V/Vi = gõ tiếng Việt · E/EN = tắt (gõ tiếng Anh) · P/Pā = gõ Pali · V đỏ = xung đột (xem resource.h)
//   Bấm chuột trái: bật/tắt tiếng Việt · Bấm chuột phải: chọn kiểu gõ
class LangBarButton : public ITfLangBarItemButton, public ITfSource {
public:
    explicit LangBarButton(TextService* ts);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfLangBarItem
    STDMETHODIMP GetInfo(TF_LANGBARITEMINFO* info) override;
    STDMETHODIMP GetStatus(DWORD* status) override;
    STDMETHODIMP Show(BOOL) override { return E_NOTIMPL; }
    STDMETHODIMP GetTooltipString(BSTR* tip) override;

    // ITfLangBarItemButton
    STDMETHODIMP OnClick(TfLBIClick click, POINT pt, const RECT* area) override;
    STDMETHODIMP InitMenu(ITfMenu* menu) override;
    STDMETHODIMP OnMenuSelect(UINT id) override;
    STDMETHODIMP GetIcon(HICON* icon) override;
    STDMETHODIMP GetText(BSTR* text) override;

    // ITfSource
    STDMETHODIMP AdviseSink(REFIID riid, IUnknown* punk, DWORD* cookie) override;
    STDMETHODIMP UnadviseSink(DWORD cookie) override;

    void Refresh();  // báo Windows vẽ lại biểu tượng sau khi đổi kiểu gõ
    void Detach();   // TextService sắp hủy

private:
    ~LangBarButton();
    void ShowMenu(POINT pt);

    LONG refs_ = 1;
    TextService* ts_;
    ITfLangBarItemSink* sink_ = nullptr;
};
