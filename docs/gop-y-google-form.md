# Thiết lập Google Form nhận góp ý cho DhammaKey

1. Vào https://forms.google.com bằng tài khoản Google của DhammaWare, tạo biểu mẫu mới, đặt tên "Góp ý DhammaKey".
2. Tạo **4 câu hỏi**, đúng thứ tự, đều là loại **Trả lời ngắn** hoặc **Đoạn văn**, KHÔNG bật "Bắt buộc":
   1. Loại
   2. Nội dung (chọn loại Đoạn văn)
   3. Liên hệ
   4. Máy
3. Trong **Cài đặt** của biểu mẫu: tắt "Thu thập địa chỉ email" và tắt "Giới hạn 1 câu trả lời" (để người dùng không cần đăng nhập).
4. Lấy mã:
   - Bấm dấu ⋮ (góc trên phải) → **Nhận đường liên kết điền sẵn** (Get pre-filled link).
   - Gõ chữ `a` vào câu 1, `b` vào câu 2, `c` vào câu 3, `d` vào câu 4 → **Nhận đường liên kết** → **Sao chép**.
   - Đường liên kết có dạng:
     `https://docs.google.com/forms/d/e/1FAIpQLSf…XYZ/viewform?usp=pp_url&entry.111=a&entry.222=b&entry.333=c&entry.444=d`
5. Gửi đường liên kết đó cho Claude, hoặc tự sửa tệp `settings/config.h`:
   - `DK_FEEDBACK_FORM_ID` = đoạn `1FAIpQLSf…XYZ` (giữa `/d/e/` và `/viewform`)
   - `DK_FEEDBACK_ENTRY_KIND` = `entry.111` (ứng với `a`), `…_TEXT` = `entry.222`, `…_CONTACT` = `entry.333`, `…_SYSTEM` = `entry.444`
6. Biên dịch lại. Góp ý sẽ hiện ở thẻ **Câu trả lời** của biểu mẫu; bấm biểu tượng Sheets để xem thành bảng.
