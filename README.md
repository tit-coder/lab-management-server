# Thiết Kế và Xây Dựng Web Server bằng C++ Kết Nối Cơ Sở Dữ Liệu MariaDB trên Ubuntu Linux Để Quản Lý Thiết Bị Phòng Máy

> **Đồ án cuối kỳ môn học:** Linux và Phần mềm mã nguồn mở  
> **Nền tảng thực thi:** Ubuntu Linux 24.04 LTS / 26.04 LTS  
> **Backend:** C++17, GCC/G++, Make, `cpp-httplib`, `libmariadb` (MariaDB Connector/C), `nlohmann/json`  
> **Cơ sở dữ liệu:** MariaDB Server 10.11+  
> **Frontend:** HTML5, CSS3, JavaScript thuần (Vanilla JS ES6+) với Fetch API  
> **Cổng dịch vụ:** 8080 (`http://localhost:8080`)  

---

## 1. Giới Thiệu & Mục Tiêu Đồ Án

Đồ án tập trung nghiên cứu, thiết kế và phát triển một hệ thống Web nội bộ hoàn chỉnh phục vụ công tác quản lý trang thiết bị phòng máy tại trường học hoặc trung tâm dữ liệu. 

Hệ thống được phát triển thuần túy dựa trên các công nghệ mã nguồn mở và kỹ thuật lập trình hệ thống trên môi trường Linux:
- **Backend C++ hiệu năng cao**: Tự xây dựng máy chủ Web Server bằng C++17 đa luồng với thư viện `cpp-httplib`, không sử dụng các framework cấp cao như Node.js, Express, PHP hay Python nhằm nắm vững bản chất tầng mạng (Socket, HTTP Protocol, Multi-threading) và quản lý bộ nhớ trực tiếp.
- **Tích hợp cơ sở dữ liệu cấp thấp**: Sử dụng trực tiếp **MariaDB Connector/C** (`libmariadb`) để kết nối, quản trị transaction nguyên tử, truy vấn an toàn và chống tấn công SQL Injection thông qua thoát chuỗi và ràng buộc khóa ngoại.
- **Chính sách bảo toàn dữ liệu kỹ thuật**: Áp dụng cơ chế khóa ngoại `ON DELETE RESTRICT` để bảo toàn lịch sử sự cố và bảo trì của thiết bị; ngăn chặn tuyệt đối việc xóa nhầm thiết bị khi đã có hồ sơ kỹ thuật liên kết.
- **Giao diện hiện đại, trực quan**: Xây dựng bảng điều khiển (Dashboard) thời gian thực, quản lý thiết bị, tìm kiếm, lọc theo phòng máy và điều phối quy trình xử lý sự cố bảo trì.

---

## 2. Công Nghệ Sử Dụng

| Thành phần | Công nghệ / Thư viện | Vai trò |
|---|---|---|
| **Hệ điều hành** | Ubuntu Linux 24.04 LTS / 26.04 LTS | Môi trường triển khai, biên dịch và chạy dịch vụ |
| **Ngôn ngữ Backend** | C++17 (G++ 13+) | Xử lý logic máy chủ, điều hướng HTTP và nghiệp vụ |
| **HTTP Web Server** | `cpp-httplib` (0.14+) | Lắng nghe kết nối TCP port 8080, phục vụ file tĩnh và REST API |
| **Cơ sở dữ liệu** | MariaDB Server 10.11+ | Lưu trữ quan hệ bền vững, toàn vẹn dữ liệu |
| **Database Driver** | MariaDB Connector/C (`libmariadb-dev`) | Thư viện C kết nối trực tiếp tầng socket tới MariaDB |
| **Định dạng dữ liệu** | `nlohmann/json` (v3.11.3) | Phân tích cú pháp và tuần tự hóa JSON chuẩn C++ |
| **Giao diện Web** | HTML5, CSS3, Vanilla JavaScript | Giao diện Responsive, AJAX không tải lại trang qua Fetch API |
| **Công cụ biên dịch** | GNU Make (Makefile) | Quản lý quy trình build, link và chạy tự động |
| **Kiểm thử tự động** | Bash Shell Script, cURL | Bộ 23 ca kiểm thử tự động toàn diện REST API |
| **Quản trị dịch vụ** | systemd service unit | Chạy nền tự động, phục hồi khi lỗi, chạy bằng non-root user |

---

## 3. Cấu Trúc Thư Mục Dự Án

Toàn bộ dự án được tổ chức gọn gàng, chuẩn mực theo mô hình phần mềm mã nguồn mở trên Linux:

```
/home/hoai-vy-nguyen/lab-device-server/
├── .env                         # File cấu hình môi trường cục bộ (được Git bỏ qua)
├── .env.example                 # File cấu hình mẫu an toàn để tham khảo
├── .gitignore                   # Danh sách file và thư mục không đưa lên Git
├── Makefile                     # Tự động hóa build, run, clean, test, check-deps
├── README.md                    # Tài liệu hướng dẫn toàn diện dự án (file này)
├── server                       # Binary thực thi C++ sau khi biên dịch
│
├── config/                      # Cấu hình dịch vụ hệ thống
│   └── lab-management.service   # File unit mẫu triển khai dịch vụ systemd
│
├── docs/                        # Tài liệu kỹ thuật chi tiết
│   ├── api.md                   # Đặc tả chi tiết toàn bộ các RESTful API
│   ├── architecture.md          # Sơ đồ kiến trúc 3 tầng, concurrency & state machine
│   ├── deployment.md            # Hướng dẫn chi tiết cài đặt và triển khai hệ thống
│   └── test_plan.md             # Kế hoạch và ma trận kết quả kiểm thử thực tế
│
├── include/                     # File tiêu đề (Header files C++)
│   ├── config.h                 # Phân tích file .env và nạp biến môi trường
│   ├── database.h               # Wrapper MariaDB Connector/C an toàn đa luồng
│   ├── device.h                 # Cấu trúc Device và nghiệp vụ quản lý thiết bị
│   ├── maintenance.h            # Cấu trúc MaintenanceLog và nghiệp vụ sự cố/bảo trì
│   └── nlohmann/                # Thư viện JSON cho C++ (Single Header v3.11.3)
│       └── json.hpp
│
├── public/                      # Tầng giao diện người dùng (Frontend Web)
│   ├── app.js                   # Logic JavaScript thuần, Fetch API, điều khiển DOM & Modal
│   ├── index.html               # Trang chủ Dashboard, bảng thiết bị và bảo trì
│   └── style.css                # Giao diện responsive, màu sắc hiện đại, CSS variables
│
├── sql/                         # Kịch bản cơ sở dữ liệu
│   └── schema.sql               # Tạo database, bảng, ràng buộc và dữ liệu mẫu an toàn
│
├── src/                         # Mã nguồn C++ chính
│   ├── database.cpp             # Cài đặt kết nối, truy vấn, transaction, mutex lock
│   ├── device.cpp               # Cài đặt CRUD thiết bị, thống kê, kiểm tra trùng lặp
│   ├── maintenance.cpp          # Cài đặt phiếu bảo trì, đồng bộ trạng thái nguyên tử
│   └── main.cpp                 # Điểm khởi nhập, routing REST API, CORS, signal handler
│
└── tests/                       # Kịch bản kiểm thử
    └── test_api.sh              # Bộ kiểm thử tự động 23 ca test bằng cURL và shell
```

---

## 4. Yêu Cầu Môi Trường & Cài Đặt Gói Phụ Thuộc

Hệ thống yêu cầu các gói phần mềm cơ bản trên Ubuntu Linux. Chạy lệnh sau để cài đặt đầy đủ:

```bash
sudo apt update
sudo apt install -y build-essential g++ make git curl \
    mariadb-server libmariadb-dev libcpp-httplib-dev
```

Kiểm tra trạng thái các công cụ và thư viện bằng lệnh:
```bash
make check-deps
```

---

## 5. Cấu Hình Cơ Sở Dữ Liệu MariaDB

### 5.1. Khởi tạo Database và Phân quyền người dùng riêng biệt
Ứng dụng sử dụng tài khoản chuyên dụng `labapp` được phân quyền tối thiểu (`SELECT, INSERT, UPDATE, DELETE`), không sử dụng tài khoản `root`.

Đăng nhập vào MariaDB:
```bash
sudo mariadb
```

Thực thi khối lệnh SQL sau:
```sql
CREATE DATABASE IF NOT EXISTS lab_management
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;

CREATE USER IF NOT EXISTS 'labapp'@'localhost' IDENTIFIED BY 'LabApp@2026Secure';

GRANT SELECT, INSERT, UPDATE, DELETE ON lab_management.* TO 'labapp'@'localhost';

FLUSH PRIVILEGES;
EXIT;
```

### 5.2. Chạy file khởi tạo cấu trúc và dữ liệu mẫu
File `sql/schema.sql` sử dụng cú pháp `CREATE TABLE IF NOT EXISTS` và `INSERT IGNORE`, đảm bảo an toàn tuyệt đối, có thể chạy lại bất cứ lúc nào mà không phá hủy hay làm trùng lặp dữ liệu hiện có:

```bash
cd /home/hoai-vy-nguyen/lab-device-server
mariadb -h localhost -u labapp -p'LabApp@2026Secure' lab_management < sql/schema.sql
```

Kiểm tra dữ liệu đã nạp thành công:
```bash
mariadb -h localhost -u labapp -p'LabApp@2026Secure' lab_management -e "SELECT * FROM devices; SELECT * FROM maintenance_logs;"
```

---

## 6. Cấu Hình Biến Môi Trường (.env)

Hệ thống tự động đọc cấu hình kết nối từ file `.env` tại thư mục gốc.

Tạo hoặc kiểm tra file `.env`:
```ini
DB_HOST=localhost
DB_PORT=3306
DB_NAME=lab_management
DB_USER=labapp
DB_PASSWORD=LabApp@2026Secure

SERVER_HOST=0.0.0.0
SERVER_PORT=8080
```
> **Bảo mật:** File `.env` chứa mật khẩu thật đã được liệt kê trong `.gitignore` và không bao giờ được commit lên GitHub.

---

## 7. Biên Dịch và Vận Hành Hệ Thống

Makefile được cung cấp đầy đủ các target tiêu chuẩn:

| Lệnh | Ý nghĩa |
|---|---|
| `make` hoặc `make build` | Biên dịch toàn bộ mã nguồn C++ thành file nhị phân `./server` |
| `make run` | Biên dịch và khởi chạy trực tiếp Web Server tại cổng 8080 |
| `make test` | Thực thi toàn bộ bộ kiểm thử tự động 23 ca test |
| `make clean` | Xóa binary thực thi và các file object `.o` |
| `make check-deps` | Kiểm tra sự hiện diện của các thư viện và công cụ trên máy |
| `make help` | Hiển thị bảng trợ giúp hướng dẫn các lệnh |

### 7.1. Biên dịch dự án:
```bash
make clean
make build
```

### 7.2. Khởi chạy máy chủ:
```bash
make run
```
Màn hình terminal sẽ xuất hiện thông báo:
```
========================================================
  HE THONG QUAN LY THIET BI PHONG MAY - LINUX & OPEN SOURCE
  C++ Web Server + MariaDB Connector/C
========================================================
[CONFIG] DB Host: localhost:3306
[CONFIG] DB Name: lab_management
[CONFIG] DB User: labapp
[CONFIG] Server:  http://0.0.0.0:8080
[SUCCESS] Ket noi MariaDB thanh cong!
[INFO] Server dang lang nghe tren port 8080...
[INFO] Truy cap giao dien website tai: http://localhost:8080
```

### 7.3. Truy cập giao diện Website:
Mở trình duyệt Web bất kỳ (Chrome, Firefox, Edge) và truy cập:
👉 **`http://localhost:8080`**

---

## 8. Danh Sách RESTful API Hoàn Chỉnh

| Phương thức | Đường dẫn (Endpoint) | Chức năng nghiệp vụ | Mã HTTP thành công | Mã HTTP lỗi |
|---|---|---|---|---|
| `GET` | `/` | Trả về giao diện website người dùng (HTML/CSS/JS) | `200 OK` | `404 Not Found` |
| `GET` | `/api/health` | Kiểm tra trạng thái máy chủ C++ và kết nối cơ sở dữ liệu MariaDB | `200 OK` | `503 Service Unavailable` |
| `GET` | `/api/stats` | Thống kê tổng số thiết bị, số lượng Active, Broken, Repairing | `200 OK` | `500 Internal Error` |
| `GET` | `/api/devices` | Lấy danh sách thiết bị (hỗ trợ lọc theo `room`, `status`, `search`) | `200 OK` | `500 Internal Error` |
| `GET` | `/api/devices/:id` | Xem thông tin chi tiết một thiết bị theo ID | `200 OK` | `404 Not Found` |
| `POST` | `/api/devices` | Thêm thiết bị mới (xác thực trường bắt buộc, kiểm tra trùng mã) | `201 Created` | `400 Bad Request`, `409 Conflict` |
| `PUT` | `/api/devices/:id` | Cập nhật thông tin thiết bị (tên, loại, phòng, trạng thái) | `200 OK` | `400 Bad Request`, `404`, `409` |
| `DELETE` | `/api/devices/:id` | Xóa thiết bị (áp dụng chính sách bảo toàn: chặn xóa nếu có phiếu bảo trì) | `200 OK` | `404 Not Found`, `409 Conflict` |
| `GET` | `/api/maintenance` | Lấy danh sách phiếu bảo trì (kèm mã, tên thiết bị, phòng máy) | `200 OK` | `500 Internal Error` |
| `POST` | `/api/maintenance` | Ghi nhận sự cố / tạo phiếu (tự động cập nhật thiết bị thành Broken/Repairing) | `201 Created` | `400 Bad Request`, `404 Not Found` |
| `PUT` | `/api/maintenance/:id` | Cập nhật tiến độ phiếu (khi Completed, tự chuyển thiết bị về Active) | `200 OK` | `400 Bad Request`, `404 Not Found` |

> *Xem chi tiết payload request và response mẫu tại:* [docs/api.md](file:///home/hoai-vy-nguyen/lab-device-server/docs/api.md).

---

## 9. Kiểm Thử Tự Động (Automated Testing)

Dự án tích hợp bộ kiểm thử tự động tại `tests/test_api.sh` bao gồm **23 ca kiểm thử thực tế**:
- Sức khỏe hệ thống và phản ứng khi MariaDB mất kết nối (`status: 503`).
- Kiểm tra tính toàn vẹn dữ liệu qua các lần khởi động lại máy chủ (Server Restart Persistence).
- Xác thực đầu vào: JSON sai định dạng, thiếu trường bắt buộc, ID tồn tại và không tồn tại.
- Kiểm tra xung đột dữ liệu: Thêm thiết bị trùng mã (`409 Conflict`).
- Đồng bộ trạng thái thiết bị và phiếu bảo trì theo Transaction nguyên tử.
- Thực thi chính sách bảo toàn lịch sử bảo trì (`ON DELETE RESTRICT` - `409 Conflict`).

Chạy kiểm thử:
```bash
make test
```

**Kết quả kiểm thử thực tế:**
```
======================================================================
 BÁO CÁO TỔNG KẾT KIỂM THỬ (TEST SUMMARY)
======================================================================
  - Tổng số test case thực thi: 23
  - Số test case thành công:   23
  - Số test case thất bại:     0

✔ TẤT CẢ 23 TEST CASES ĐÃ PASS 100%! HỆ THỐNG HOẠT ĐỘNG HOÀN HẢO!
```

---

## 10. Triển Khai Dịch Vụ Hệ Thống (systemd Service)

Để ứng dụng tự động chạy nền dưới dạng dịch vụ hệ điều hành Ubuntu:
1. Sao chép file unit mẫu:
   ```bash
   sudo cp config/lab-management.service /etc/systemd/system/
   sudo systemctl daemon-reload
   ```
2. Bật và khởi động dịch vụ:
   ```bash
   sudo systemctl enable --now lab-management
   ```
3. Kiểm tra trạng thái và theo dõi nhật ký:
   ```bash
   systemctl status lab-management
   journalctl -u lab-management -f
   ```

---

## 11. Xử Lý Các Sự Cố Thường Gặp (Troubleshooting)

1. **Lỗi `Cannot start server on port 8080`**:
   - Đang có tiến trình khác chiếm cổng 8080.
   - Dùng lệnh `ss -tulpn | grep 8080` để tìm và `pkill -f "./server"` để tắt tiến trình cũ.
2. **Lỗi `Access denied for user 'labapp'`**:
   - Kiểm tra mật khẩu trong `.env` đã trùng khớp với câu lệnh `CREATE USER` trong MariaDB chưa.
3. **Thanh trạng thái báo đỏ "Cơ sở dữ liệu MariaDB mất kết nối"**:
   - Kiểm tra dịch vụ MariaDB: `sudo systemctl status mariadb`.
   - Nếu MariaDB bị tắt, chạy: `sudo systemctl start mariadb`. Hệ thống C++ có tính năng tự động kết nối lại (`auto-reconnect`), sẽ tự khôi phục kết nối ngay lập tức mà không cần khởi động lại server.
