## 📌 Mô tả Pull Request (PR)

- **Mục tiêu PR:** <!-- Tóm tắt ngắn gọn tính năng hoặc lỗi được xử lý -->
- **Nhánh nguồn (Source branch):** `feature/`
- **Nhánh đích (Target branch):** `main`
- **Thành viên phụ trách:** <!-- Ghi tên hoặc GitHub username -->

---

## 🔍 Các thay đổi chính

- [ ] Thêm mới / cập nhật API endpoint
- [ ] Thao tác / cấu trúc cơ sở dữ liệu MariaDB
- [ ] Giao diện người dùng (HTML/CSS/JS)
- [ ] Kiểm thử hoặc tài liệu triển khai

<!-- Liệt kê chi tiết các thay đổi -->
-

---

## 🧪 Kết quả kiểm thử (Testing)

- [ ] Đã chạy `make check-deps` trên môi trường Ubuntu
- [ ] Đã biên dịch thành công bằng `make` (không có warning hoặc lỗi cú pháp)
- [ ] Đã kiểm thử API với curl hoặc chạy thử trên trình duyệt (port 8080)
- [ ] Không có thông tin nhạy cảm, mật khẩu hoặc secrets trong code

---

## 📋 Checklist dành cho Reviewer (Team Leader / Reviewer)

- [ ] Code tuân thủ quy ước đặt tên và kiến trúc dự án
- [ ] Sử dụng Prepared Statements khi truy vấn MariaDB
- [ ] Định dạng JSON request/response đúng với [API_SPECIFICATION.md](docs/API_SPECIFICATION.md)
- [ ] Đã kiểm tra không phát sinh merge conflict với nhánh `main`
