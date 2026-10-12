# 🤝 Quy Định Đóng Góp & Hợp Tác Dự Án (Contribution Guidelines)

Chào mừng các thành viên tham gia phát triển dự án **Lab Management Server**! Để dự án vận hành chuyên nghiệp, hiệu quả và tránh xung đột mã nguồn, toàn bộ thành viên bắt buộc tuân thủ các quy tắc sau đây.

---

## 1. Phân chia Nhiệm vụ Nhóm 4 Người

Dự án được phân chia theo 4 khối chức năng độc lập:

### 👑 Team Leader (`tit-coder`):
- Khởi tạo và quản trị GitHub Repository, bảo vệ nhánh `main`.
- Thiết lập quy chuẩn mã nguồn, kiểm duyệt và merge các Pull Request (PR).
- Điều phối tiến độ chung và đóng gói sản phẩm cuối kỳ.

### 🚀 Thành viên 1 — Backend Web Server (`feature/server-api`):
- Xây dựng HTTP Web Server bằng thư viện `cpp-httplib`.
- Định tuyến RESTful API (10 endpoints theo chuẩn [docs/API_SPECIFICATION.md](docs/API_SPECIFICATION.md)).
- Chuẩn hóa `Makefile` tối ưu thời gian biên dịch và cờ liên kết thư viện.
- Xử lý CORS và hỗ trợ phục vụ tài nguyên tĩnh từ thư mục `public/`.

### 💾 Thành viên 2 — Cơ sở Dữ liệu & Tích hợp (`feature/database`):
- Hoàn thiện schema MariaDB `sql/schema.sql` và tài khoản ứng dụng `lab_user`.
- Cài đặt lớp `lab::Database` trong `include/database.h` và `src/database.cpp`.
- Áp dụng **Prepared Statements** chống SQL Injection cho toàn bộ truy vấn CRUD.
- Tối ưu hóa chỉ mục (Indexes) và xử lý ngoại lệ kết nối MariaDB.

### 🎨 Thành viên 3 — Giao diện Người dùng (`feature/frontend`):
- Phát triển giao diện Dashboard trực quan trong `public/index.html`.
- Xây dựng bảng điều khiển CSS hiện đại, responsive trong `public/style.css`.
- Viết JavaScript thuần (`public/app.js`) kết nối RESTful API: tìm kiếm theo thời gian thực (debounce), bộ lọc theo phòng, modal thêm/sửa thiết bị và ghi nhận sự cố.

### 🧪 Thành viên 4 — Kiểm thử & Triển khai (`feature/testing-deployment`):
- Viết ma trận kịch bản kiểm thử chi tiết trong `docs/TEST_PLAN.md`.
- Phát triển script tự động hóa kiểm thử cURL trong `tests/run_api_tests.sh`.
- Xây dựng file cấu hình dịch vụ chạy ngầm `systemd` trên Ubuntu Linux.
- Soạn thảo tài liệu hướng dẫn triển khai máy chủ và kiểm thử nghiệm thu.

---

## 2. Tiêu Chuẩn Viết Mã Nguồn (Code Style)

### C++ Backend:
- **Chuẩn C++:** C++17 trở lên.
- **Quy ước đặt tên:**
  - Class/Struct: `PascalCase` (ví dụ: `Database`, `Device`).
  - Hàm/Phương thức: `snake_case` (ví dụ: `get_device_by_id`, `connect`).
  - Biến cục bộ & tham số: `snake_case` (ví dụ: `device_code`, `error_message`).
  - Biến thành viên private: kết thúc bằng dấu gạch dưới `var_` (ví dụ: `conn_`).
  - Hằng số: `UPPER_SNAKE_CASE`.
- **An toàn bộ nhớ:** Sử dụng RAII, smart pointers hoặc `std::lock_guard` cho mutex; luôn giải phóng con trỏ MYSQL khi hoàn tất.

### Frontend (HTML/CSS/JS):
- **HTML:** Semantic HTML5, cấu trúc rõ ràng, id và class có nghĩa.
- **CSS:** BEM hoặc class mô tả công năng, sử dụng CSS Variables cho bảng màu.
- **JavaScript:** ES6+, viết hàm module hóa, dùng `async/await` khi gọi `fetch()`, escape HTML để ngăn ngừa XSS.

---

## 3. Quy Ước Commit & Tạo Pull Request

### Commit Message:
```text
<loại>: <mô tả ngắn bằng tiếng Việt hoặc tiếng Anh>

[Tùy chọn: giải thích chi tiết hơn]
```
Các loại commit hợp lệ:
- `feat`: Tính năng mới.
- `fix`: Sửa lỗi.
- `docs`: Cập nhật tài liệu.
- `refactor`: Tái cấu trúc mã nguồn.
- `test`: Bổ sung kiểm thử.

### Quy trình Pull Request:
1. Luôn kéo code mới nhất từ `main` về nhánh của mình trước khi mở PR.
2. Không mở PR khi mã nguồn chưa chạy thử thành công bằng `make`.
3. Điền đầy đủ thông tin vào mẫu PR Template trên GitHub.
4. Chờ ít nhất 1 phê duyệt (Approve) từ Leader trước khi merge.
