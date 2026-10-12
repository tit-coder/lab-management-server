# 📑 Đặc Tả Kỹ Thuật RESTful API (API Specification)

- **Hệ thống:** Lab Management Server — Quản lý Thiết bị Phòng máy
- **Giao thức:** HTTP/1.1
- **Host mặc định:** `http://localhost:8080` (hoặc `http://<ubuntu-ip>:8080`)
- **Định dạng dữ liệu:** JSON (`Content-Type: application/json; charset=utf-8`)

---

## 1. Chuẩn hóa Mã trạng thái HTTP (HTTP Status Codes)

| Mã HTTP | Tên chuẩn | Ý nghĩa sử dụng trong hệ thống |
|---|---|---|
| **200** | OK | Yêu cầu xử lý thành công (GET, PUT, DELETE). |
| **201** | Created | Tạo mới tài nguyên thành công (POST device, POST maintenance). |
| **400** | Bad Request | Dữ liệu đầu vào không hợp lệ (thiếu trường bắt buộc, trùng mã, sai enum). |
| **404** | Not Found | Tài nguyên yêu cầu không tồn tại (ID không tìm thấy). |
| **500** | Internal Server Error | Lỗi nội bộ hệ thống hoặc mất kết nối MariaDB. |

---

## 2. Quy chuẩn Cấu trúc JSON Chung

### 2.1. Phản hồi thành công (Standard Success Response)
```json
{
  "success": true,
  "message": "Thông điệp mô tả (tùy chọn)",
  "data": { ... } // hoặc mảng [ { ... } ]
}
```

### 2.2. Phản hồi lỗi (Standard Error Response)
```json
{
  "success": false,
  "error": "Mô tả chi tiết nguyên nhân lỗi"
}
```

---

## 3. Danh sách Endpoints Chi tiết

### 3.1. Health Check
Kiểm tra trạng thái sẵn sàng của Web Server và kết nối đến MariaDB.

- **Method:** `GET`
- **Path:** `/api/health`
- **Request Headers:** Không yêu cầu
- **Response (200 OK):**
```json
{
  "status": "healthy",
  "database": true,
  "uptime_seconds": 125,
  "version": "1.0.0"
}
```

---

### 3.2. Lấy thống kê hệ thống (Dashboard Stats)
Trả về các số liệu tổng hợp phục vụ dashboard phòng máy.

- **Method:** `GET`
- **Path:** `/api/stats`
- **Response (200 OK):**
```json
{
  "success": true,
  "data": {
    "total_devices": 12,
    "active_devices": 8,
    "broken_devices": 2,
    "repairing_devices": 2,
    "pending_maintenance": 2
  }
}
```

---

### 3.3. Danh sách thiết bị (Có lọc & tìm kiếm)
Lấy toàn bộ danh sách thiết bị hoặc lọc theo điều kiện.

- **Method:** `GET`
- **Path:** `/api/devices`
- **Query Parameters:**
  - `search` (string, optional): Tìm kiếm tương đối theo `device_code` hoặc `device_name`.
  - `room` (string, optional): Lọc chính xác theo phòng (ví dụ: `LAB101`).
  - `status` (string, optional): Lọc theo trạng thái (`Active`, `Broken`, `Repairing`).
- **Response (200 OK):**
```json
{
  "success": true,
  "count": 1,
  "data": [
    {
      "id": 1,
      "device_code": "PC-LAB101-01",
      "device_name": "Dell OptiPlex 7090 MT",
      "device_type": "PC",
      "room": "LAB101",
      "status": "Active",
      "created_at": "2026-09-01 08:00:00"
    }
  ]
}
```

---

### 3.4. Chi tiết thiết bị theo ID
- **Method:** `GET`
- **Path:** `/api/devices/:id`
- **Path Parameters:**
  - `id` (int, required): ID định danh của thiết bị.
- **Response (200 OK):**
```json
{
  "success": true,
  "data": {
    "id": 1,
    "device_code": "PC-LAB101-01",
    "device_name": "Dell OptiPlex 7090 MT",
    "device_type": "PC",
    "room": "LAB101",
    "status": "Active",
    "created_at": "2026-09-01 08:00:00"
  }
}
```
- **Response (404 Not Found):**
```json
{
  "success": false,
  "error": "Device not found"
}
```

---

### 3.5. Thêm mới thiết bị
- **Method:** `POST`
- **Path:** `/api/devices`
- **Headers:** `Content-Type: application/json`
- **Request Body (JSON):**
```json
{
  "device_code": "PC-LAB101-05",
  "device_name": "Dell OptiPlex 7090 MT",
  "device_type": "PC",
  "room": "LAB101",
  "status": "Active"
}
```
- **Validation Rules:**
  - `device_code` (string, required): Mã duy nhất trong toàn hệ thống.
  - `device_name` (string, required): Không được để trống.
  - `device_type` (string, required): Ví dụ: `PC`, `Monitor`, `Keyboard`, `Mouse`, `Network`.
  - `room` (string, required): Tên phòng máy.
  - `status` (string, optional): Mặc định là `Active`. Chỉ nhận `Active`, `Broken`, `Repairing`.
- **Response (201 Created):**
```json
{
  "success": true,
  "message": "Device created successfully",
  "data": {
    "id": 13,
    "device_code": "PC-LAB101-05",
    "device_name": "Dell OptiPlex 7090 MT",
    "device_type": "PC",
    "room": "LAB101",
    "status": "Active",
    "created_at": ""
  }
}
```
- **Response (400 Bad Request - Trùng mã hoặc thiếu trường):**
```json
{
  "success": false,
  "error": "Failed to create device: Duplicate entry 'PC-LAB101-05' for key 'device_code'"
}
```

---

### 3.6. Cập nhật thông tin thiết bị
- **Method:** `PUT`
- **Path:** `/api/devices/:id`
- **Path Parameters:**
  - `id` (int, required): ID của thiết bị cần cập nhật.
- **Request Body (JSON):**
```json
{
  "device_name": "Dell OptiPlex 7090 MT (Upgraded RAM)",
  "device_type": "PC",
  "room": "LAB101",
  "status": "Repairing"
}
```
- **Validation Rules:**
  - `status`: Phải là một trong ba giá trị `Active`, `Broken`, `Repairing`.
  - `device_code` không được phép thay đổi để đảm bảo tính toàn vẹn.
- **Response (200 OK):**
```json
{
  "success": true,
  "message": "Device updated successfully",
  "data": {
    "id": 1,
    "device_code": "PC-LAB101-01",
    "device_name": "Dell OptiPlex 7090 MT (Upgraded RAM)",
    "device_type": "PC",
    "room": "LAB101",
    "status": "Repairing",
    "created_at": "2026-09-01 08:00:00"
  }
}
```

---

### 3.7. Xóa thiết bị
Xóa một thiết bị khỏi hệ thống (các bản ghi bảo trì liên quan trong `maintenance_logs` sẽ được tự động xóa theo ràng buộc khóa ngoại `CASCADE`).

- **Method:** `DELETE`
- **Path:** `/api/devices/:id`
- **Path Parameters:**
  - `id` (int, required): ID thiết bị cần xóa.
- **Response (200 OK):**
```json
{
  "success": true,
  "message": "Device deleted successfully",
  "deleted_id": 1
}
```
- **Response (404 Not Found):**
```json
{
  "success": false,
  "error": "Device not found with id 999"
}
```

---

### 3.8. Danh sách sự cố & bảo trì
- **Method:** `GET`
- **Path:** `/api/maintenance`
- **Query Parameters:**
  - `status` (string, optional): Lọc theo trạng thái (`Pending`, `InProgress`, `Completed`).
- **Response (200 OK):**
```json
{
  "success": true,
  "count": 1,
  "data": [
    {
      "id": 1,
      "device_id": 4,
      "device_code": "MS-LAB101-01",
      "issue_description": "Chuột bị liệt nút bấm chuột trái và đứt dây cáp USB ngầm.",
      "status": "Pending",
      "created_at": "2026-10-01 10:15:00",
      "completed_at": null
    }
  ]
}
```

---

### 3.9. Báo cáo sự cố mới (Tạo phiếu bảo trì)
- **Method:** `POST`
- **Path:** `/api/maintenance`
- **Headers:** `Content-Type: application/json`
- **Request Body (JSON):**
```json
{
  "device_id": 4,
  "issue_description": "Chuột bị kẹt con lăn và không nhận tín hiệu USB.",
  "status": "Pending"
}
```
- **Validation Rules:**
  - `device_id` (int, required): ID thiết bị hiện có trong MariaDB.
  - `issue_description` (string, required): Không được rỗng.
  - `status` (string, optional): Mặc định là `Pending`. Các giá trị hợp lệ: `Pending`, `InProgress`, `Completed`.
- **Response (201 Created):**
```json
{
  "success": true,
  "message": "Maintenance log created successfully",
  "data": {
    "id": 6,
    "device_id": 4,
    "device_code": "",
    "issue_description": "Chuột bị kẹt con lăn và không nhận tín hiệu USB.",
    "status": "Pending",
    "created_at": "",
    "completed_at": null
  }
}
```

---

### 3.10. Cập nhật tiến độ xử lý bảo trì
- **Method:** `PUT`
- **Path:** `/api/maintenance/:id`
- **Request Body (JSON):**
```json
{
  "status": "Completed"
}
```
- **Validation Rules:**
  - `status` (string, required): Nhận `Pending`, `InProgress`, hoặc `Completed`.
  - Khi chuyển sang `Completed`, server sẽ tự động gán `completed_at` bằng thời gian hiện tại.
- **Response (200 OK):**
```json
{
  "success": true,
  "message": "Maintenance log updated successfully",
  "updated_id": 1,
  "status": "Completed"
}
```
