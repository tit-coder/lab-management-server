#include "config.h"
#include "database.h"
#include "device.h"
#include "maintenance.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <csignal>
#include <memory>

static std::unique_ptr<httplib::Server> g_server;

void signalHandler(int signum) {
    std::cout << "\n[INFO] Nhan tin hieu dung (" << signum << "). Dang tat server...\n";
    if (g_server) {
        g_server->stop();
    }
}

// Helper set JSON response
void setJsonResponse(httplib::Response& res, int statusCode, const nlohmann::json& body) {
    res.status = statusCode;
    res.set_content(body.dump(), "application/json; charset=utf-8");
}

int main() {
    // 1. Nap cau hinh tu file .env va bien moi truong
    AppConfig config = AppConfig::load();

    std::cout << "========================================================\n";
    std::cout << "  HE THONG QUAN LY THIET BI PHONG MAY - LINUX & OPEN SOURCE\n";
    std::cout << "  C++ Web Server + MariaDB Connector/C\n";
    std::cout << "========================================================\n";
    std::cout << "[CONFIG] DB Host: " << config.dbHost << ":" << config.dbPort << "\n";
    std::cout << "[CONFIG] DB Name: " << config.dbName << "\n";
    std::cout << "[CONFIG] DB User: " << config.dbUser << "\n";
    std::cout << "[CONFIG] Server:  http://" << config.serverHost << ":" << config.serverPort << "\n";

    // 2. Ket noi co so du lieu MariaDB
    Database db;
    bool dbConnected = db.connect(
        config.dbHost,
        config.dbPort,
        config.dbUser,
        config.dbPassword,
        config.dbName
    );

    if (dbConnected) {
        std::cout << "[SUCCESS] Ket noi MariaDB thanh cong!\n";
    } else {
        std::cerr << "[WARNING] Ket noi MariaDB that bai: " << db.getError() << "\n";
        std::cerr << "[WARNING] Server van khoi dong, API se tra ve degraded health status.\n";
    }

    DeviceService deviceService(db);
    MaintenanceService maintenanceService(db);

    // 3. Khoi tao HTTP Server
    g_server = std::make_unique<httplib::Server>();

    // Dang ky xu ly tin hieu tat server
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Mount thu muc public chua giao dien HTML/CSS/JS
    g_server->set_mount_point("/", "./public");

    // Route trang chu
    g_server->Get("/", [](const httplib::Request&, httplib::Response& res) {
        std::ifstream file("./public/index.html");
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            res.set_content(buffer.str(), "text/html; charset=utf-8");
        } else {
            res.status = 404;
            res.set_content("<h1>404 - File index.html not found</h1>", "text/html; charset=utf-8");
        }
    });

    // CORS pre-flight
    g_server->Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        res.status = 204;
    });

    // Global middleware them CORS headers cho moi response
    g_server->set_post_routing_handler([](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
    });

    // ---------------------------------------------------------
    // API: GET /api/health
    // ---------------------------------------------------------
    g_server->Get("/api/health", [&db](const httplib::Request&, httplib::Response& res) {
        try {
            bool isDbOk = db.ping();
            nlohmann::json resp{
                {"success", isDbOk},
                {"status", isDbOk ? "ok" : "degraded"},
                {"server", "running"},
                {"database", isDbOk ? "connected" : "disconnected"}
            };
            if (!isDbOk) {
                resp["error"] = "Mat ket noi toi co so du lieu MariaDB";
                setJsonResponse(res, 503, resp);
            } else {
                setJsonResponse(res, 200, resp);
            }
        } catch (const std::exception& ex) {
            setJsonResponse(res, 500, {
                {"success", false},
                {"status", "error"},
                {"error", "Loi he thong noi bo"}
            });
        }
    });

    // ---------------------------------------------------------
    // API: GET /api/stats
    // ---------------------------------------------------------
    g_server->Get("/api/stats", [&deviceService](const httplib::Request&, httplib::Response& res) {
        try {
            DeviceStats stats;
            std::string errorMsg;
            if (!deviceService.getStats(stats, errorMsg)) {
                setJsonResponse(res, 500, {
                    {"success", false},
                    {"error", errorMsg.empty() ? "Loi truy van thong ke" : errorMsg}
                });
                return;
            }
            setJsonResponse(res, 200, {
                {"success", true},
                {"data", stats.toJson()}
            });
        } catch (const std::exception&) {
            setJsonResponse(res, 500, {
                {"success", false},
                {"error", "Loi he thong noi bo"}
            });
        }
    });

    // ---------------------------------------------------------
    // API: GET /api/devices
    // ---------------------------------------------------------
    g_server->Get("/api/devices", [&deviceService](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string room = req.has_param("room") ? req.get_param_value("room") : "";
            std::string status = req.has_param("status") ? req.get_param_value("status") : "";
            std::string search = req.has_param("search") ? req.get_param_value("search") : "";

            std::vector<Device> list;
            if (!deviceService.getAll(list, room, status, search)) {
                setJsonResponse(res, 500, {
                    {"success", false},
                    {"error", "Loi truy van danh sach thiet bi"}
                });
                return;
            }

            nlohmann::json arr = nlohmann::json::array();
            for (const auto& d : list) {
                arr.push_back(d.toJson());
            }

            setJsonResponse(res, 200, {
                {"success", true},
                {"count", arr.size()},
                {"data", arr}
            });
        } catch (const std::exception&) {
            setJsonResponse(res, 500, {
                {"success", false},
                {"error", "Loi he thong noi bo"}
            });
        }
    });

    // ---------------------------------------------------------
    // API: GET /api/devices/:id
    // ---------------------------------------------------------
    g_server->Get(R"(/api/devices/(\d+))", [&deviceService](const httplib::Request& req, httplib::Response& res) {
        try {
            int id = std::stoi(req.matches[1]);
            Device device;
            if (!deviceService.getById(id, device)) {
                setJsonResponse(res, 404, {
                    {"success", false},
                    {"error", "Khong tim thay thiet bi voi ID " + std::to_string(id)}
                });
                return;
            }
            setJsonResponse(res, 200, {
                {"success", true},
                {"data", device.toJson()}
            });
        } catch (const std::exception&) {
            setJsonResponse(res, 500, {
                {"success", false},
                {"error", "Loi he thong noi bo"}
            });
        }
    });

    // ---------------------------------------------------------
    // API: POST /api/devices
    // ---------------------------------------------------------
    g_server->Post("/api/devices", [&deviceService](const httplib::Request& req, httplib::Response& res) {
        try {
            nlohmann::json body;
            try {
                body = nlohmann::json::parse(req.body);
            } catch (...) {
                setJsonResponse(res, 400, {
                    {"success", false},
                    {"error", "Du lieu JSON khong hop le hoac sai dinh dang"}
                });
                return;
            }

            Device device;
            std::string parseError;
            if (!Device::fromJson(body, device, parseError)) {
                setJsonResponse(res, 400, {
                    {"success", false},
                    {"error", parseError}
                });
                return;
            }

            int newId = 0;
            std::string createError;
            bool isConflict = false;
            if (!deviceService.create(device, newId, createError, isConflict)) {
                if (isConflict) {
                    setJsonResponse(res, 409, {
                        {"success", false},
                        {"error", createError}
                    });
                } else {
                    setJsonResponse(res, 500, {
                        {"success", false},
                        {"error", createError}
                    });
                }
                return;
            }

            Device createdDevice;
            deviceService.getById(newId, createdDevice);

            setJsonResponse(res, 201, {
                {"success", true},
                {"message", "Them thiet bi thanh cong"},
                {"data", createdDevice.toJson()}
            });
        } catch (const std::exception&) {
            setJsonResponse(res, 500, {
                {"success", false},
                {"error", "Loi he thong noi bo"}
            });
        }
    });

    // ---------------------------------------------------------
    // API: PUT /api/devices/:id
    // ---------------------------------------------------------
    g_server->Put(R"(/api/devices/(\d+))", [&deviceService](const httplib::Request& req, httplib::Response& res) {
        try {
            int id = std::stoi(req.matches[1]);

            nlohmann::json body;
            try {
                body = nlohmann::json::parse(req.body);
            } catch (...) {
                setJsonResponse(res, 400, {
                    {"success", false},
                    {"error", "Du lieu JSON khong hop le hoac sai dinh dang"}
                });
                return;
            }

            Device device;
            std::string parseError;
            if (!Device::fromJson(body, device, parseError)) {
                setJsonResponse(res, 400, {
                    {"success", false},
                    {"error", parseError}
                });
                return;
            }

            std::string updateError;
            bool notFound = false;
            bool isConflict = false;
            if (!deviceService.update(id, device, updateError, notFound, isConflict)) {
                if (notFound) {
                    setJsonResponse(res, 404, {
                        {"success", false},
                        {"error", updateError}
                    });
                } else if (isConflict) {
                    setJsonResponse(res, 409, {
                        {"success", false},
                        {"error", updateError}
                    });
                } else {
                    setJsonResponse(res, 500, {
                        {"success", false},
                        {"error", updateError}
                    });
                }
                return;
            }

            Device updatedDevice;
            deviceService.getById(id, updatedDevice);

            setJsonResponse(res, 200, {
                {"success", true},
                {"message", "Cap nhat thiet bi thanh cong"},
                {"data", updatedDevice.toJson()}
            });
        } catch (const std::exception&) {
            setJsonResponse(res, 500, {
                {"success", false},
                {"error", "Loi he thong noi bo"}
            });
        }
    });

    // ---------------------------------------------------------
    // API: DELETE /api/devices/:id (Chinh sach bao toan lich su bao tri)
    // ---------------------------------------------------------
    g_server->Delete(R"(/api/devices/(\d+))", [&deviceService](const httplib::Request& req, httplib::Response& res) {
        try {
            int id = std::stoi(req.matches[1]);
            std::string removeError;
            bool notFound = false;
            bool hasConflict = false;

            if (!deviceService.remove(id, removeError, notFound, hasConflict)) {
                if (notFound) {
                    setJsonResponse(res, 404, {
                        {"success", false},
                        {"error", removeError}
                    });
                } else if (hasConflict) {
                    // Xung dot rang buoc khoa ngoai de bao toan lich su bao tri
                    setJsonResponse(res, 409, {
                        {"success", false},
                        {"error", removeError}
                    });
                } else {
                    setJsonResponse(res, 500, {
                        {"success", false},
                        {"error", removeError}
                    });
                }
                return;
            }

            setJsonResponse(res, 200, {
                {"success", true},
                {"message", "Xoa thiet bi thanh cong"}
            });
        } catch (const std::exception&) {
            setJsonResponse(res, 500, {
                {"success", false},
                {"error", "Loi he thong noi bo"}
            });
        }
    });

    // ---------------------------------------------------------
    // API: GET /api/maintenance
    // ---------------------------------------------------------
    g_server->Get("/api/maintenance", [&maintenanceService](const httplib::Request& req, httplib::Response& res) {
        try {
            int deviceId = 0;
            if (req.has_param("device_id")) {
                try { deviceId = std::stoi(req.get_param_value("device_id")); } catch (...) {}
            }
            std::string status = req.has_param("status") ? req.get_param_value("status") : "";

            std::vector<MaintenanceLog> logs;
            if (!maintenanceService.getAll(logs, deviceId, status)) {
                setJsonResponse(res, 500, {
                    {"success", false},
                    {"error", "Loi truy van danh sach phieu bao tri"}
                });
                return;
            }

            nlohmann::json arr = nlohmann::json::array();
            for (const auto& item : logs) {
                arr.push_back(item.toJson());
            }

            setJsonResponse(res, 200, {
                {"success", true},
                {"count", arr.size()},
                {"data", arr}
            });
        } catch (const std::exception&) {
            setJsonResponse(res, 500, {
                {"success", false},
                {"error", "Loi he thong noi bo"}
            });
        }
    });

    // ---------------------------------------------------------
    // API: POST /api/maintenance (Ghi nhan su co / Tao phieu)
    // ---------------------------------------------------------
    g_server->Post("/api/maintenance", [&maintenanceService](const httplib::Request& req, httplib::Response& res) {
        try {
            nlohmann::json body;
            try {
                body = nlohmann::json::parse(req.body);
            } catch (...) {
                setJsonResponse(res, 400, {
                    {"success", false},
                    {"error", "Du lieu JSON khong hop le hoac sai dinh dang"}
                });
                return;
            }

            if (!body.is_object() || !body.contains("device_id") || !body["device_id"].is_number_integer()) {
                setJsonResponse(res, 400, {
                    {"success", false},
                    {"error", "Truong 'device_id' (so nguyen) la bat buoc"}
                });
                return;
            }

            std::string description;
            if (body.contains("description") && body["description"].is_string()) {
                description = body["description"].get<std::string>();
            } else if (body.contains("issue_description") && body["issue_description"].is_string()) {
                description = body["issue_description"].get<std::string>();
            }

            if (description.empty()) {
                setJsonResponse(res, 400, {
                    {"success", false},
                    {"error", "Truong 'description' (hoac 'issue_description') la bat buoc va khong duoc de trong"}
                });
                return;
            }

            int deviceId = body["device_id"].get<int>();
            std::string status = "Pending";
            if (body.contains("status") && body["status"].is_string()) {
                status = body["status"].get<std::string>();
            }

            if (!MaintenanceService::isValidStatus(status)) {
                setJsonResponse(res, 400, {
                    {"success", false},
                    {"error", "Trang thai phieu khong hop le (chi chap nhan: Pending, InProgress, Completed)"}
                });
                return;
            }

            int newId = 0;
            std::string createError;
            bool deviceNotFound = false;

            if (!maintenanceService.create(deviceId, description, status, newId, createError, deviceNotFound)) {
                if (deviceNotFound) {
                    setJsonResponse(res, 404, {
                        {"success", false},
                        {"error", createError}
                    });
                } else {
                    setJsonResponse(res, 500, {
                        {"success", false},
                        {"error", createError}
                    });
                }
                return;
            }

            MaintenanceLog createdLog;
            maintenanceService.getById(newId, createdLog);

            setJsonResponse(res, 201, {
                {"success", true},
                {"message", "Ghi nhan su co va tao phieu bao tri thanh cong"},
                {"data", createdLog.toJson()}
            });
        } catch (const std::exception&) {
            setJsonResponse(res, 500, {
                {"success", false},
                {"error", "Loi he thong noi bo"}
            });
        }
    });

    // ---------------------------------------------------------
    // API: PUT /api/maintenance/:id (Cap nhat tien do/trang thai phieu)
    // ---------------------------------------------------------
    g_server->Put(R"(/api/maintenance/(\d+))", [&maintenanceService](const httplib::Request& req, httplib::Response& res) {
        try {
            int id = std::stoi(req.matches[1]);

            nlohmann::json body;
            try {
                body = nlohmann::json::parse(req.body);
            } catch (...) {
                setJsonResponse(res, 400, {
                    {"success", false},
                    {"error", "Du lieu JSON khong hop le hoac sai dinh dang"}
                });
                return;
            }

            std::string newStatus;
            if (body.contains("status")) {
                if (!body["status"].is_string()) {
                    setJsonResponse(res, 400, {
                        {"success", false},
                        {"error", "Truong 'status' phai la chuoi ky tu"}
                    });
                    return;
                }
                newStatus = body["status"].get<std::string>();
                if (!MaintenanceService::isValidStatus(newStatus)) {
                    setJsonResponse(res, 400, {
                        {"success", false},
                        {"error", "Trang thai phieu khong hop le (chi chap nhan: Pending, InProgress, Completed)"}
                    });
                    return;
                }
            }

            std::string newDescription;
            if (body.contains("description") && body["description"].is_string()) {
                newDescription = body["description"].get<std::string>();
            }

            if (newStatus.empty() && newDescription.empty()) {
                setJsonResponse(res, 400, {
                    {"success", false},
                    {"error", "Can cung cap it nhat truong 'status' hoac 'description' de cap nhat"}
                });
                return;
            }

            std::string updateError;
            bool notFound = false;

            if (!maintenanceService.update(id, newStatus, newDescription, updateError, notFound)) {
                if (notFound) {
                    setJsonResponse(res, 404, {
                        {"success", false},
                        {"error", updateError}
                    });
                } else {
                    setJsonResponse(res, 500, {
                        {"success", false},
                        {"error", updateError}
                    });
                }
                return;
            }

            MaintenanceLog updatedLog;
            maintenanceService.getById(id, updatedLog);

            setJsonResponse(res, 200, {
                {"success", true},
                {"message", "Cap nhat phieu bao tri thanh cong"},
                {"data", updatedLog.toJson()}
            });
        } catch (const std::exception&) {
            setJsonResponse(res, 500, {
                {"success", false},
                {"error", "Loi he thong noi bo"}
            });
        }
    });

    // ---------------------------------------------------------
    // Catch-all xu ly 404 cho API routes
    // ---------------------------------------------------------
    g_server->set_error_handler([](const httplib::Request& req, httplib::Response& res) {
        if (res.body.empty() && req.path.rfind("/api/", 0) == 0) {
            nlohmann::json err{
                {"success", false},
                {"error", "Endpoint khong ton tai hoac HTTP method khong duoc ho tro"}
            };
            res.set_content(err.dump(), "application/json; charset=utf-8");
        }
    });

    // 4. Khoi dong Web Server
    std::cout << "[INFO] Server dang lang nghe tren port " << config.serverPort << "...\n";
    std::cout << "[INFO] Truy cap giao dien website tai: http://localhost:" << config.serverPort << "\n";

    if (!g_server->listen(config.serverHost.c_str(), config.serverPort)) {
        std::cerr << "[ERROR] Khong the khoi dong Web Server tren port " << config.serverPort << "\n";
        return 1;
    }

    std::cout << "[INFO] Web Server da dung an toan.\n";
    return 0;
}
