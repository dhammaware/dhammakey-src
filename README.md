# DhammaKey 0.1 — Giai đoạn 1: Gõ Việt và Pali trên Windows

Bộ gõ tiếng Việt, Pali và Sanskrit. Chạy được từ Windows 7 SP1 đến Windows 11.

## Cách gõ

| Kiểu gõ | Ví dụ |
|---|---|
| **Telex** (mặc định) | `tieengs vieetj` → tiếng việt · `dduwowngf` → đường |
| **VNI** | `tie61ng vie65t` → tiếng việt |
| **VIQR** | `vie^.t` → việt |
| **Pali Velthuis** | `nibbaana` → nibbāna · `sa"ngha` → saṅgha · `~naa.na` → ñāṇa |

### Pali trong lúc gõ tiếng Việt: AltGr (phím Alt bên phải)

| AltGr + | a | i | u | m | n | k | j | t | d | l |
|---|---|---|---|---|---|---|---|---|---|---|
| ra chữ | ā | ī | ū | ṃ | ṇ | ṅ | ñ | ṭ | ḍ | ḷ |

Thêm cho Sanskrit: AltGr + s → ṣ, c → ś, r → ṛ, h → ḥ. Giữ Shift để ra chữ hoa (AltGr + Shift + a → Ā).

Mỗi chữ có một phím riêng nên gõ được chữ kép như puṇṇa, puñña, satipaṭṭhāna.

### Gõ Pali không dấu vẫn giữ nguyên

Khi một từ không phải là âm tiết tiếng Việt, bộ gõ tự trả lại đúng các phím đã gõ: buddha, dhamma, majjhima, vipassana, sutta, bhante. Chữ tiếng Anh cũng vậy (missing, were).

### Phím tắt

| Phím | Tác dụng |
|---|---|
| `Ctrl` + `` ` `` | Bật / tắt tiếng Việt |
| `Ctrl` + `Shift` + `P` | Bật / tắt chế độ Pali Velthuis |

### Cài đặt (tạm thời sửa trong registry)

Các tuỳ chọn nằm ở `HKEY_CURRENT_USER\Software\DhammaKey`, tất cả là giá trị DWORD:

| Tên | Ý nghĩa |
|---|---|
| `Method` | Kiểu gõ: 0 tắt, 1 Telex, 2 VNI, 3 VIQR, 4 Pali Velthuis |
| `ToneNewStyle` | Kiểu đặt dấu: 0 → òa, thủy · 1 → oà, thuỷ |
| `SpellCheck` | 1: khôi phục phím khi từ không phải tiếng Việt |
| `WToU` | 1: phím w đứng riêng ra ư |
| `OutputCombining` | 0: Unicode dựng sẵn · 1: Unicode tổ hợp |
| `PaliMDotAbove` | 0: ra ṃ (kiểu VRI) · 1: ra ṁ (kiểu PTS) |
| `TrayGlyph` | Chữ trên biểu tượng ở khay: 0 Latinh (V P E) · 1 bản địa (Vi Pā EN). Chưa đặt: máy ở Việt Nam dùng 0, nước khác dùng 1 |

## Biên dịch

```bash
# Lõi và kiểm thử (Linux, macOS, Windows)
cmake -S . -B build && cmake --build build
./build/dk_tests                       # chạy kiểm thử
./build/dkdemo telex "tieengs vieetj"  # gõ thử
```

**Windows:** cần Visual Studio 2022 và WiX 5. Cách đơn giản nhất là đẩy mã nguồn lên GitHub: tệp `.github/workflows/build.yml` sẽ tự biên dịch bản x86 và x64, chạy kiểm thử rồi tạo hai bộ cài `DhammaKey-x64.msi` và `DhammaKey-x86.msi`.

## Cấu trúc thư mục

```
core/        Lõi C++17, dùng chung cho mọi nền tảng
  include/dhammakey.h   C API cho các lớp vỏ
  src/vn.*              Bảng nguyên âm, vị trí đặt dấu, kiểm tra âm tiết tiếng Việt
  src/pali.*            Bảng phím AltGr, kiểu gõ Velthuis
  src/engine.*          Xử lý Telex, VNI, VIQR, khôi phục từ, xoá lùi
  tests/                Kiểm thử tự động (178 ca)
platform/windows/   Bộ gõ TSF (DLL COM)
  res/tray/             Tray icon (Latinh + bản địa, thanh tác vụ sáng/tối), mã trong resource.h
installer/windows/  Bộ cài MSI (WiX)
tools/              dkdemo (gõ thử), roundtrip.py (thử cả đoạn văn)
  tray/                 make_tray.py: sinh lại bộ tray icon từ font
```

## Việc tiếp theo

- Biểu tượng V / P / E trên thanh tác vụ và menu chuột phải.
- Ứng dụng cài đặt có giao diện (Qt 5.15, phong cách Windows 11).
- Gõ tắt và công cụ chuyển mã (TCVN3, VNI-Windows).
- Chế độ hook dự phòng cho các ứng dụng không hỗ trợ TSF.

## Bản quyền và giấy phép

Copyright (C) 2026 Ujudhammo · DhammaKey là một dự án của DhammaWare — https://dhammaware.org

Mã nguồn được **công khai để học hỏi và dùng phi thương mại** theo giấy phép
[PolyForm Noncommercial 1.0.0](LICENSE) (bản dịch tham khảo: [LICENSE.vi.md](LICENSE.vi.md)).
Cá nhân, chùa, trường học, tổ chức từ thiện được dùng, sửa và chia sẻ tự do;
**nghiêm cấm bán, cho thuê hoặc đưa vào sản phẩm, dịch vụ thu phí.**

- Mỗi tệp mã có tiêu đề SPDX ở đầu tệp; tệp không ghi chú thích được (ảnh, biểu tượng, tài liệu) khai trong `REUSE.toml`.
- Thành phần bên thứ ba (Qt, VC++ runtime, phông chữ dùng vẽ tray icon) giữ giấy phép riêng: xem `THIRD-PARTY-NOTICES.txt` và thư mục `LICENSES/`.

DhammaKey™ và DhammaWare™ thuộc về DhammaWare.
