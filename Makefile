# ====================================================================
# Makefile - Web Server C++ Ket Noi MariaDB Quan Ly Thiet Bi Phong May
# ====================================================================

CXX ?= g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude -I/usr/include/mariadb -I/usr/include/mariadb/mysql
LDFLAGS = -L/usr/lib/x86_64-linux-gnu -lcpp-httplib -lmariadb -lpthread

TARGET = server

SRCS = src/main.cpp \
       src/database.cpp \
       src/device.cpp \
       src/maintenance.cpp

OBJS = $(SRCS:.cpp=.o)

.PHONY: all build run test clean help check-deps

all: build

build: $(TARGET)

$(TARGET): $(OBJS)
	@echo "[LINK] Dang lien ket $(TARGET)..."
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "[SUCCESS] Bien dich thanh cong binary: ./$(TARGET)"

src/%.o: src/%.cpp
	@echo "[CXX] Bien dich $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: build
	@echo "[RUN] Khoi chay Web Server..."
	./$(TARGET)

test:
	@echo "[TEST] Chay bo kiem thu tu dong..."
	@bash tests/test_api.sh

clean:
	@echo "[CLEAN] Don dep file thuc thi va file object..."
	rm -f $(OBJS) $(TARGET)

check-deps:
	@echo "--- Kiem tra moi truong va thu vien phu thuoc ---"
	@which g++ > /dev/null && echo "[OK] G++: $$(g++ --version | head -n1)" || echo "[FAIL] G++ chua duoc cai dat"
	@which make > /dev/null && echo "[OK] Make: $$(make --version | head -n1)" || echo "[FAIL] Make chua duoc cai dat"
	@which mariadb > /dev/null && echo "[OK] MariaDB Client: $$(mariadb --version)" || echo "[FAIL] MariaDB chua duoc cai dat"
	@test -f /usr/include/mariadb/mysql.h && echo "[OK] MariaDB Connector/C headers tim thay" || echo "[FAIL] Thieu libmariadb-dev"
	@test -f /usr/include/httplib.h && echo "[OK] cpp-httplib tim thay" || echo "[FAIL] Thieu libcpp-httplib-dev"
	@test -f include/nlohmann/json.hpp && echo "[OK] nlohmann/json tim thay" || echo "[FAIL] Thieu nlohmann/json"

help:
	@echo "=========================================================="
	@echo "  Lenh su dung Makefile do an Quan Ly Thiet Bi Phong May"
	@echo "=========================================================="
	@echo "  make            - Mac dinh bien dich binary server"
	@echo "  make build      - Bien dich ma nguon C++ thanh ./server"
	@echo "  make run        - Bien dich va khoi chay server tai cong 8080"
	@echo "  make test       - Chay bo test API tu dong bang shell script"
	@echo "  make clean      - Xoa binary server va cac file .o"
	@echo "  make check-deps - Kiem tra cac goi phu thuoc can thiet tren may"
	@echo "  make help       - Hien thi huong dan nay"
