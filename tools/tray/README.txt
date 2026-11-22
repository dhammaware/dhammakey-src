DhammaKey — Tray icon (14/11/2026)

latin/   bộ chữ Latinh: V P S M T H K J N, E = tắt, V đỏ = xung đột
native/  bộ chữ bản địa: Vi Pā अ က ก 中 한 あ 喃, EN = tắt, Vi đỏ = xung đột
         (registry TrayGlyph: 0 = latin, 1 = native)

windows/  dk-<chế độ>-light.ico (thanh tác vụ sáng), -dark.ico (thanh tác vụ tối); mỗi .ico có 16/20/24/32 px
macos/    dk-<chế độ>Template.png 18 px và @2x 36 px (ảnh template, macOS tự đổi màu)
linux/    dk-<chế độ>.svg và dk-<chế độ>-symbolic.svg
png/      PNG rời 16–64 px;  src/  SVG gốc từng cỡ

Chữ đã chuyển thành nét vẽ, không cần cài font.
Font dùng: Inter Bold (Latinh, Vi, Pā, EN) · Poppins Bold (अ) · Loma Bold (ก) · Noto Sans Myanmar Bold (က, kèm trong fonts/, giấy phép OFL) · Noto Sans CJK Bold (中 한 あ 喃).
Chạy lại:  python3 make_tray.py out   (cần fontTools, Pillow, rsvg-convert, ImageMagick)
