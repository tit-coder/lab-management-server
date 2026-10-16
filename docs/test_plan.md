# Kế Hoạch & Báo Cáo Kiểm Thử (Test Plan & Report)

## 1. Mục tiêu kiểm thử
Xác minh toàn bộ các tính năng của hệ thống Web Server C++ kết nối MariaDB đáp ứng đầy đủ yêu cầu nghiệp vụ, tuân thủ chuẩn RESTful API, bảo toàn toàn vẹn dữ liệu, chống lỗi crash và hoạt động ổn định trên nền tảng Ubuntu Linux 24.04 LTS.

---

## 2. Ma Trận Ca Kiểm Thử Thực Tế (Test Matrix - 23/23 Passed)

| # | Nhóm chức năng | Phương thức & URL | Dữ liệu đầu vào (Payload) | Mã HTTP kỳ vọng | Tiêu chí kiểm tra | Kết quả thực tế | Trạng thái |
|---|---|---|---|---|---|---|---|
| **1** | Giám sát | `GET /api/health` | Không | `200 OK` | Chứa `"database":"connected"` | HTTP 200, database connected | **PASS** |
| **2** | Giao diện | `GET /` | Không | `200 OK` | Chứa HTML trang chủ | HTTP 200, đầy đủ HTML UI | **PASS** |
| **3** | Thống kê | `GET /api/stats` | Không | `200 OK` | Chứa `"total":`, `"active":` | HTTP 200, số liệu chính xác | **PASS** |
| **4** | Thiết bị | `GET /api/devices` | Không | `200 OK` | Chứa danh sách mảng JSON | HTTP 200, trả về đủ bản ghi | **PASS** |
| **5** | Thiết bị | `GET /api/devices/1` | Không | `200 OK` | Chứa `"device_code":"PC-001"` | HTTP 200, khớp mã PC-001 | **PASS** |
| **6** | Ngoại lệ | `GET /api/devices/99999` | Không | `404 Not Found` | Thông báo "Khong tim thay thiet bi" | HTTP 404, JSON lỗi rõ ràng | **PASS** |
| **7** | Ngoại lệ | `GET /api/nonexistent` | Không | `404 Not Found` | Thông báo "Endpoint khong ton tai" | HTTP 404 từ catch-all handler | **PASS** |
| **8** | Xác thực | `POST /api/devices` | Chuỗi text sai định dạng JSON | `400 Bad Request` | Thông báo "JSON khong hop le" | HTTP 400, không bị crash server | **PASS** |
| **9** | Xác thực | `POST /api/devices` | Thiếu trường bắt buộc `device_name` | `400 Bad Request` | Thông báo trường bắt buộc | HTTP 400, chặn dữ liệu rỗng | **PASS** |
| **10** | Thiết bị | `POST /api/devices` | Đầy đủ trường hợp lệ (`TEST-AUTO-...`) | `201 Created` | Sinh ID mới, trả về JSON thiết bị | HTTP 201, tạo thành công | **PASS** |
| **11** | Toàn vẹn | `POST /api/devices` | Mã trùng với thiết bị vừa tạo ở test #10 | `409 Conflict` | Báo "da ton tai trong he thong" | HTTP 409, ngăn trùng lặp mã | **PASS** |
| **12** | Thiết bị | `PUT /api/devices/:id` | Cập nhật phòng và tên thiết bị | `200 OK` | Thông báo cập nhật thành công | HTTP 200, dữ liệu MariaDB đổi | **PASS** |
| **13** | Ngoại lệ | `PUT /api/devices/99999` | Dữ liệu cập nhật cho ID không tồn tại | `404 Not Found` | Báo "Khong tim thay thiet bi" | HTTP 404 chính xác | **PASS** |
| **14** | Bảo trì | `POST /api/maintenance` | `{device_id, description, status: "Pending"}` | `201 Created` | Tạo phiếu thành công | HTTP 201, sinh ID phiếu mới | **PASS** |
| **15** | Nghiệp vụ | `GET /api/devices/:id` | ID thiết bị vừa gắn phiếu bảo trì | `200 OK` | Trạng thái thiết bị = `"Broken"` | HTTP 200, đồng bộ tự động | **PASS** |
| **16** | Bảo toàn | `DELETE /api/devices/:id` | Thao tác xóa thiết bị đang có phiếu bảo trì | `409 Conflict` | Chặn xóa do có lịch sử bảo trì | HTTP 409, chính sách bảo toàn | **PASS** |
| **17** | Bảo trì | `PUT /api/maintenance/:id` | `{status: "InProgress"}` | `200 OK` | Cập nhật tiến độ thành công | HTTP 200, cập nhật tiến độ | **PASS** |
| **18** | Nghiệp vụ | `GET /api/devices/:id` | ID thiết bị đang sửa | `200 OK` | Trạng thái thiết bị = `"Repairing"` | HTTP 200, đồng bộ sang Repairing | **PASS** |
| **19** | Bảo trì | `PUT /api/maintenance/:id` | `{status: "Completed"}` | `200 OK` | Ghi nhận `"completed_at"` | HTTP 200, ghi thời gian xong | **PASS** |
| **20** | Nghiệp vụ | `GET /api/devices/:id` | ID thiết bị sau khi hoàn thành phiếu | `200 OK` | Trạng thái thiết bị = `"Active"` | HTTP 200, hoàn nguyên Active | **PASS** |
| **21** | Thiết bị | `DELETE /api/devices/:id` | Thiết bị KHÔNG có lịch sử bảo trì | `200 OK` | Xóa thành công bản ghi | HTTP 200, xóa sạch khỏi DB | **PASS** |
| **22** | Khả năng chịu lỗi | `GET :8089/api/health` | Server chạy với sai mật khẩu MariaDB | `503 Service Unavailable` | Báo `"database":"disconnected"` | HTTP 503, không làm sập server | **PASS** |
| **23** | Bền vững dữ liệu | Khởi động lại Server C++ | Thêm bản ghi trước khi kill, query sau restart | `200 OK` | Bản ghi vẫn nguyên vẹn trong DB | HTTP 200, MariaDB lưu vĩnh viễn | **PASS** |

---

## 3. Cách thức thực thi bộ kiểm thử
Toàn bộ quy trình kiểm thử đã được tự động hóa thông qua target `make test`:
```bash
cd /home/hoai-vy-nguyen/lab-device-server
make test
```
Script sẽ tự động:
1. Phát hiện trạng thái máy chủ (tự khởi động nếu chưa chạy).
2. Chạy tuần tự 23 test case kiểm tra mọi khía cạnh: REST API, mã HTTP, xác thực đầu vào, nghiệp vụ Transaction, chính sách bảo toàn khóa ngoại, cơ chế dự phòng khi mất kết nối MariaDB và tính toàn vẹn dữ liệu qua các lần khởi động lại.
3. Xuất kết quả chi tiết từng ca và bảng tổng kết.
