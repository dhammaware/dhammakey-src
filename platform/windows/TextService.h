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
#include "dhammakey.h"
#include <string>

class LangBarButton;

// Bộ gõ TSF: nhận phím, gửi vào lõi, hiển thị chữ đang gõ bằng composition.
class TextService : public ITfTextInputProcessorEx,
                    public ITfKeyEventSink,
                    public ITfCompositionSink,
                    public ITfThreadMgrEventSink {
public:
    TextService();
    ~TextService();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfTextInputProcessor(Ex)
    STDMETHODIMP Activate(ITfThreadMgr* tm, TfClientId cid) override { return ActivateEx(tm, cid, 0); }
    STDMETHODIMP ActivateEx(ITfThreadMgr* tm, TfClientId cid, DWORD flags) override;
    STDMETHODIMP Deactivate() override;

    // ITfKeyEventSink
    STDMETHODIMP OnSetFocus(BOOL) override { return S_OK; }
    STDMETHODIMP OnTestKeyDown(ITfContext* ctx, WPARAM wp, LPARAM lp, BOOL* eaten) override;
    STDMETHODIMP OnKeyDown(ITfContext* ctx, WPARAM wp, LPARAM lp, BOOL* eaten) override;
    STDMETHODIMP OnTestKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* eaten) override { *eaten = FALSE; return S_OK; }
    STDMETHODIMP OnKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* eaten) override { *eaten = FALSE; return S_OK; }
    STDMETHODIMP OnPreservedKey(ITfContext* ctx, REFGUID guid, BOOL* eaten) override;

    // ITfCompositionSink
    STDMETHODIMP OnCompositionTerminated(TfEditCookie ec, ITfComposition* comp) override;

    // ITfThreadMgrEventSink — đổi cửa sổ thì chốt chữ đang gõ
    STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr*) override { return S_OK; }
    STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr*) override { return S_OK; }
    STDMETHODIMP OnSetFocus(ITfDocumentMgr* focus, ITfDocumentMgr* prev) override;
    STDMETHODIMP OnPushContext(ITfContext*) override { return S_OK; }
    STDMETHODIMP OnPopContext(ITfContext*) override { return S_OK; }

    // Gọi từ edit session
    HRESULT DoUpdate(TfEditCookie ec, ITfContext* ctx, const std::wstring& text, bool finish);

    // Gọi từ biểu tượng ở khay hệ thống
    int Method();
    void ChooseMethod(int method);
    int Charset();                 // bảng mã đang dùng (DK_CHARSET_*) — 0.3.20
    void ChooseCharset(int cs);    // đổi bảng mã từ menu khay, lưu registry cho mọi ứng dụng
    void ToggleOnOff();
    bool HasConflict() const { return !conflictName_.empty(); }
    int Guard() const { return guard_; }   // guard::Kind — ô đang tạm tắt gõ dấu (0.3.23)
    const std::wstring& ConflictName() const { return conflictName_; }

private:
    LONG refs_ = 1;
    ITfThreadMgr* tm_ = nullptr;
    TfClientId cid_ = TF_CLIENTID_NULL;
    DWORD tmSinkCookie_ = TF_INVALID_COOKIE;
    ITfComposition* comp_ = nullptr;
    dk_engine* eng_ = nullptr;
    int lastMethod_ = DK_METHOD_TELEX;
    LangBarButton* lb_ = nullptr;
    std::wstring conflictName_;          // bộ gõ khác đang chạy (UniKey…), "" nếu không có
    ULONGLONG lastConflictCheck_ = 0;
    // Tự tắt gõ dấu (0.3.23): loại ô hiện tại + bộ nhớ tạm để không hỏi lại ở mỗi phím
    int guard_ = 0;
    ITfContext* guardCtx_ = nullptr;     // chỉ so con trỏ, không giữ tham chiếu
    ULONGLONG guardTick_ = 0;
    bool guardUnsure_ = false;           // MSAA chưa trả lời được (0.3.27)
    bool guardPassword_ = true, guardUrl_ = true, guardConsole_ = true, guardAscii_ = true;

    void LoadSettings();
    void SaveMethod();
    bool TranslateKey(WPARAM wp, LPARAM lp, uint32_t& ch, uint32_t& flags);
    void Update(ITfContext* ctx, const char* utf8, bool finish);
    void CommitIfComposing(ITfContext* ctx);
    void CommitFromUI(ITfContext* ctx);
    void ApplyMethod(int next);
    void SyncMethodFromRegistry();
    ITfContext* FocusContext();
    void CheckConflict();
    int UpdateGuard(ITfContext* ctx, bool force);
    int ReadInputScopeGuard(ITfContext* ctx);
};
