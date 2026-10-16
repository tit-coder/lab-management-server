#!/usr/bin/env bash
# ====================================================================
# Test Suite: Web Server C++ Ket Noi MariaDB Quan Ly Thiet Bi Phong May
# ====================================================================

set -u

BASE_URL="http://localhost:8080"
SERVER_BIN="./server"

# ANSI Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

echo -e "${BLUE}======================================================================${NC}"
echo -e "${CYAN} BỘ KIỂM THỬ TỰ ĐỘNG RESTful API - WEB SERVER C++ & MARIADB${NC}"
echo -e "${BLUE}======================================================================${NC}"

run_test() {
    local test_num="$1"
    local test_name="$2"
    local method="$3"
    local url="$4"
    local payload="$5"
    local expected_status="$6"
    local check_pattern="$7"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    echo -e "\n${YELLOW}[Test #$test_num] $test_name${NC}"
    echo -e "  - Method/URL: ${CYAN}$method $url${NC}"
    if [ -n "$payload" ]; then
        echo -e "  - Payload:    ${NC}$payload${NC}"
    fi
    echo -e "  - Kỳ vọng:    HTTP Status = ${GREEN}$expected_status${NC}, Khớp: \"$check_pattern\""

    local response_file
    response_file=$(mktemp)

    local http_code
    if [ -n "$payload" ]; then
        http_code=$(curl -s -w "%{http_code}" -X "$method" "$url" \
            -H "Content-Type: application/json" \
            -d "$payload" \
            -o "$response_file")
    else
        http_code=$(curl -s -w "%{http_code}" -X "$method" "$url" \
            -o "$response_file")
    fi

    local body
    body=$(cat "$response_file")
    rm -f "$response_file"

    echo -e "  - Thực tế:    HTTP Status = $http_code"
    echo -e "  - Phản hồi:   $body"

    local status_ok=0
    local pattern_ok=0

    if [ "$http_code" -eq "$expected_status" ]; then
        status_ok=1
    fi

    if echo "$body" | grep -iq "$check_pattern"; then
        pattern_ok=1
    fi

    if [ $status_ok -eq 1 ] && [ $pattern_ok -eq 1 ]; then
        echo -e "  -> ${GREEN}✔ KẾT QUẢ: PASS${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        return 0
    else
        echo -e "  -> ${RED}✘ KẾT QUẢ: FAIL (Status OK: $status_ok, Pattern OK: $pattern_ok)${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi
}

# 1. Kiem tra xem server da chay chua
echo -e "\n${CYAN}Kiểm tra trạng thái máy chủ...${NC}"
if ! curl -s "$BASE_URL/api/health" > /dev/null 2>&1; then
    echo -e "[INFO] Server chưa chạy. Đang khởi động ./server..."
    make build
    $SERVER_BIN > /dev/null 2>&1 &
    sleep 2
else
    echo -e "[OK] Server đang hoạt động tại $BASE_URL"
fi

# ====================================================================
# DANH SÁCH CÁC TEST CASES
# ====================================================================

# Test 1: Health check
run_test "1" "Health Check & Database Connectivity" \
    "GET" "$BASE_URL/api/health" "" 200 '"database":"connected"'

# Test 2: Lay trang chu
run_test "2" "Serve Static Website Frontend (GET /)" \
    "GET" "$BASE_URL/" "" 200 "Quản Lý Thiết Bị Phòng Máy"

# Test 3: Thong ke thiet bi
run_test "3" "Lấy thống kê thiết bị (Stats)" \
    "GET" "$BASE_URL/api/stats" "" 200 '"total":'

# Test 4: Danh sach thiet bi
run_test "4" "Lấy danh sách thiết bị (GET /api/devices)" \
    "GET" "$BASE_URL/api/devices" "" 200 '"success":true'

# Test 5: Chi tiet thiet bi ton tai
run_test "5" "Chi tiết thiết bị ID=1 (PC-001)" \
    "GET" "$BASE_URL/api/devices/1" "" 200 '"device_code":"PC-001"'

# Test 6: Chi tiet thiet bi khong ton tai
run_test "6" "Chi tiết thiết bị không tồn tại ID=99999" \
    "GET" "$BASE_URL/api/devices/99999" "" 404 "Khong tim thay thiet bi"

# Test 7: Route khong ton tai
run_test "7" "Truy cập endpoint không tồn tại (404 API Not Found)" \
    "GET" "$BASE_URL/api/nonexistent_route" "" 404 "Endpoint khong ton tai"

# Test 8: Them thiet bi voi JSON loi dinh dang
run_test "8" "Thêm thiết bị với JSON sai định dạng (Malformed JSON)" \
    "POST" "$BASE_URL/api/devices" "this is not a valid json {" 400 "JSON khong hop le"

# Test 9: Them thiet bi thieu truong bat buoc
run_test "9" "Thêm thiết bị thiếu trường bắt buộc (Missing fields)" \
    "POST" "$BASE_URL/api/devices" '{"device_code":"INCOMPLETE-01"}' 400 "la bat buoc"

# Test 10: Them thiet bi hop le moi
TEMP_CODE="TEST-AUTO-$((RANDOM % 9000 + 1000))"
CREATE_PAYLOAD="{\"device_code\":\"$TEMP_CODE\",\"device_name\":\"Auto Test PC\",\"device_type\":\"Computer\",\"room\":\"Lab 102\",\"status\":\"Active\"}"
run_test "10" "Thêm thiết bị hợp lệ ($TEMP_CODE)" \
    "POST" "$BASE_URL/api/devices" "$CREATE_PAYLOAD" 201 "Them thiet bi thanh cong"

NEW_DEV_ID=$(curl -s "$BASE_URL/api/devices?search=$TEMP_CODE" | grep -o '"id":[0-9]*' | head -n1 | cut -d: -f2)
echo -e "  ${BLUE}ℹ ID thiết bị tạo mới:${NC} $NEW_DEV_ID"

# Test 11: Them thiet bi trung ma (Conflict 409)
run_test "11" "Thêm thiết bị với mã trùng lặp (Conflict 409)" \
    "POST" "$BASE_URL/api/devices" "$CREATE_PAYLOAD" 409 "da ton tai"

# Test 12: Cap nhat thiet bi hop le
UPDATE_PAYLOAD="{\"device_code\":\"$TEMP_CODE\",\"device_name\":\"Auto Test PC Updated\",\"device_type\":\"Workstation\",\"room\":\"Room C\",\"status\":\"Active\"}"
run_test "12" "Cập nhật thông tin thiết bị (PUT /api/devices/:id)" \
    "PUT" "$BASE_URL/api/devices/$NEW_DEV_ID" "$UPDATE_PAYLOAD" 200 "Cap nhat thiet bi thanh cong"

# Test 13: Cap nhat thiet bi khong ton tai
run_test "13" "Cập nhật thiết bị không tồn tại ID=99999" \
    "PUT" "$BASE_URL/api/devices/99999" "$UPDATE_PAYLOAD" 404 "Khong tim thay thiet bi"

# Test 14: Tao phieu bao tri / ghi nhan su co
MAINT_PAYLOAD="{\"device_id\":$NEW_DEV_ID,\"description\":\"Loi bo nguon may tinh\",\"status\":\"Pending\"}"
run_test "14" "Ghi nhận sự cố / tạo phiếu bảo trì (POST /api/maintenance)" \
    "POST" "$BASE_URL/api/maintenance" "$MAINT_PAYLOAD" 201 "tao phieu bao tri thanh cong"

# Test 15: Kiem tra dong bo trang thai thiet bi: thiet bi phai chuyen sang Broken
run_test "15" "Xác nhận trạng thái thiết bị tự động chuyển sang 'Broken'" \
    "GET" "$BASE_URL/api/devices/$NEW_DEV_ID" "" 200 '"status":"Broken"'

NEW_MAINT_ID=$(curl -s "$BASE_URL/api/maintenance?device_id=$NEW_DEV_ID" | grep -o '"id":[0-9]*' | head -n1 | cut -d: -f2)
echo -e "  ${BLUE}ℹ ID phiếu bảo trì tạo mới:${NC} $NEW_MAINT_ID"

# Test 16: Chinh sach bao toan lich su bao tri (Xoa bi tu choi do co rang buoc khoa ngoai)
run_test "16" "Xóa thiết bị có lịch sử bảo trì bị từ chối (Chính sách bảo toàn - 409)" \
    "DELETE" "$BASE_URL/api/devices/$NEW_DEV_ID" "" 409 "chinh sach bao toan lich su bao tri"

# Test 17: Cap nhat tien do phieu bao tri sang InProgress (Thiet bi chuyen Repairing)
run_test "17" "Cập nhật tiến độ phiếu sang 'InProgress'" \
    "PUT" "$BASE_URL/api/maintenance/$NEW_MAINT_ID" '{"status":"InProgress"}' 200 "Cap nhat phieu bao tri thanh cong"

run_test "18" "Xác nhận trạng thái thiết bị chuyển sang 'Repairing'" \
    "GET" "$BASE_URL/api/devices/$NEW_DEV_ID" "" 200 '"status":"Repairing"'

# Test 19: Hoan tat phieu bao tri (Thiet bi tro ve Active)
run_test "19" "Cập nhật hoàn tất phiếu bảo trì ('Completed')" \
    "PUT" "$BASE_URL/api/maintenance/$NEW_MAINT_ID" '{"status":"Completed"}' 200 '"completed_at":'

run_test "20" "Xác nhận thiết bị tự động hoàn nguyên về 'Active'" \
    "GET" "$BASE_URL/api/devices/$NEW_DEV_ID" "" 200 '"status":"Active"'

# Test 21: Tao thiet bi khong co lich su bao tri va xoa thanh cong
DISPOSABLE_CODE="DISP-$((RANDOM % 9000 + 1000))"
curl -s -X POST "$BASE_URL/api/devices" \
    -H "Content-Type: application/json" \
    -d "{\"device_code\":\"$DISPOSABLE_CODE\",\"device_name\":\"Disposable\",\"device_type\":\"Other\",\"room\":\"Room A\",\"status\":\"Active\"}" > /dev/null
DISP_ID=$(curl -s "$BASE_URL/api/devices?search=$DISPOSABLE_CODE" | grep -o '"id":[0-9]*' | head -n1 | cut -d: -f2)

run_test "21" "Xóa thiết bị không có liên kết bảo trì (DELETE thành công)" \
    "DELETE" "$BASE_URL/api/devices/$DISP_ID" "" 200 "Xoa thiet bi thanh cong"

# Don dep phieu va thiet bi test
mariadb -h localhost -P 3306 -u labapp -p'LabApp@2026Secure' lab_management \
    -e "DELETE FROM maintenance_logs WHERE id = $NEW_MAINT_ID; DELETE FROM devices WHERE id = $NEW_DEV_ID;" > /dev/null 2>&1

# Test 22: Kiem tra khi database mat ket noi (Degraded health status 503)
echo -e "\n${CYAN}Kiểm tra cơ chế ứng phó sự cố: Database mất kết nối...${NC}"
DB_PASSWORD=WrongPassword SERVER_PORT=8089 $SERVER_BIN > /dev/null 2>&1 &
DEGRADED_PID=$!
sleep 1.5

run_test "22" "Kiểm tra phản hồi khi mất kết nối MariaDB (Status 503 Service Unavailable)" \
    "GET" "http://localhost:8089/api/health" "" 503 "disconnected"

kill -9 $DEGRADED_PID > /dev/null 2>&1 || true

# Test 23: Kiem tra tinh toan ven du lieu sau khi restart server (Data Persistence across restarts)
echo -e "\n${CYAN}Kiểm tra tính bền vững của dữ liệu sau khi khởi động lại Server...${NC}"
PERSIST_CODE="PERSIST-$((RANDOM % 9000 + 1000))"
curl -s -X POST "$BASE_URL/api/devices" \
    -H "Content-Type: application/json" \
    -d "{\"device_code\":\"$PERSIST_CODE\",\"device_name\":\"Persistent Device\",\"device_type\":\"Storage\",\"room\":\"Server Room\",\"status\":\"Active\"}" > /dev/null

PERSIST_ID=$(curl -s "$BASE_URL/api/devices?search=$PERSIST_CODE" | grep -o '"id":[0-9]*' | head -n1 | cut -d: -f2)
echo -e "  - Đã thêm thiết bị kiểm tra: $PERSIST_CODE (ID: $PERSIST_ID)"

echo -e "  - Đang dừng Web Server và khởi động lại..."
pkill -f "./server" || true
sleep 1.5
$SERVER_BIN > /dev/null 2>&1 &
sleep 2

run_test "23" "Xác nhận dữ liệu vẫn tồn tại vẹn toàn trong MariaDB sau khi khởi động lại" \
    "GET" "$BASE_URL/api/devices/$PERSIST_ID" "" 200 "$PERSIST_CODE"

# Don dep ban ghi persistent
mariadb -h localhost -P 3306 -u labapp -p'LabApp@2026Secure' lab_management \
    -e "DELETE FROM devices WHERE id = $PERSIST_ID;" > /dev/null 2>&1

# ====================================================================
# TỔNG KẾT KẾT QUẢ KIỂM THỬ
# ====================================================================
echo -e "\n${BLUE}======================================================================${NC}"
echo -e "${CYAN} BÁO CÁO TỔNG KẾT KIỂM THỬ (TEST SUMMARY)${NC}"
echo -e "${BLUE}======================================================================${NC}"
echo -e "  - Tổng số test case thực thi: ${BLUE}$TOTAL_TESTS${NC}"
echo -e "  - Số test case thành công:   ${GREEN}$PASSED_TESTS${NC}"
echo -e "  - Số test case thất bại:     ${RED}$FAILED_TESTS${NC}"

if [ "$FAILED_TESTS" -eq 0 ]; then
    echo -e "\n${GREEN}✔ TẤT CẢ $TOTAL_TESTS TEST CASES ĐÃ PASS 100%! HỆ THỐNG HOẠT ĐỘNG HOÀN HẢO!${NC}\n"
    exit 0
else
    echo -e "\n${RED}✘ CÓ $FAILED_TESTS TEST CASE THẤT BẠI. VUI LÒNG KIỂM TRA LẠI!${NC}\n"
    exit 1
fi
