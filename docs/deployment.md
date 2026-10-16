# Hướng Dẫn Triển Khai Hệ Thống (Deployment Guide)

Tài liệu hướng dẫn chi tiết quy trình triển khai máy chủ Web Server C++ và cơ sở dữ liệu MariaDB trên môi trường Ubuntu Linux (hỗ trợ đầy đủ Ubuntu 24.04 LTS và 26.04 LTS).

---

## 1. Yêu cầu hệ thống và cài đặt gói phụ thuộc

### 1.1. Các phần mềm bắt buộc
- **Hệ điều hành**: Ubuntu Linux 24.04 LTS / 26.04 LTS.
- **Trình biên dịch**: GCC/G++ phiên bản 11 trở lên (hỗ trợ chuẩn C++17).
- **Công cụ build**: GNU Make, Git, cURL.
- **Cơ sở dữ liệu**: MariaDB Server 10.11+.
- **Thư viện C++**:
  - `libmariadb-dev` (MariaDB Connector/C).
  - `libcpp-httplib-dev` (HTTP/HTTPS Server & Client cho C++).
  - `nlohmann-json3-dev` (hoặc sử dụng single-header đã tích hợp sẵn tại `include/nlohmann/json.hpp`).

### 1.2. Lệnh cài đặt gói (dành cho Quản trị viên hệ thống)
```bash
sudo apt update
sudo apt install -y build-essential g++ make git curl \
    mariadb-server libmariadb-dev libcpp-httplib-dev
```

---

## 2. Cấu hình Cơ sở dữ liệu MariaDB

### 2.1. Khởi động và kích hoạt MariaDB service
```bash
sudo systemctl enable --now mariadb
sudo systemctl status mariadb
```

### 2.2. Khởi tạo người dùng riêng biệt và phân quyền tối thiểu
Nhằm đảm bảo an toàn thông tin theo nguyên tắc đặc quyền tối thiểu (Least Privilege), ứng dụng sử dụng tài khoản chuyên dụng `labapp`, **tuyệt đối không sử dụng tài khoản `root`**:

Đăng nhập vào MariaDB CLI bằng quyền quản trị:
```bash
sudo mariadb
```

Thực thi các lệnh SQL sau:
```sql
-- Tạo database lab_management với bảng mã utf8mb4
CREATE DATABASE IF NOT EXISTS lab_management
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;

-- Tạo người dùng labapp với mật khẩu an toàn
CREATE USER IF NOT EXISTS 'labapp'@'localhost' IDENTIFIED BY 'LabApp@2026Secure';

-- Phân quyền thao tác dữ liệu cơ bản trên database
GRANT SELECT, INSERT, UPDATE, DELETE ON lab_management.* TO 'labapp'@'localhost';

-- Làm mới bảng phân quyền
FLUSH PRIVILEGES;
EXIT;
```

### 2.3. Khởi tạo cấu trúc bảng và nạp dữ liệu mẫu
File `sql/schema.sql` được thiết kế có tính **Idempotent** (an toàn khi chạy lại nhiều lần mà không làm mất hoặc nhân đôi dữ liệu hiện có nhờ `CREATE TABLE IF NOT EXISTS` và `INSERT IGNORE`):

```bash
cd /home/hoai-vy-nguyen/lab-device-server
mariadb -h localhost -u labapp -p'LabApp@2026Secure' lab_management < sql/schema.sql
```

Kiểm tra kết quả khởi tạo:
```bash
mariadb -h localhost -u labapp -p'LabApp@2026Secure' lab_management -e "SHOW TABLES; SELECT COUNT(*) FROM devices;"
```

---

## 3. Cấu hình Ứng Dụng (.env)

Hệ thống đọc thông tin kết nối từ file `.env` được Git bỏ qua (`.gitignore`) để bảo vệ bí mật thông tin:

Sao chép từ file mẫu `.env.example`:
```bash
cp .env.example .env
chmod 600 .env
```

Nội dung file `/home/hoai-vy-nguyen/lab-device-server/.env`:
```ini
DB_HOST=localhost
DB_PORT=3306
DB_NAME=lab_management
DB_USER=labapp
DB_PASSWORD=LabApp@2026Secure

SERVER_HOST=0.0.0.0
SERVER_PORT=8080
```

---

## 4. Biên dịch và Chạy Thử Nghiệm

### 4.1. Kiểm tra môi trường và thư viện
```bash
make check-deps
```

### 4.2. Biên dịch mã nguồn
```bash
make clean
make build
```
Quá trình biên dịch sẽ tạo ra file thực thi `./server`.

### 4.3. Chạy kiểm thử tự động
```bash
make test
```
Đảm bảo toàn bộ 23 test case đều trả về kết quả `PASS`.

### 4.4. Khởi chạy máy chủ trực tiếp
```bash
make run
```
Truy cập giao diện Web tại địa chỉ: `http://localhost:8080` (hoặc `http://<IP_MAY_CHU>:8080`).

---

## 5. Triển Khai Dịch Vụ Hệ Thống (systemd Service)

Để Web Server tự động khởi động cùng hệ thống Ubuntu, chạy ngầm dưới quyền người dùng thông thường (`hoai-vy-nguyen`), tự phục hồi khi gặp sự cố, hệ thống sử dụng file unit systemd mẫu tại `config/lab-management.service`.

### 5.1. Thiết lập file Service
Sao chép file service vào thư mục cấu hình của systemd:
```bash
sudo cp /home/hoai-vy-nguyen/lab-device-server/config/lab-management.service /etc/systemd/system/
sudo systemctl daemon-reload
```

### 5.2. Kích hoạt và khởi chạy dịch vụ
```bash
# Kích hoạt tự khởi động khi máy tính bật
sudo systemctl enable lab-management

# Bắt đầu chạy dịch vụ
sudo systemctl start lab-management
```

### 5.3. Giám sát và kiểm tra trạng thái
```bash
# Xem trạng thái hoạt động của tiến trình
systemctl status lab-management

# Xem log thời gian thực của ứng dụng qua journalctl
journalctl -u lab-management -f
```

### 5.4. Dừng hoặc khởi động lại dịch vụ
```bash
# Khởi động lại khi cập nhật code
sudo systemctl restart lab-management

# Dừng dịch vụ
sudo systemctl stop lab-management
```

---

## 6. Cấu hình Tường Lửa (UFW Firewall)

Nếu triển khai trong mạng nội bộ phòng máy để các máy trạm sinh viên hoặc giáo viên có thể truy cập:
```bash
sudo ufw allow 8080/tcp comment "Lab Management C++ Web Server"
sudo ufw status
```

---

## 7. Xử Lý Các Sự Cố Thường Gặp (Troubleshooting)

### 7.1. Cổng 8080 bị chiếm dụng (Port already in use)
- **Hiện tượng**: `Cannot start server on port 8080`.
- **Nguyên nhân**: Đang có một tiến trình `server` khác hoặc ứng dụng khác chiếm cổng 8080.
- **Xử lý**:
  ```bash
  ss -tulpn | grep 8080
  # Tìm PID tiến trình cũ và dừng nó:
  pkill -f "./server"
  ```

### 7.2. Lỗi kết nối MariaDB (Access denied for user 'labapp')
- **Hiện tượng**: Log báo lỗi `Access denied for user 'labapp'@'localhost'`.
- **Xử lý**:
  - Kiểm tra mật khẩu trong file `.env` đã khớp với mật khẩu được gán trong MariaDB chưa.
  - Chạy lệnh cập nhật mật khẩu trong MariaDB:
    ```sql
    ALTER USER 'labapp'@'localhost' IDENTIFIED BY 'LabApp@2026Secure';
    FLUSH PRIVILEGES;
    ```

### 7.3. Giao diện báo "Cơ sở dữ liệu MariaDB mất kết nối"
- **Hiện tượng**: Thanh trạng thái hiển thị màu đỏ trên website.
- **Xử lý**:
  - Kiểm tra MariaDB service: `systemctl status mariadb`.
  - Nếu service bị dừng, khởi động lại: `sudo systemctl restart mariadb`.
  - Nhờ cơ chế tự động kết nối lại (`reconnectIfNeeded` & `ping()`), Web Server C++ sẽ tự động khôi phục kết nối ngay khi MariaDB hoạt động trở lại mà không cần phải restart Web Server.
