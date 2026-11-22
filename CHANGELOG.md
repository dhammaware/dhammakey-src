# Nhật ký thay đổi DhammaKey

Khi đổi số phiên bản, sửa đủ hai chỗ (từ 0.3.18):
- `CMakeLists.txt`: dòng `project(DhammaKey VERSION …)`. Bảng điều khiển và thông tin tệp của DhammaKey.dll, DhammaKeySettings.exe lấy từ đây.
- `installer/windows/DhammaKey.wxs`: dòng `<?define DkVersion = "x.y.z" ?>`.

Đồng thời thêm một mục lên đầu `settings/whatsnew.md`. Đó là nhật ký "Có gì mới" hiển thị ở trang Cập nhật, viết bằng lời dễ hiểu cho người dùng.

Quy tắc viết `whatsnew.md`: thay đổi chỉ về giao diện thì ghi đúng một dòng "Tinh chỉnh giao diện.", đứng cuối mục. Các bản liền nhau chỉ có tinh chỉnh giao diện được gộp thành một mục, ví dụ "0.3.14 – 0.3.16".

Các mục đánh dấu "(không phát hành)" là bản nội bộ không đưa đến người dùng.

## 0.4.0 — đang làm
- Menu khay (cả menu Win32 `ShowMenu` và menu TSF `InitMenu`): thêm "Kiểm tra cập nhật" ngay dưới "Bảng điều khiển", mở `DhammaKeySettings.exe --check-update`.
- Menu khay đổi theo ngôn ngữ giao diện (registry `UiLang`). Telex, VNI, VIQR giữ nguyên.
- Chưa làm: 186 chuỗi của cửa sổ Cài đặt; chưa kiểm trên máy Windows thật.

## 0.3.34 — 16/11/2026
Gộp cả 0.3.33.
- Menu khay, mục "Bảng mã": mỗi mục có biểu tượng theo nhóm (quả cầu Unicode, chữ a kèm dấu tổ hợp, tờ giấy cho TCVN3/VNI/VPS/VISCII/CP 1258, `</>` cho VIQR/NCR) và nhãn nhóm bên phải ("Mặc định", "Phông cũ", "Chữ thường").
- Sửa lỗi: trong menu, "&" là ký hiệu phím tắt nên "NCR thập phân · &#7879;" mất chữ "&". Nay viết đôi "&&".
- Thanh địa chỉ cửa sổ ẩn danh Chrome vẫn chưa tắt được. Đã thêm `FocusIsProtected` và ghi nhật ký dò trợ năng khi bật `GuardDebug`.
- Chưa làm: danh sách ứng dụng loại trừ `ExcludedApps`.

## 0.3.33 — 16/11/2026
- Menu khay: biểu tượng không còn sát chữ ở các mục Pāli, Tắt, Bảng mã, Bảng điều khiển, Về. Bitmap biểu tượng có đệm trong suốt bên phải (`Px(8)`).
- Chỉ giao diện; không đổi lõi và bộ gõ.

## 0.3.32 — 16/11/2026
- Khung đầu tiên cho macOS: `platform/macos` (IMKit) và `tools/macos/make-pkg.sh`. Option phải + chữ cho Pāli dùng cùng bảng `dhammakey_pali_keys.h`. Chưa biên dịch trên Mac.
- Cài đặt: trang mới "Tự tắt gõ dấu" với bốn công tắc ghi registry `PasswordGuard`, `UrlGuard`, `ConsoleGuard`, `AsciiFieldGuard` (mặc định bật).
- Lõi và bộ gõ không đổi.

## 0.3.31 — 16/11/2026
- Alt phải + g đổi thành Alt phải + k, vì Alt + g trùng phím tắt của Google Gemini. Bỏ phím g khỏi danh sách phím giữ trước.
- Bảng phím Alt phải được gom về một nơi: `core/include/dhammakey_pali_keys.h` (`kDkPaliAltKeys`). Đổi phím chỉ sửa tệp này.
- Cài đặt: dấu ⓘ ở các mục trang Kiểu gõ và trang Pāli. Bảng Velthuis 16 tổ hợp.
- Lõi: 228/228 phép thử đạt.

## 0.3.30 — 16/11/2026
- Velthuis: dấu chấm mở đầu từ (`.t` đầu từ) nay ra được ṭ. Thêm 6 ca thử.
- Alt phải + chữ: phím bổ trợ đứng riêng không còn bị bộ gõ nhận khi đang soạn. Đăng ký 28 phím giữ trước `TF_MOD_RALT` (+Shift).
- Ghi nhật ký dò lỗi khi `GuardDebug` = 1.

## 0.3.29 — 15/11/2026
- Bộ cài: bỏ `ARPCONTACT`, tên người sáng lập không hiển thị công khai trong Programs and Features.
- 0.3.28 không phát hành vì tệp cài còn tên đó.

## 0.3.28 — 15/11/2026 (không phát hành)
- `FocusIsProtected` đi xuống theo `get_accFocus` (tối đa 12 tầng), dừng khi gặp ô mật khẩu hoặc ô thường.
- Khi IS_PRIVATE mà MSAA chưa rõ, tạm tắt gõ dấu và hỏi lại ở phím sau.

## 0.3.27 — 15/11/2026 (không phát hành)
- `FocusIsProtected` chỉ kết luận "không phải mật khẩu" khi ô đang trỏ là ô chữ không được bảo vệ. Các trường hợp còn lại là "chưa biết", không nhớ kết quả.

## 0.3.26 — 15/11/2026 (không phát hành)
- Chỉ khi có IS_PRIVATE mới hỏi thêm qua MSAA (`STATE_SYSTEM_PROTECTED`). Liên kết thêm `oleacc`.
- "Có gì mới": thêm lại mục cửa sổ dòng lệnh.

## 0.3.25 — 15/11/2026 (không phát hành)
- Kiểm tra loại ô ở đầu `OnKeyDown`, phòng trường hợp ứng dụng bỏ qua `OnTestKeyDown`.

## 0.3.24 — 15/11/2026 (không phát hành)
- Chrome và Edge báo InputScope qua thuộc tính của ứng dụng (`GetAppProperty`). Nay đọc thuộc tính này trước.
- Phím tắt của ứng dụng (Ctrl+A/C/V) đi thẳng, không hỏi loại ô.
- `tools/release/build-all.bat`: đọc số phiên bản, đối chiếu `DkVersion`, dựng x86 và x64, chạy `dk_tests`, dựng MSI.

## 0.3.23 — 15/11/2026 (không phát hành)
- Tự tắt gõ dấu (`platform/windows/Guard.cpp`): ô mật khẩu, thanh địa chỉ, dòng lệnh, ô email/số thì phím đi thẳng vào ứng dụng.
- Khay: khi ở ô được bảo vệ, biểu tượng hiện trạng thái tắt, chú thích ghi rõ loại ô.
- Chưa làm: dò bằng UI Automation cho trình duyệt không báo IS_URL.

## 0.3.22 — 15/11/2026
- `legacyChar`: byte 0xB7 trả về U+00B7 như cp1252. Đã thử trong Word với .VnTime và .VnArialH.

## 0.3.21 — 15/11/2026
- Bộ cài: `MajorUpgrade` có `AllowSameVersionUpgrades="yes"` và `Schedule="afterInstallInitialize"`, gỡ bản cũ trước khi chép tệp.
- `ScheduleReboot` sau `SetupUser`: luôn nhắc khởi động lại sau khi cài.
- `DisplayIcon` trong khoá Uninstall trỏ vào biểu tượng DhammaKey.

## 0.3.20 — 15/11/2026
- Bảng mã tiếng Việt cũ: tuỳ chọn `DK_OPT_CHARSET` với Unicode dựng sẵn (mặc định), Unicode tổ hợp, TCVN3/ABC, VNI Windows, VPS, VISCII, VIQR, Windows CP 1258, NCR thập phân và thập lục.
- Bảng 134 chữ trong `core/src/charset_tables.inc`, kiểm bằng `tools/charset/verify_tables.py`. TCVN1 không được hỗ trợ.
- Bảng điều khiển: danh sách thả xuống "Bảng mã" thay cho dòng "Unicode tổ hợp".
- Menu khay có mục "Bảng mã" mở danh sách 10 bảng mã. Chân menu tự vẽ có logo và chấm xanh ṃ nảy.
- Kiểm thử lõi: 212/212 đạt.

## 0.3.19 — 15/11/2026
- Kiểm tra cập nhật phân biệt bốn trường hợp: không kết nối được, chưa có thông tin bản mới (404 hoặc không phải JSON), máy chủ trục trặc, và thông tin sai định dạng.
- `latest.json` chung của DhammaWare: đọc mục `"dhammakey"` (hằng `DK_UPDATE_APP_KEY`).
- Menu khay: "Về chúng tôi" đổi thành "Về DhammaKey™".

## 0.3.18 — 15/11/2026
- Cập nhật tự động dùng địa chỉ `https://dhammaware.org/update/latest.json` (hằng `DK_UPDATE_URL`). `latest.json` bắt buộc có `version`, `url` (HTTPS) và `sha256` (64 ký tự hex).
- Bộ cài tải về luôn được so SHA-256 trước khi chạy.
- Slogan "Breaking barriers – typing beyond" hiển thị ở thanh tiêu đề và cửa sổ Về.
- Tiêu đề SPDX và điều khoản phi thương mại ở đầu mọi tệp mã. Thêm `LICENSE`, `LICENSE.vi.md`, `THIRD-PARTY-NOTICES.txt`, `LICENSES/`, `REUSE.toml`.
- Thông tin tệp đầy đủ cho DLL và EXE. Số phiên bản lấy từ CMake.
- `tools/release/make-latest.bat` tính SHA-256 và ghi `latest-x.y.z.json`.

## 0.3.17 — 14/11/2026 (không có trong mã)
- Thay đổi được làm lại ở 0.3.18.

## 0.3.16 — 14/11/2026
- Cửa sổ Về: bánh xe luôn quay, không phụ thuộc cài đặt hiệu ứng của Windows.
- Khung giới thiệu không còn giãn; cửa sổ cao vừa nội dung.
- Thêm logo DhammaKey trước tiêu đề Về.

## 0.3.15 — 14/11/2026
- Menu chuột phải: đường kẻ ngăn trước "Bảng điều khiển DhammaKey…".
- Cửa sổ Về: bánh xe DhammaWare quay liên tục và loé sáng định kỳ.
- Lời giới thiệu mới; bỏ kệ Pháp Cú.

## 0.3.14 — 14/11/2026
- Cửa sổ Về: lời giới thiệu bản ngắn với khẩu hiệu "Đúng ngôn ngữ. Đúng truyền thống. Không rào cản."

## 0.3.13 — 14/11/2026
- Trang Cập nhật có mục "Có gì mới", hiển thị `settings/whatsnew.md`.

## 0.3.12 — 14/11/2026
- Cửa sổ Về: câu mở đầu mới.

## 0.3.11 — 14/11/2026
- Sửa lỗi biên dịch Bảng điều khiển ở 0.3.9–0.3.10. Bản 0.3.10 đã đóng MSI vẫn mang Bảng điều khiển lỗi, không dùng.

## 0.3.10 — 14/11/2026
- Bản quyền: "© 2026 Ujudhammo" ở chân cột trái, cửa sổ Về và thuộc tính tệp.

## 0.3.9 — 14/11/2026
- Menu khay: "Về chúng tôi" mở cửa sổ giới thiệu ngắn (`DhammaKeySettings.exe --about`).
- Thuộc tính tệp: CompanyName DhammaWare, LegalCopyright, LegalTrademarks.
- Ký hiệu ™ cho DhammaKey và DhammaWare.

## 0.3.8 — 14/11/2026
- Bánh xe DhammaWare ở chân cột trái to hơn, 44 px.

## 0.3.7 — 14/11/2026
- Dòng "Bật DhammaKey khi mở máy" có biểu tượng nút nguồn.

## 0.3.6 — 14/11/2026
- Công tắc "Bật DhammaKey khi mở máy" (trang Kiểu gõ, mặc định bật, registry `DefaultIme`). Tắt thì trả lại bộ gõ mặc định trước đó.
- DLL có thêm `DkLogon`, `DkSetDefault`, `DkUninstallUser`.

## 0.3.5 — 14/11/2026
- Bảng điều khiển: thanh tiêu đề tự vẽ, cửa sổ cỡ cố định 840 × 606, logo DhammaKey và DhammaWare ở các vị trí riêng.

## 0.3.4
- Chữ trên tray icon to thêm khoảng 15%.
- Ô ngôn ngữ dùng biểu tượng riêng `dhammakey-indicator.ico`.

## 0.3.3
- Logo phím ṃ cho ứng dụng và lối tắt Start.

## 0.3.2
- Tray icon thương hiệu. Đổi tên cửa sổ thành "Bảng điều khiển DhammaKey".
