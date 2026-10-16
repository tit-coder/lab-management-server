# Đặc Tả Giao Diện Lập Trình Ứng Dụng (RESTful API Specification)

Tài liệu mô tả chi tiết toàn bộ các điểm cuối (endpoints), phương thức HTTP, định dạng dữ liệu đầu vào và đầu ra của hệ thống Web Server C++.

- **Base URL**: `http://localhost:8080` (hoặc `http://<IP_MAY_CHU>:8080`)
- **Định dạng dữ liệu**: `application/json; charset=utf-8`
- **Mã phản hồi chuẩn**:
  - `200 OK`: Yêu cầu thực thi thành công.
  - `201 Created`: Tạo mới tài nguyên thành công.
  - `400 Bad Request`: Dữ liệu gửi lên không hợp lệ, thiếu trường bắt buộc hoặc sai định dạng JSON.
  - `404 Not Found`: Không tìm thấy tài nguyên theo ID hoặc endpoint không tồn tại.
  - `409 Conflict`: Xung đột dữ liệu (trùng mã duy nhất hoặc vi phạm chính sách bảo toàn khóa ngoại).
  - `500 Internal Server Error`: Lỗi xử lý nội bộ tại tầng C++ hoặc cơ sở dữ liệu.
  - `503 Service Unavailable`: Dịch vụ tạm thời không khả dụng (mất kết nối MariaDB).

---

## 1. Giám Sát & Sức Khỏe Hệ Thống

### 1.1. Kiểm tra tình trạng Server và Cơ sở dữ liệu
- **Endpoint**: `GET /api/health`
- **Mô tả**: Kiểm tra phản hồi của tiến trình C++ và ping kết nối tới MariaDB.
- **Headers**: Không yêu cầu
- **Phản hồi thành công (`200 OK`)**:
  ```json
  {
    "success": true,
    "status": "ok",
    "server": "running",
    "database": "connected"
  }
  ```
- **Phản hồi khi mất kết nối MariaDB (`503 Service Unavailable`)**:
  ```json
  {
    "success": false,
    "status": "degraded",
    "server": "running",
    "database": "disconnected",
    "error": "Mat ket noi toi co so du lieu MariaDB"
  }
  ```

---

## 2. Quản Lý Thiết Bị Phòng Máy (Devices API)

### 2.1. Thống kê thiết bị
- **Endpoint**: `GET /api/stats`
- **Mô tả**: Tổng hợp tổng số thiết bị và số lượng phân loại theo trạng thái kỹ thuật.
- **Phản hồi (`200 OK`)**:
  ```json
  {
    "success": true,
    "data": {
      "total": 4,
      "active": 2,
      "broken": 1,
      "repairing": 1
    }
  }
  ```

### 2.2. Lấy danh sách thiết bị
- **Endpoint**: `GET /api/devices`
- **Tham số truy vấn (Query Params)**:
  - `room` (string, tùy chọn): Lọc theo phòng máy (VD: `Room A`, `Lab 101`).
  - `status` (string, tùy chọn): Lọc theo trạng thái (`Active`, `Broken`, `Repairing`).
  - `search` (string, tùy chọn): Tìm kiếm gần đúng theo mã hoặc tên thiết bị.
- **Ví dụ**: `GET /api/devices?room=Room%20A&status=Active`
- **Phản hồi (`200 OK`)**:
  ```json
  {
    "success": true,
    "count": 2,
    "data": [
      {
        "id": 1,
        "device_code": "PC-001",
        "device_name": "Computer 01",
        "device_type": "Computer",
        "room": "Room A",
        "status": "Active",
        "created_at": "2026-10-13 08:54:24",
        "updated_at": "2026-10-13 08:54:24"
      }
    ]
  }
  ```

### 2.3. Xem chi tiết thiết bị theo ID
- **Endpoint**: `GET /api/devices/:id`
- **Ví dụ**: `GET /api/devices/1`
- **Phản hồi (`200 OK`)**:
  ```json
  {
    "success": true,
    "data": {
      "id": 1,
      "device_code": "PC-001",
      "device_name": "Computer 01",
      "device_type": "Computer",
      "room": "Room A",
      "status": "Active",
      "created_at": "2026-10-13 08:54:24",
      "updated_at": "2026-10-13 08:54:24"
    }
  }
  ```
- **Phản hồi khi không tìm thấy (`404 Not Found`)**:
  ```json
  {
    "success": false,
    "error": "Khong tim thay thiet bi voi ID 99999"
  }
  ```

### 2.4. Thêm thiết bị mới
- **Endpoint**: `POST /api/devices`
- **Headers**: `Content-Type: application/json`
- **Body mẫu**:
  ```json
  {
    "device_code": "PC-105",
    "device_name": "Dell OptiPlex 7090",
    "device_type": "Computer",
    "room": "Lab 101",
    "status": "Active"
  }
  ```
- **Ràng buộc kiểm tra**:
  - `device_code`: Bắt buộc, chuỗi không rỗng, tối đa 30 ký tự.
  - `device_name`: Bắt buộc, chuỗi không rỗng, tối đa 100 ký tự.
  - `device_type`: Bắt buộc, chuỗi không rỗng, tối đa 50 ký tự.
  - `room`: Bắt buộc, chuỗi không rỗng, tối đa 50 ký tự.
  - `status`: Tùy chọn (mặc định: `Active`). Chỉ chấp nhận: `Active`, `Broken`, `Repairing`.
- **Phản hồi thành công (`201 Created`)**:
  ```json
  {
    "success": true,
    "message": "Them thiet bi thanh cong",
    "data": {
      "id": 5,
      "device_code": "PC-105",
      "device_name": "Dell OptiPlex 7090",
      "device_type": "Computer",
      "room": "Lab 101",
      "status": "Active",
      "created_at": "2026-10-16 09:15:00",
      "updated_at": "2026-10-16 09:15:00"
    }
  }
  ```
- **Phản hồi khi trùng mã thiết bị (`409 Conflict`)**:
  ```json
  {
    "success": false,
    "error": "Ma thiet bi 'PC-105' da ton tai trong he thong"
  }
  ```
- **Phản hồi khi thiếu trường dữ liệu (`400 Bad Request`)**:
  ```json
  {
    "success": false,
    "error": "Truong 'device_name' la bat buoc va khong duoc de trong"
  }
  ```

### 2.5. Chỉnh sửa thông tin thiết bị
- **Endpoint**: `PUT /api/devices/:id`
- **Ví dụ**: `PUT /api/devices/5`
- **Headers**: `Content-Type: application/json`
- **Body mẫu**:
  ```json
  {
    "device_code": "PC-105",
    "device_name": "Dell OptiPlex 7090 Upgraded",
    "device_type": "Workstation",
    "room": "Lab 102",
    "status": "Active"
  }
  ```
- **Phản hồi thành công (`200 OK`)**:
  ```json
  {
    "success": true,
    "message": "Cap nhat thiet bi thanh cong",
    "data": {
      "id": 5,
      "device_code": "PC-105",
      "device_name": "Dell OptiPlex 7090 Upgraded",
      "device_type": "Workstation",
      "room": "Lab 102",
      "status": "Active",
      "created_at": "2026-10-16 09:15:00",
      "updated_at": "2026-10-16 09:16:30"
    }
  }
  ```

### 2.6. Xóa thiết bị (Áp dụng chính sách bảo toàn lịch sử)
- **Endpoint**: `DELETE /api/devices/:id`
- **Ví dụ**: `DELETE /api/devices/2`
- **Trường hợp thiết bị đã có lịch sử bảo trì (`409 Conflict`)**:
  ```json
  {
    "success": false,
    "error": "Khong the xoa thiet bi vi da co 1 phieu bao tri lien ket (chinh sach bao toan lich su bao tri)."
  }
  ```
- **Trường hợp xóa thành công (`200 OK`)**:
  ```json
  {
    "success": true,
    "message": "Xoa thiet bi thanh cong"
  }
  ```

---

## 3. Quản Lý Nhật Ký Bảo Trì & Sự Cố (Maintenance API)

### 3.1. Lấy danh sách phiếu bảo trì
- **Endpoint**: `GET /api/maintenance`
- **Tham số truy vấn (Query Params)**:
  - `device_id` (int, tùy chọn): Lọc theo ID thiết bị.
  - `status` (string, tùy chọn): Lọc theo tiến độ (`Pending`, `InProgress`, `Completed`).
- **Phản hồi (`200 OK`)**:
  ```json
  {
    "success": true,
    "count": 1,
    "data": [
      {
        "id": 1,
        "device_id": 2,
        "device_code": "PC-002",
        "device_name": "Computer 02",
        "room": "Room A",
        "description": "Keyboard is not working",
        "status": "Pending",
        "created_at": "2026-10-13 08:54:44",
        "completed_at": null
      }
    ]
  }
  ```

### 3.2. Ghi nhận sự cố / Tạo phiếu bảo trì mới
- **Endpoint**: `POST /api/maintenance`
- **Headers**: `Content-Type: application/json`
- **Body mẫu**:
  ```json
  {
    "device_id": 1,
    "description": "Hỏng nguồn máy tính, quạt kêu to",
    "status": "Pending"
  }
  ```
- **Tác động nghiệp vụ**:
  - Tạo một bản ghi mới trong bảng `maintenance_logs`.
  - Tự động đồng bộ trạng thái của thiết bị có ID tương ứng:
    - Nếu status phiếu là `Pending` -> Thiết bị chuyển thành `Broken`.
    - Nếu status phiếu là `InProgress` -> Thiết bị chuyển thành `Repairing`.
- **Phản hồi thành công (`201 Created`)**:
  ```json
  {
    "success": true,
    "message": "Ghi nhan su co va tao phieu bao tri thanh cong",
    "data": {
      "id": 2,
      "device_id": 1,
      "device_code": "PC-001",
      "device_name": "Computer 01",
      "room": "Room A",
      "description": "Hỏng nguồn máy tính, quạt kêu to",
      "status": "Pending",
      "created_at": "2026-10-16 09:18:20",
      "completed_at": null
    }
  }
  ```

### 3.3. Cập nhật tiến độ phiếu bảo trì
- **Endpoint**: `PUT /api/maintenance/:id`
- **Headers**: `Content-Type: application/json`
- **Body mẫu khi chuyển trạng thái sang Đang sửa**:
  ```json
  {
    "status": "InProgress",
    "description": "Đã nhận linh kiện thay thế, đang kiểm tra bo mạch"
  }
  ```
- **Body mẫu khi hoàn tất sửa chữa**:
  ```json
  {
    "status": "Completed"
  }
  ```
- **Tác động nghiệp vụ**:
  - Cập nhật bản ghi `maintenance_logs`.
  - Khi hoàn thành (`status: "Completed"`):
    - Tự động ghi nhận thời gian hoàn tất `completed_at = CURRENT_TIMESTAMP`.
    - Kiểm tra thiết bị có còn phiếu bảo trì nào khác chưa xong không. Nếu không còn phiếu nào chưa xong, tự động hoàn nguyên thiết bị về `Active`.
- **Phản hồi thành công (`200 OK`)**:
  ```json
  {
    "success": true,
    "message": "Cap nhat phieu bao tri thanh cong",
    "data": {
      "id": 2,
      "device_id": 1,
      "device_code": "PC-001",
      "device_name": "Computer 01",
      "room": "Room A",
      "description": "Đã thay nguồn mới, máy chạy ổn định",
      "status": "Completed",
      "created_at": "2026-10-16 09:18:20",
      "completed_at": "2026-10-16 09:22:10"
    }
  }
  ```
