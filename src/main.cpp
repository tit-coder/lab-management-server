#include "httplib.h"
#include "database.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <csignal>
#include <regex>
#include <chrono>

namespace {
    lab::Database g_db;
    httplib::Server g_server;
    auto g_start_time = std::chrono::steady_clock::now();

    void signal_handler(int signum) {
        std::cout << "\n[Server] Received signal " << signum << ", shutting down gracefully..." << std::endl;
        g_server.stop();
        g_db.disconnect();
        std::exit(0);
    }

    // Load simple key-value pairs from .env if present
    void load_env_file(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return;

        std::string line;
        while (std::getline(file, line)) {
            // Trim leading whitespaces
            size_t start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos || line[start] == '#') continue;

            size_t eq = line.find('=', start);
            if (eq == std::string::npos) continue;

            std::string key = line.substr(start, eq - start);
            std::string val = line.substr(eq + 1);

            // Trim quotes if wrapped
            if (val.size() >= 2 && ((val.front() == '"' && val.back() == '"') || (val.front() == '\'' && val.back() == '\''))) {
                val = val.substr(1, val.size() - 2);
            }

            // Set environment variable if not already defined in system environment
            if (std::getenv(key.c_str()) == nullptr) {
                #if defined(_WIN32)
                _putenv_s(key.c_str(), val.c_str());
                #else
                setenv(key.c_str(), val.c_str(), 0);
                #endif
            }
        }
    }

    std::string get_env_var(const std::string& key, const std::string& default_val) {
        const char* val = std::getenv(key.c_str());
        return (val != nullptr && std::string(val).length() > 0) ? std::string(val) : default_val;
    }

    // Simple JSON value extractor helper functions
    std::string extract_json_string(const std::string& json, const std::string& key) {
        std::regex re("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
        std::smatch match;
        if (std::regex_search(json, match, re) && match.size() > 1) {
            return match[1].str();
        }
        return "";
    }

    int extract_json_int(const std::string& json, const std::string& key, int default_val = 0) {
        std::regex re("\"" + key + "\"\\s*:\\s*(-?\\d+)");
        std::smatch match;
        if (std::regex_search(json, match, re) && match.size() > 1) {
            return std::stoi(match[1].str());
        }
        return default_val;
    }

    void set_json_response(httplib::Response& res, int status_code, const std::string& body) {
        res.status = status_code;
        res.set_content(body, "application/json; charset=utf-8");
    }

    void set_error_response(httplib::Response& res, int status_code, const std::string& message) {
        std::ostringstream ss;
        ss << "{\"success\":false,\"error\":\"" << message << "\"}";
        set_json_response(res, status_code, ss.str());
    }
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "========================================================\n"
              << "   Lab Management Server - Equipment Monitoring System   \n"
              << "========================================================" << std::endl;

    // Load .env if present in current directory
    load_env_file(".env");

    // Read configuration
    std::string server_host = get_env_var("SERVER_HOST", "0.0.0.0");
    int server_port = std::stoi(get_env_var("SERVER_PORT", "8080"));

    std::string db_host = get_env_var("DB_HOST", "127.0.0.1");
    int db_port = std::stoi(get_env_var("DB_PORT", "3306"));
    std::string db_user = get_env_var("DB_USER", "lab_user");
    std::string db_pass = get_env_var("DB_PASSWORD", "lab_password_change_me");
    std::string db_name = get_env_var("DB_NAME", "lab_management");

    std::cout << "[Config] Server binding: " << server_host << ":" << server_port << std::endl;
    std::cout << "[Config] Database target: " << db_user << "@" << db_host << ":" << db_port << "/" << db_name << std::endl;

    // Connect to database
    bool db_ok = g_db.connect(db_host, db_port, db_user, db_pass, db_name);
    if (!db_ok) {
        std::cerr << "[Warning] Could not connect to MariaDB. API requests requiring database will return 500." << std::endl;
        std::cerr << "[Warning] Please ensure MariaDB is running and sql/schema.sql has been imported." << std::endl;
    }

    // Serve static files from ./public
    g_server.set_mount_point("/", "./public");

    // CORS middleware headers
    g_server.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        if (req.method == "OPTIONS") {
            res.status = 204;
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });

    // -------------------------------------------------------------------------
    // 1. Health check: GET /api/health
    // -------------------------------------------------------------------------
    g_server.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        auto now = std::chrono::steady_clock::now();
        auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - g_start_time).count();
        bool db_healthy = g_db.is_healthy();

        std::ostringstream ss;
        ss << "{"
           << "\"status\":\"" << (db_healthy ? "healthy" : "degraded") << "\","
           << "\"database\":" << (db_healthy ? "true" : "false") << ","
           << "\"uptime_seconds\":" << uptime << ","
           << "\"version\":\"1.0.0\""
           << "}";
        set_json_response(res, 200, ss.str());
    });

    // -------------------------------------------------------------------------
    // 2. Dashboard Statistics: GET /api/stats
    // -------------------------------------------------------------------------
    g_server.Get("/api/stats", [](const httplib::Request&, httplib::Response& res) {
        lab::SystemStats stats = g_db.get_stats();
        std::ostringstream ss;
        ss << "{\"success\":true,\"data\":" << stats.to_json() << "}";
        set_json_response(res, 200, ss.str());
    });

    // -------------------------------------------------------------------------
    // 3. List & Filter Devices: GET /api/devices
    // -------------------------------------------------------------------------
    g_server.Get("/api/devices", [](const httplib::Request& req, httplib::Response& res) {
        std::string search = req.has_param("search") ? req.get_param_value("search") : "";
        std::string room = req.has_param("room") ? req.get_param_value("room") : "";
        std::string status = req.has_param("status") ? req.get_param_value("status") : "";

        auto list = g_db.get_devices(search, room, status);

        std::ostringstream ss;
        ss << "{\"success\":true,\"count\":" << list.size() << ",\"data\":[";
        for (size_t i = 0; i < list.size(); ++i) {
            ss << list[i].to_json();
            if (i + 1 < list.size()) ss << ",";
        }
        ss << "]}";
        set_json_response(res, 200, ss.str());
    });

    // -------------------------------------------------------------------------
    // 4. Get Device by ID: GET /api/devices/:id
    // -------------------------------------------------------------------------
    g_server.Get(R"(/api/devices/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        auto dev = g_db.get_device_by_id(id);
        if (!dev.has_value()) {
            set_error_response(res, 404, "Device not found");
            return;
        }

        std::ostringstream ss;
        ss << "{\"success\":true,\"data\":" << dev->to_json() << "}";
        set_json_response(res, 200, ss.str());
    });

    // -------------------------------------------------------------------------
    // 5. Create Device: POST /api/devices
    // -------------------------------------------------------------------------
    g_server.Post("/api/devices", [](const httplib::Request& req, httplib::Response& res) {
        lab::Device dev;
        dev.device_code = extract_json_string(req.body, "device_code");
        dev.device_name = extract_json_string(req.body, "device_name");
        dev.device_type = extract_json_string(req.body, "device_type");
        dev.room = extract_json_string(req.body, "room");
        dev.status = extract_json_string(req.body, "status");
        if (dev.status.empty()) dev.status = "Active";

        // Validation
        if (dev.device_code.empty() || dev.device_name.empty() || dev.device_type.empty() || dev.room.empty()) {
            set_error_response(res, 400, "Missing required fields: device_code, device_name, device_type, room");
            return;
        }

        if (dev.status != "Active" && dev.status != "Broken" && dev.status != "Repairing") {
            set_error_response(res, 400, "Invalid status. Allowed values: Active, Broken, Repairing");
            return;
        }

        std::string err;
        int new_id = g_db.create_device(dev, err);
        if (new_id < 0) {
            set_error_response(res, 400, "Failed to create device: " + err);
            return;
        }

        dev.id = new_id;
        std::ostringstream ss;
        ss << "{\"success\":true,\"message\":\"Device created successfully\",\"data\":" << dev.to_json() << "}";
        set_json_response(res, 201, ss.str());
    });

    // -------------------------------------------------------------------------
    // 6. Update Device: PUT /api/devices/:id
    // -------------------------------------------------------------------------
    g_server.Put(R"(/api/devices/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        auto existing = g_db.get_device_by_id(id);
        if (!existing.has_value()) {
            set_error_response(res, 404, "Device not found");
            return;
        }

        lab::Device dev = existing.value();
        std::string name = extract_json_string(req.body, "device_name");
        std::string type = extract_json_string(req.body, "device_type");
        std::string room = extract_json_string(req.body, "room");
        std::string status = extract_json_string(req.body, "status");

        if (!name.empty()) dev.device_name = name;
        if (!type.empty()) dev.device_type = type;
        if (!room.empty()) dev.room = room;
        if (!status.empty()) {
            if (status != "Active" && status != "Broken" && status != "Repairing") {
                set_error_response(res, 400, "Invalid status. Allowed values: Active, Broken, Repairing");
                return;
            }
            dev.status = status;
        }

        std::string err;
        if (!g_db.update_device(id, dev, err)) {
            set_error_response(res, 500, "Failed to update device: " + err);
            return;
        }

        std::ostringstream ss;
        ss << "{\"success\":true,\"message\":\"Device updated successfully\",\"data\":" << dev.to_json() << "}";
        set_json_response(res, 200, ss.str());
    });

    // -------------------------------------------------------------------------
    // 7. Delete Device: DELETE /api/devices/:id
    // -------------------------------------------------------------------------
    g_server.Delete(R"(/api/devices/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        std::string err;
        if (!g_db.delete_device(id, err)) {
            set_error_response(res, 404, "Device not found or deletion failed: " + err);
            return;
        }

        std::ostringstream ss;
        ss << "{\"success\":true,\"message\":\"Device deleted successfully\",\"deleted_id\":" << id << "}";
        set_json_response(res, 200, ss.str());
    });

    // -------------------------------------------------------------------------
    // 8. List Maintenance Logs: GET /api/maintenance
    // -------------------------------------------------------------------------
    g_server.Get("/api/maintenance", [](const httplib::Request& req, httplib::Response& res) {
        std::string status = req.has_param("status") ? req.get_param_value("status") : "";
        auto list = g_db.get_maintenance_logs(status);

        std::ostringstream ss;
        ss << "{\"success\":true,\"count\":" << list.size() << ",\"data\":[";
        for (size_t i = 0; i < list.size(); ++i) {
            ss << list[i].to_json();
            if (i + 1 < list.size()) ss << ",";
        }
        ss << "]}";
        set_json_response(res, 200, ss.str());
    });

    // -------------------------------------------------------------------------
    // 9. Create Maintenance Log: POST /api/maintenance
    // -------------------------------------------------------------------------
    g_server.Post("/api/maintenance", [](const httplib::Request& req, httplib::Response& res) {
        lab::MaintenanceLog log;
        log.device_id = extract_json_int(req.body, "device_id", 0);
        log.issue_description = extract_json_string(req.body, "issue_description");
        log.status = extract_json_string(req.body, "status");
        if (log.status.empty()) log.status = "Pending";

        if (log.device_id <= 0 || log.issue_description.empty()) {
            set_error_response(res, 400, "Missing required fields: device_id, issue_description");
            return;
        }

        if (log.status != "Pending" && log.status != "InProgress" && log.status != "Completed") {
            set_error_response(res, 400, "Invalid status. Allowed values: Pending, InProgress, Completed");
            return;
        }

        std::string err;
        int new_id = g_db.create_maintenance_log(log, err);
        if (new_id < 0) {
            set_error_response(res, 400, "Failed to create maintenance log: " + err);
            return;
        }

        log.id = new_id;
        std::ostringstream ss;
        ss << "{\"success\":true,\"message\":\"Maintenance log created successfully\",\"data\":" << log.to_json() << "}";
        set_json_response(res, 201, ss.str());
    });

    // -------------------------------------------------------------------------
    // 10. Update Maintenance Log: PUT /api/maintenance/:id
    // -------------------------------------------------------------------------
    g_server.Put(R"(/api/maintenance/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        std::string status = extract_json_string(req.body, "status");

        if (status.empty()) {
            set_error_response(res, 400, "Missing required field: status");
            return;
        }

        if (status != "Pending" && status != "InProgress" && status != "Completed") {
            set_error_response(res, 400, "Invalid status. Allowed values: Pending, InProgress, Completed");
            return;
        }

        std::string err;
        if (!g_db.update_maintenance_log(id, status, err)) {
            set_error_response(res, 500, "Failed to update maintenance log: " + err);
            return;
        }

        std::ostringstream ss;
        ss << "{\"success\":true,\"message\":\"Maintenance log updated successfully\",\"updated_id\":" << id << ",\"status\":\"" << status << "\"}";
        set_json_response(res, 200, ss.str());
    });

    // Start Web Server
    std::cout << "[Server] Web server listening at http://" << server_host << ":" << server_port << std::endl;
    std::cout << "[Server] Static web UI hosted from ./public directory." << std::endl;
    std::cout << "[Server] Press Ctrl+C to terminate." << std::endl;

    if (!g_server.listen(server_host.c_str(), server_port)) {
        std::cerr << "[Error] Failed to bind/listen on " << server_host << ":" << server_port << std::endl;
        return 1;
    }

    return 0;
}
