# 🧪 Hướng dẫn Kiểm thử (Testing Guide)

Thư mục này chứa kế hoạch kiểm thử và các kịch bản kiểm thử API tự động cho hệ thống **Lab Management Server**.

---

## 1. Yêu cầu trước khi chạy kiểm thử
- Đảm bảo MariaDB đang chạy và đã khởi tạo schema:
  ```bash
  mariadb -u root -p < sql/schema.sql
  ```
- Đảm bảo server backend đang chạy tại port 8080:
  ```bash
  make run
  ```

---

## 2. Kiểm thử tự động với script `run_api_tests.sh`
Mở một terminal mới trên Ubuntu và chạy lệnh:

```bash
# Cấp quyền thực thi (nếu cần)
chmod +x tests/run_api_tests.sh

# Chạy kiểm thử tự động
./tests/run_api_tests.sh 127.0.0.1 8080
```
Hoặc thông qua Makefile:
```bash
make test
```

Script sẽ tự động kiểm tra:
1. `GET /api/health` -> HTTP 200 (kiểm tra trạng thái server và kết nối CSDL).
2. `GET /api/stats` -> HTTP 200 (kiểm tra tổng hợp số liệu).
3. `GET /api/devices` -> HTTP 200 (kiểm tra danh sách và bộ lọc tìm kiếm).
4. `POST /api/devices` -> HTTP 201 (tạo thiết bị mới).
5. `POST /api/devices` (trùng mã thiết bị) -> HTTP 400 (kiểm tra ràng buộc UNIQUE).
6. `GET /api/maintenance` -> HTTP 200 (danh sách sự cố).
7. `POST /api/maintenance` -> HTTP 201 (tạo phiếu bảo trì).

---

## 3. Kiểm thử thủ công bằng cURL

### 3.1. Kiểm tra trạng thái hệ thống (Health Check)
```bash
curl -X GET http://localhost:8080/api/health
```

### 3.2. Lấy thống kê thiết bị
```bash
curl -X GET http://localhost:8080/api/stats
```

### 3.3. Lấy danh sách thiết bị có lọc theo phòng và từ khóa
```bash
curl -X GET "http://localhost:8080/api/devices?room=LAB101&search=OptiPlex"
```

### 3.4. Thêm thiết bị mới
```bash
curl -X POST http://localhost:8080/api/devices \
  -H "Content-Type: application/json" \
  -d '{
    "device_code": "PC-LAB101-99",
    "device_name": "Dell Precision 3650",
    "device_type": "PC",
    "room": "LAB101",
    "status": "Active"
  }'
```

### 3.5. Cập nhật thiết bị theo ID
```bash
curl -X PUT http://localhost:8080/api/devices/1 \
  -H "Content-Type: application/json" \
  -d '{
    "device_name": "Dell OptiPlex 7090 MT Upgraded",
    "device_type": "PC",
    "room": "LAB101",
    "status": "Repairing"
  }'
```

### 3.6. Xóa thiết bị
```bash
curl -X DELETE http://localhost:8080/api/devices/12
```

### 3.7. Báo cáo sự cố bảo trì
```bash
curl -X POST http://localhost:8080/api/maintenance \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": 1,
    "issue_description": "Nguồn phát ra tiếng kêu rè rè và chập chờn.",
    "status": "Pending"
  }'
```

### 3.8. Cập nhật trạng thái sự cố
```bash
curl -X PUT http://localhost:8080/api/maintenance/1 \
  -H "Content-Type: application/json" \
  -d '{
    "status": "InProgress"
  }'
```

---

## 4. Tài liệu chi tiết
Vui lòng tham khảo thêm tài liệu [docs/TEST_PLAN.md](../docs/TEST_PLAN.md) để xem ma trận test case hoàn chỉnh và tiêu chí nghiệm thu.
