#!/usr/bin/env bash
# =============================================================================
# Lab Management Server - API Automated Test Suite (cURL-based)
# Usage: ./tests/run_api_tests.sh [HOST] [PORT]
# Example: ./tests/run_api_tests.sh 127.0.0.1 8080
# =============================================================================

HOST="${1:-127.0.0.1}"
PORT="${2:-8080}"
BASE_URL="http://${HOST}:${PORT}/api"

PASSED=0
FAILED=0

color_green="\033[0;32m"
color_red="\033[0;31m"
color_reset="\033[0m"

echo "=========================================================="
echo " Running Lab Management Server API Test Suite against: ${BASE_URL}"
echo "=========================================================="

assert_status() {
    local test_name="$1"
    local expected_code="$2"
    local actual_code="$3"

    if [ "$expected_code" -eq "$actual_code" ]; then
        echo -e " [${color_green}PASS${color_reset}] ${test_name} (HTTP ${actual_code})"
        ((PASSED++))
    else
        echo -e " [${color_red}FAIL${color_reset}] ${test_name} (Expected: ${expected_code}, Got: ${actual_code})"
        ((FAILED++))
    fi
}

# 1. Health check
echo -e "\n--- 1. Health Check Endpoint ---"
CODE=$(curl -s -o /dev/null -w "%{http_code}" "${BASE_URL}/health")
assert_status "GET /api/health" 200 "$CODE"

# 2. Statistics
echo -e "\n--- 2. System Statistics ---"
CODE=$(curl -s -o /dev/null -w "%{http_code}" "${BASE_URL}/stats")
assert_status "GET /api/stats" 200 "$CODE"

# 3. List Devices
echo -e "\n--- 3. Device Retrieval & Filtering ---"
CODE=$(curl -s -o /dev/null -w "%{http_code}" "${BASE_URL}/devices")
assert_status "GET /api/devices (all)" 200 "$CODE"

CODE=$(curl -s -o /dev/null -w "%{http_code}" "${BASE_URL}/devices?search=OptiPlex&room=LAB101")
assert_status "GET /api/devices (filtered)" 200 "$CODE"

# 4. Create New Device
echo -e "\n--- 4. Device Creation ---"
RANDOM_ID=$((1000 + RANDOM % 9000))
NEW_DEV_CODE="PC-TEST-${RANDOM_ID}"
CREATE_RESP=$(curl -s -w "\n%{http_code}" -X POST "${BASE_URL}/devices" \
    -H "Content-Type: application/json" \
    -d "{\"device_code\":\"${NEW_DEV_CODE}\",\"device_name\":\"Test Machine ${RANDOM_ID}\",\"device_type\":\"PC\",\"room\":\"LAB101\",\"status\":\"Active\"}")

CODE=$(echo "$CREATE_RESP" | tail -n1)
assert_status "POST /api/devices (valid device)" 201 "$CODE"

# 5. Prevent Duplicate Device Code
echo -e "\n--- 5. Duplicate Validation ---"
DUP_RESP=$(curl -s -w "\n%{http_code}" -X POST "${BASE_URL}/devices" \
    -H "Content-Type: application/json" \
    -d "{\"device_code\":\"${NEW_DEV_CODE}\",\"device_name\":\"Duplicate Test\",\"device_type\":\"PC\",\"room\":\"LAB101\"}")
CODE=$(echo "$DUP_RESP" | tail -n1)
if [ "$CODE" -eq 409 ] || [ "$CODE" -eq 400 ]; then
    assert_status "POST /api/devices (duplicate code check)" "$CODE" "$CODE"
else
    assert_status "POST /api/devices (duplicate code check)" 409 "$CODE"
fi

# 6. Maintenance Logs
echo -e "\n--- 6. Maintenance Workflow ---"
CODE=$(curl -s -o /dev/null -w "%{http_code}" "${BASE_URL}/maintenance")
assert_status "GET /api/maintenance" 200 "$CODE"

MAINT_RESP=$(curl -s -w "\n%{http_code}" -X POST "${BASE_URL}/maintenance" \
    -H "Content-Type: application/json" \
    -d "{\"device_id\":1,\"issue_description\":\"Test issue automated verification\",\"status\":\"Pending\"}")
CODE=$(echo "$MAINT_RESP" | tail -n1)
assert_status "POST /api/maintenance (report issue)" 201 "$CODE"

# Summary
echo -e "\n=========================================================="
echo " Test Suite Summary: ${PASSED} Passed, ${FAILED} Failed"
echo "=========================================================="

if [ "$FAILED" -eq 0 ]; then
    exit 0
else
    exit 1
fi
