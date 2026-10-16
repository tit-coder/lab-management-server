# Kiến Trúc Hệ Thống (System Architecture)

## 1. Giới thiệu tổng quan
Hệ thống **Quản Lý Thiết Bị Phòng Máy** được thiết kế dưới dạng ứng dụng Web 3 tầng (3-Tier Architecture) hoàn chỉnh, chạy trực tiếp trên hệ điều hành Ubuntu Linux. Dự án kết hợp hiệu năng cao, quản lý bộ nhớ trực tiếp của C++ hiện đại (C++17) với tính toàn vẹn dữ liệu của hệ quản trị cơ sở dữ liệu quan hệ MariaDB và giao diện người dùng trực quan trên nền tảng Web thuần (Vanilla HTML5/CSS3/JavaScript).

```
+-------------------------------------------------------------+
|                      Trình Duyệt Web                        |
|   (HTML5, CSS3, JavaScript thuần, Fetch API, Responsive)    |
+------------------------------+------------------------------+
                               | HTTP Requests (RESTful JSON)
                               v Port 8080
+-------------------------------------------------------------+
|              C++ Web Server (cpp-httplib)                   |
|   - Static File Server (public/index.html, style.css, js)   |
|   - RESTful API Router (GET, POST, PUT, DELETE, OPTIONS)    |
|   - Input Validator & JSON Parser (nlohmann/json)           |
|   - Business Services (DeviceService, MaintenanceService)   |
|   - Thread Synchronization & Connection Pool (std::mutex)   |
+------------------------------+------------------------------+
                               | MariaDB Connector/C (libmariadb)
                               v Port 3306 (TCP/Unix Socket)
+-------------------------------------------------------------+
|                 MariaDB Database Server                     |
|   - Database: lab_management (utf8mb4_unicode_ci)           |
|   - Tables: devices, maintenance_logs                       |
|   - Integrity: Foreign Keys, Unique Keys, ENUMs             |
|   - Policy: ON DELETE RESTRICT (Bảo toàn lịch sử bảo trì)   |
+-------------------------------------------------------------+
```

---

## 2. Các tầng thành phần

### 2.1. Tầng Giao Diện (Presentation Tier)
- **Công nghệ**: HTML5 ngữ nghĩa, CSS3 hiện đại (CSS Variables, Flexbox, Grid), JavaScript ES6+ thuần.
- **Phương thức giao tiếp**: Sử dụng `fetch()` API bất đồng bộ (Asynchronous AJAX), dữ liệu trao đổi 100% bằng JSON.
- **Chức năng chính**:
  - **Dashboard tổng quan**: Hiển thị số liệu thống kê thời gian thực (Tổng số, Đang hoạt động, Hỏng, Đang sửa chữa).
  - **Quản lý thiết bị**: Bảng danh sách thiết bị, tìm kiếm trực tiếp theo mã/tên, lọc theo phòng máy và trạng thái; modal thêm mới, chỉnh sửa và xác nhận xóa an toàn.
  - **Quản lý bảo trì**: Theo dõi lịch sử sự cố, tạo phiếu báo hỏng, cập nhật tiến độ (Pending -> InProgress -> Completed).
  - **Giám sát kết nối**: Tự động thăm dò tình trạng máy chủ và cơ sở dữ liệu qua `/api/health`, hiển thị cảnh báo trực quan khi xảy ra sự cố mạng hoặc database mất kết nối.

### 2.2. Tầng Máy Chủ Ứng Dụng (Application / Business Tier)
- **Công nghệ**: C++17, trình biên dịch GCC/G++, thư viện `cpp-httplib` (HTTP/1.1 multi-threaded server), `nlohmann/json` (xử lý JSON).
- **Module kiến trúc**:
  - `src/main.cpp`: Điểm khởi nhập (Entry point), nạp cấu hình `.env`, khởi tạo kết nối database, đăng ký định tuyến URL, CORS middleware, bộ xử lý tín hiệu hệ thống (SIGINT, SIGTERM) để tắt server an toàn.
  - `include/database.h` & `src/database.cpp`: Đóng gói MariaDB Connector/C (`MYSQL*`), cung cấp các hàm thực thi an toàn (`execute`, `query`), transaction (`beginTransaction`, `commit`, `rollback`), thoát ký tự chống SQL Injection (`escape`), tự động kết nối lại (`reconnectIfNeeded`) và đồng bộ đa luồng (`std::mutex`).
  - `include/device.h` & `src/device.cpp`: Cung cấp nghiệp vụ thiết bị: chuẩn hóa dữ liệu, kiểm tra tính duy nhất của mã thiết bị, chính sách bảo toàn lịch sử bảo trì khi xóa.
  - `include/maintenance.h` & `src/maintenance.cpp`: Quản lý vòng đời phiếu bảo trì và đồng bộ trạng thái thiết bị nguyên tử (Atomic Transactions).
  - `include/config.h`: Phân tích file cấu hình `.env` cục bộ kết hợp biến môi trường hệ thống.

### 2.3. Tầng Dữ Liệu (Data Tier)
- **Hệ quản trị**: MariaDB Server 10.11+
- **Database**: `lab_management`, bộ mã ký tự `utf8mb4` và collation `utf8mb4_unicode_ci`.
- **Tài khoản**: Người dùng chuyên dụng `labapp` được cấp quyền tối thiểu (Least Privilege: `SELECT, INSERT, UPDATE, DELETE`) trên database `lab_management`. Tuyệt đối không dùng quyền `root` trong mã nguồn ứng dụng.

---

## 3. Mô hình dữ liệu & Quy tắc ràng buộc toàn vẹn

### 3.1. Bảng `devices` (Thiết bị)
| Tên cột | Kiểu dữ liệu | Ràng buộc | Ý nghĩa |
|---|---|---|---|
| `id` | INT | PRIMARY KEY, AUTO_INCREMENT | Mã định danh duy nhất (khóa chính) |
| `device_code` | VARCHAR(30) | NOT NULL, UNIQUE | Mã thiết bị quản lý (VD: PC-001, MON-001) |
| `device_name` | VARCHAR(100) | NOT NULL | Tên thiết bị chi tiết |
| `device_type` | VARCHAR(50) | NOT NULL | Chủng loại (Computer, Monitor, Printer, Switch...) |
| `room` | VARCHAR(50) | NOT NULL | Vị trí phòng máy (Room A, Lab 101, Lab 102...) |
| `status` | ENUM('Active', 'Broken', 'Repairing') | NOT NULL, DEFAULT 'Active' | Trạng thái kỹ thuật |
| `created_at` | TIMESTAMP | DEFAULT CURRENT_TIMESTAMP | Thời điểm tạo bản ghi |
| `updated_at` | TIMESTAMP | ON UPDATE CURRENT_TIMESTAMP | Thời điểm sửa đổi gần nhất |

### 3.2. Bảng `maintenance_logs` (Nhật ký sự cố & bảo trì)
| Tên cột | Kiểu dữ liệu | Ràng buộc | Ý nghĩa |
|---|---|---|---|
| `id` | INT | PRIMARY KEY, AUTO_INCREMENT | Mã phiếu bảo trì |
| `device_id` | INT | NOT NULL, FOREIGN KEY | Khóa ngoại tham chiếu `devices(id)` |
| `description` | TEXT | NOT NULL | Mô tả chi tiết lỗi / sự cố |
| `status` | ENUM('Pending', 'InProgress', 'Completed') | NOT NULL, DEFAULT 'Pending' | Tiến độ xử lý phiếu |
| `created_at` | TIMESTAMP | DEFAULT CURRENT_TIMESTAMP | Thời điểm ghi nhận sự cố |
| `completed_at` | DATETIME | NULL | Thời điểm hoàn thành xử lý (NULL khi chưa xong) |

### 3.3. Ràng buộc Khóa Ngoại & Chính sách bảo toàn lịch sử bảo trì
```sql
CONSTRAINT fk_maintenance_device
    FOREIGN KEY (device_id)
    REFERENCES devices(id)
    ON DELETE RESTRICT
```
- **Ý nghĩa kỹ thuật**: Nhờ ràng buộc `ON DELETE RESTRICT`, MariaDB sẽ từ chối thao tác xóa thiết bị nếu thiết bị đó đã có bất kỳ phiếu bảo trì nào được lưu trữ.
- **Xử lý nghiệp vụ tại C++**: Server chủ động kiểm tra số lượng bản ghi bảo trì của thiết bị trước khi xóa. Nếu đã tồn tại dữ liệu bảo trì, API trả về HTTP Status `409 Conflict` cùng thông điệp giải thích rõ ràng cho người dùng, ngăn ngừa mất dấu vết lịch sử kỹ thuật của thiết bị.

---

## 4. Máy trạng thái (State Machine) & Quy tắc Transaction

Khi có biến động trong công tác bảo trì, trạng thái của phiếu và thiết bị liên kết được đồng bộ trong một **MariaDB Transaction nguyên tử**:

```
[Thiết bị: Active]
       |
       | Tạo phiếu sự cố (Pending)
       v
[Phiếu: Pending]    ---> Cập nhật thiết bị sang [Broken]
       |
       | Kỹ thuật viên tiếp nhận sửa chữa (InProgress)
       v
[Phiếu: InProgress] ---> Cập nhật thiết bị sang [Repairing]
       |
       | Sửa chữa hoàn tất (Completed)
       v
[Phiếu: Completed]  ---> Ghi nhận completed_at = NOW()
                         Kiểm tra xem còn phiếu nào (Pending/InProgress) khác không?
                         - Nếu KHÔNG: Chuyển thiết bị về [Active]
                         - Nếu CÒN: Giữ nguyên trạng thái sửa chữa.
```

Tất cả các bước ghi nhận nhật ký và đổi trạng thái thiết bị đều được bọc trong `START TRANSACTION` và gọi `COMMIT` khi thành công, hoặc `ROLLBACK` nếu có bất kỳ lỗi nào xảy ra.

---

## 5. Mô hình đa luồng và an toàn tương tranh (Concurrency & Thread Safety)
- `cpp-httplib` sử dụng một luồng chính lắng nghe các kết nối TCP và phân phối mỗi request HTTP vào một nhóm luồng làm việc (Thread Pool).
- Theo tài liệu chính thức của MariaDB Connector/C, một cấu trúc `MYSQL*` không an toàn khi truy cập đồng thời từ nhiều luồng (Not Thread-Safe for concurrent queries on the same handle).
- **Giải pháp**: Lớp `Database` cài đặt `std::mutex dbMutex`. Tất cả các tác vụ kết nối, thoát chuỗi, truy vấn dữ liệu và quản lý transaction đều được khóa bằng `std::lock_guard<std::mutex> lock(dbMutex);`, loại bỏ triệt để nguy cơ Race Condition và Crash server khi nhiều người dùng thao tác cùng lúc.
