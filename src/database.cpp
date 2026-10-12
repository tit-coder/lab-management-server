#include "database.h"
#include <iostream>
#include <sstream>
#include <cstring>

namespace lab {

// Helper: Escape JSON string values to prevent broken JSON and injection
std::string Database::escape_json(const std::string& input) {
    std::ostringstream ss;
    for (char c : input) {
        switch (c) {
            case '"':  ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\b': ss << "\\b";  break;
            case '\f': ss << "\\f";  break;
            case '\n': ss << "\\n";  break;
            case '\r': ss << "\\r";  break;
            case '\t': ss << "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < ' ') {
                    ss << "\\u" << std::hex << (int)c;
                } else {
                    ss << c;
                }
        }
    }
    return ss.str();
}

std::string Device::to_json() const {
    std::ostringstream ss;
    ss << "{"
       << "\"id\":" << id << ","
       << "\"device_code\":\"" << Database::escape_json(device_code) << "\","
       << "\"device_name\":\"" << Database::escape_json(device_name) << "\","
       << "\"device_type\":\"" << Database::escape_json(device_type) << "\","
       << "\"room\":\"" << Database::escape_json(room) << "\","
       << "\"status\":\"" << Database::escape_json(status) << "\","
       << "\"created_at\":\"" << Database::escape_json(created_at) << "\""
       << "}";
    return ss.str();
}

std::string MaintenanceLog::to_json() const {
    std::ostringstream ss;
    ss << "{"
       << "\"id\":" << id << ","
       << "\"device_id\":" << device_id << ","
       << "\"device_code\":\"" << Database::escape_json(device_code) << "\","
       << "\"issue_description\":\"" << Database::escape_json(issue_description) << "\","
       << "\"status\":\"" << Database::escape_json(status) << "\","
       << "\"created_at\":\"" << Database::escape_json(created_at) << "\","
       << "\"completed_at\":" << (completed_at.empty() ? "null" : ("\"" + Database::escape_json(completed_at) + "\""))
       << "}";
    return ss.str();
}

std::string SystemStats::to_json() const {
    std::ostringstream ss;
    ss << "{"
       << "\"total_devices\":" << total_devices << ","
       << "\"active_devices\":" << active_devices << ","
       << "\"broken_devices\":" << broken_devices << ","
       << "\"repairing_devices\":" << repairing_devices << ","
       << "\"pending_maintenance\":" << pending_maintenance
       << "}";
    return ss.str();
}

Database::Database() : conn_(nullptr) {}

Database::~Database() {
    disconnect();
}

bool Database::connect(const std::string& host,
                       int port,
                       const std::string& user,
                       const std::string& password,
                       const std::string& database_name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    if (conn_) {
        mysql_close(conn_);
        conn_ = nullptr;
    }

    conn_ = mysql_init(nullptr);
    if (!conn_) {
        std::cerr << "[Database] mysql_init failed" << std::endl;
        return false;
    }

    // Set utf8mb4 encoding and connect timeout
    mysql_options(conn_, MYSQL_SET_CHARSET_NAME, "utf8mb4");
    unsigned int timeout_sec = 5;
    mysql_options(conn_, MYSQL_OPT_CONNECT_TIMEOUT, &timeout_sec);

    if (!mysql_real_connect(conn_,
                            host.c_str(),
                            user.c_str(),
                            password.c_str(),
                            database_name.c_str(),
                            port,
                            nullptr,
                            0)) {
        log_error("mysql_real_connect");
        mysql_close(conn_);
        conn_ = nullptr;
        return false;
    }

    std::cout << "[Database] Successfully connected to MariaDB: "
              << user << "@" << host << ":" << port << "/" << database_name << std::endl;
    return true;
}

void Database::disconnect() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (conn_) {
        mysql_close(conn_);
        conn_ = nullptr;
        std::cout << "[Database] Connection closed." << std::endl;
    }
}

bool Database::is_healthy() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (!conn_) return false;
    return (mysql_ping(conn_) == 0);
}

void Database::log_error(const std::string& context) {
    if (conn_) {
        std::cerr << "[Database Error] " << context << ": "
                  << mysql_error(conn_) << " (code: " << mysql_errno(conn_) << ")" << std::endl;
    } else {
        std::cerr << "[Database Error] " << context << ": connection is NULL" << std::endl;
    }
}

// -----------------------------------------------------------------------------
// Device CRUD with Prepared Statements
// -----------------------------------------------------------------------------

std::vector<Device> Database::get_devices(const std::string& search,
                                          const std::string& room,
                                          const std::string& status) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<Device> list;
    if (!conn_) return list;

    std::string sql = "SELECT id, device_code, device_name, device_type, room, status, created_at FROM devices WHERE 1=1";
    std::vector<std::string> params;

    if (!search.empty()) {
        sql += " AND (device_code LIKE ? OR device_name LIKE ?)";
        std::string pattern = "%" + search + "%";
        params.push_back(pattern);
        params.push_back(pattern);
    }
    if (!room.empty()) {
        sql += " AND room = ?";
        params.push_back(room);
    }
    if (!status.empty()) {
        sql += " AND status = ?";
        params.push_back(status);
    }
    sql += " ORDER BY id DESC";

    MYSQL_STMT* stmt = mysql_stmt_init(conn_);
    if (!stmt) return list;

    if (mysql_stmt_prepare(stmt, sql.c_str(), sql.length()) != 0) {
        log_error("mysql_stmt_prepare in get_devices");
        mysql_stmt_close(stmt);
        return list;
    }

    std::vector<MYSQL_BIND> binds(params.size());
    std::vector<unsigned long> str_lens(params.size());
    std::memset(binds.data(), 0, sizeof(MYSQL_BIND) * binds.size());

    for (size_t i = 0; i < params.size(); ++i) {
        str_lens[i] = params[i].length();
        binds[i].buffer_type = MYSQL_TYPE_STRING;
        binds[i].buffer = const_cast<char*>(params[i].c_str());
        binds[i].buffer_length = params[i].length();
        binds[i].length = &str_lens[i];
    }

    if (!params.empty()) {
        if (mysql_stmt_bind_param(stmt, binds.data()) != 0) {
            log_error("mysql_stmt_bind_param in get_devices");
            mysql_stmt_close(stmt);
            return list;
        }
    }

    if (mysql_stmt_execute(stmt) != 0) {
        log_error("mysql_stmt_execute in get_devices");
        mysql_stmt_close(stmt);
        return list;
    }

    // Output bindings
    int out_id = 0;
    char out_code[64] = {0};
    char out_name[128] = {0};
    char out_type[64] = {0};
    char out_room[64] = {0};
    char out_status[32] = {0};
    char out_created_at[64] = {0};
    unsigned long len_code, len_name, len_type, len_room, len_status, len_created;

    MYSQL_BIND result_binds[7];
    std::memset(result_binds, 0, sizeof(result_binds));

    result_binds[0].buffer_type = MYSQL_TYPE_LONG;
    result_binds[0].buffer = &out_id;

    result_binds[1].buffer_type = MYSQL_TYPE_STRING;
    result_binds[1].buffer = out_code;
    result_binds[1].buffer_length = sizeof(out_code);
    result_binds[1].length = &len_code;

    result_binds[2].buffer_type = MYSQL_TYPE_STRING;
    result_binds[2].buffer = out_name;
    result_binds[2].buffer_length = sizeof(out_name);
    result_binds[2].length = &len_name;

    result_binds[3].buffer_type = MYSQL_TYPE_STRING;
    result_binds[3].buffer = out_type;
    result_binds[3].buffer_length = sizeof(out_type);
    result_binds[3].length = &len_type;

    result_binds[4].buffer_type = MYSQL_TYPE_STRING;
    result_binds[4].buffer = out_room;
    result_binds[4].buffer_length = sizeof(out_room);
    result_binds[4].length = &len_room;

    result_binds[5].buffer_type = MYSQL_TYPE_STRING;
    result_binds[5].buffer = out_status;
    result_binds[5].buffer_length = sizeof(out_status);
    result_binds[5].length = &len_status;

    result_binds[6].buffer_type = MYSQL_TYPE_STRING;
    result_binds[6].buffer = out_created_at;
    result_binds[6].buffer_length = sizeof(out_created_at);
    result_binds[6].length = &len_created;

    if (mysql_stmt_bind_result(stmt, result_binds) == 0) {
        mysql_stmt_store_result(stmt);
        while (mysql_stmt_fetch(stmt) == 0) {
            Device d;
            d.id = out_id;
            d.device_code = std::string(out_code, len_code);
            d.device_name = std::string(out_name, len_name);
            d.device_type = std::string(out_type, len_type);
            d.room = std::string(out_room, len_room);
            d.status = std::string(out_status, len_status);
            d.created_at = std::string(out_created_at, len_created);
            list.push_back(d);
            std::memset(out_code, 0, sizeof(out_code));
            std::memset(out_name, 0, sizeof(out_name));
            std::memset(out_type, 0, sizeof(out_type));
            std::memset(out_room, 0, sizeof(out_room));
            std::memset(out_status, 0, sizeof(out_status));
            std::memset(out_created_at, 0, sizeof(out_created_at));
        }
    }

    mysql_stmt_close(stmt);
    return list;
}

std::optional<Device> Database::get_device_by_id(int id) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (!conn_) return std::nullopt;

    const char* sql = "SELECT id, device_code, device_name, device_type, room, status, created_at FROM devices WHERE id = ?";
    MYSQL_STMT* stmt = mysql_stmt_init(conn_);
    if (!stmt) return std::nullopt;

    if (mysql_stmt_prepare(stmt, sql, std::strlen(sql)) != 0) {
        log_error("mysql_stmt_prepare in get_device_by_id");
        mysql_stmt_close(stmt);
        return std::nullopt;
    }

    MYSQL_BIND param_bind;
    std::memset(&param_bind, 0, sizeof(param_bind));
    param_bind.buffer_type = MYSQL_TYPE_LONG;
    param_bind.buffer = &id;

    if (mysql_stmt_bind_param(stmt, &param_bind) != 0 || mysql_stmt_execute(stmt) != 0) {
        log_error("mysql_stmt_execute in get_device_by_id");
        mysql_stmt_close(stmt);
        return std::nullopt;
    }

    int out_id = 0;
    char out_code[64] = {0}, out_name[128] = {0}, out_type[64] = {0}, out_room[64] = {0}, out_status[32] = {0}, out_created_at[64] = {0};
    unsigned long len_code, len_name, len_type, len_room, len_status, len_created;

    MYSQL_BIND result_binds[7];
    std::memset(result_binds, 0, sizeof(result_binds));
    result_binds[0].buffer_type = MYSQL_TYPE_LONG; result_binds[0].buffer = &out_id;
    result_binds[1].buffer_type = MYSQL_TYPE_STRING; result_binds[1].buffer = out_code; result_binds[1].buffer_length = sizeof(out_code); result_binds[1].length = &len_code;
    result_binds[2].buffer_type = MYSQL_TYPE_STRING; result_binds[2].buffer = out_name; result_binds[2].buffer_length = sizeof(out_name); result_binds[2].length = &len_name;
    result_binds[3].buffer_type = MYSQL_TYPE_STRING; result_binds[3].buffer = out_type; result_binds[3].buffer_length = sizeof(out_type); result_binds[3].length = &len_type;
    result_binds[4].buffer_type = MYSQL_TYPE_STRING; result_binds[4].buffer = out_room; result_binds[4].buffer_length = sizeof(out_room); result_binds[4].length = &len_room;
    result_binds[5].buffer_type = MYSQL_TYPE_STRING; result_binds[5].buffer = out_status; result_binds[5].buffer_length = sizeof(out_status); result_binds[5].length = &len_status;
    result_binds[6].buffer_type = MYSQL_TYPE_STRING; result_binds[6].buffer = out_created_at; result_binds[6].buffer_length = sizeof(out_created_at); result_binds[6].length = &len_created;

    std::optional<Device> result = std::nullopt;
    if (mysql_stmt_bind_result(stmt, result_binds) == 0 && mysql_stmt_store_result(stmt) == 0) {
        if (mysql_stmt_fetch(stmt) == 0) {
            Device d;
            d.id = out_id;
            d.device_code = std::string(out_code, len_code);
            d.device_name = std::string(out_name, len_name);
            d.device_type = std::string(out_type, len_type);
            d.room = std::string(out_room, len_room);
            d.status = std::string(out_status, len_status);
            d.created_at = std::string(out_created_at, len_created);
            result = d;
        }
    }

    mysql_stmt_close(stmt);
    return result;
}

int Database::create_device(const Device& device, std::string& error_message) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (!conn_) {
        error_message = "Database connection not initialized";
        return -1;
    }

    const char* sql = "INSERT INTO devices (device_code, device_name, device_type, room, status) VALUES (?, ?, ?, ?, ?)";
    MYSQL_STMT* stmt = mysql_stmt_init(conn_);
    if (!stmt) {
        error_message = "Failed to allocate statement handle";
        return -1;
    }

    if (mysql_stmt_prepare(stmt, sql, std::strlen(sql)) != 0) {
        error_message = mysql_error(conn_);
        mysql_stmt_close(stmt);
        return -1;
    }

    MYSQL_BIND binds[5];
    std::memset(binds, 0, sizeof(binds));

    unsigned long l_code = device.device_code.length();
    unsigned long l_name = device.device_name.length();
    unsigned long l_type = device.device_type.length();
    unsigned long l_room = device.room.length();
    unsigned long l_status = device.status.length();

    binds[0].buffer_type = MYSQL_TYPE_STRING; binds[0].buffer = const_cast<char*>(device.device_code.c_str()); binds[0].length = &l_code;
    binds[1].buffer_type = MYSQL_TYPE_STRING; binds[1].buffer = const_cast<char*>(device.device_name.c_str()); binds[1].length = &l_name;
    binds[2].buffer_type = MYSQL_TYPE_STRING; binds[2].buffer = const_cast<char*>(device.device_type.c_str()); binds[2].length = &l_type;
    binds[3].buffer_type = MYSQL_TYPE_STRING; binds[3].buffer = const_cast<char*>(device.room.c_str()); binds[3].length = &l_room;
    binds[4].buffer_type = MYSQL_TYPE_STRING; binds[4].buffer = const_cast<char*>(device.status.c_str()); binds[4].length = &l_status;

    if (mysql_stmt_bind_param(stmt, binds) != 0 || mysql_stmt_execute(stmt) != 0) {
        error_message = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return -1;
    }

    int inserted_id = static_cast<int>(mysql_stmt_insert_id(stmt));
    mysql_stmt_close(stmt);
    return inserted_id;
}

bool Database::update_device(int id, const Device& device, std::string& error_message) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (!conn_) {
        error_message = "Database connection not initialized";
        return false;
    }

    const char* sql = "UPDATE devices SET device_name = ?, device_type = ?, room = ?, status = ? WHERE id = ?";
    MYSQL_STMT* stmt = mysql_stmt_init(conn_);
    if (!stmt) {
        error_message = "Failed to allocate statement";
        return false;
    }

    if (mysql_stmt_prepare(stmt, sql, std::strlen(sql)) != 0) {
        error_message = mysql_error(conn_);
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND binds[5];
    std::memset(binds, 0, sizeof(binds));

    unsigned long l_name = device.device_name.length();
    unsigned long l_type = device.device_type.length();
    unsigned long l_room = device.room.length();
    unsigned long l_status = device.status.length();

    binds[0].buffer_type = MYSQL_TYPE_STRING; binds[0].buffer = const_cast<char*>(device.device_name.c_str()); binds[0].length = &l_name;
    binds[1].buffer_type = MYSQL_TYPE_STRING; binds[1].buffer = const_cast<char*>(device.device_type.c_str()); binds[1].length = &l_type;
    binds[2].buffer_type = MYSQL_TYPE_STRING; binds[2].buffer = const_cast<char*>(device.room.c_str()); binds[2].length = &l_room;
    binds[3].buffer_type = MYSQL_TYPE_STRING; binds[3].buffer = const_cast<char*>(device.status.c_str()); binds[3].length = &l_status;
    binds[4].buffer_type = MYSQL_TYPE_LONG;   binds[4].buffer = &id;

    if (mysql_stmt_bind_param(stmt, binds) != 0 || mysql_stmt_execute(stmt) != 0) {
        error_message = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return false;
    }

    my_ulonglong affected = mysql_stmt_affected_rows(stmt);
    mysql_stmt_close(stmt);

    if (affected == 0) {
        // Could be device not found or fields unchanged
        // Let's verify if device actually exists
        return true; 
    }
    return true;
}

bool Database::delete_device(int id, std::string& error_message) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (!conn_) {
        error_message = "Database connection not initialized";
        return false;
    }

    const char* sql = "DELETE FROM devices WHERE id = ?";
    MYSQL_STMT* stmt = mysql_stmt_init(conn_);
    if (!stmt) {
        error_message = "Failed to allocate statement";
        return false;
    }

    if (mysql_stmt_prepare(stmt, sql, std::strlen(sql)) != 0) {
        error_message = mysql_error(conn_);
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND bind;
    std::memset(&bind, 0, sizeof(bind));
    bind.buffer_type = MYSQL_TYPE_LONG;
    bind.buffer = &id;

    if (mysql_stmt_bind_param(stmt, &bind) != 0 || mysql_stmt_execute(stmt) != 0) {
        error_message = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return false;
    }

    my_ulonglong affected = mysql_stmt_affected_rows(stmt);
    mysql_stmt_close(stmt);

    if (affected == 0) {
        error_message = "Device not found with id " + std::to_string(id);
        return false;
    }
    return true;
}

// -----------------------------------------------------------------------------
// Maintenance Logs CRUD
// -----------------------------------------------------------------------------

std::vector<MaintenanceLog> Database::get_maintenance_logs(const std::string& status) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<MaintenanceLog> list;
    if (!conn_) return list;

    std::string sql = "SELECT m.id, m.device_id, IFNULL(d.device_code, ''), m.issue_description, m.status, "
                      "m.created_at, IFNULL(m.completed_at, '') "
                      "FROM maintenance_logs m LEFT JOIN devices d ON m.device_id = d.id";
    if (!status.empty()) {
        sql += " WHERE m.status = ?";
    }
    sql += " ORDER BY m.id DESC";

    MYSQL_STMT* stmt = mysql_stmt_init(conn_);
    if (!stmt) return list;

    if (mysql_stmt_prepare(stmt, sql.c_str(), sql.length()) != 0) {
        log_error("mysql_stmt_prepare in get_maintenance_logs");
        mysql_stmt_close(stmt);
        return list;
    }

    unsigned long l_status = status.length();
    if (!status.empty()) {
        MYSQL_BIND bind;
        std::memset(&bind, 0, sizeof(bind));
        bind.buffer_type = MYSQL_TYPE_STRING;
        bind.buffer = const_cast<char*>(status.c_str());
        bind.length = &l_status;
        mysql_stmt_bind_param(stmt, &bind);
    }

    if (mysql_stmt_execute(stmt) != 0) {
        log_error("mysql_stmt_execute in get_maintenance_logs");
        mysql_stmt_close(stmt);
        return list;
    }

    int out_id = 0, out_dev_id = 0;
    char out_code[64] = {0}, out_desc[1024] = {0}, out_status[32] = {0}, out_created[64] = {0}, out_completed[64] = {0};
    unsigned long len_code, len_desc, len_status, len_created, len_completed;

    MYSQL_BIND result_binds[7];
    std::memset(result_binds, 0, sizeof(result_binds));
    result_binds[0].buffer_type = MYSQL_TYPE_LONG; result_binds[0].buffer = &out_id;
    result_binds[1].buffer_type = MYSQL_TYPE_LONG; result_binds[1].buffer = &out_dev_id;
    result_binds[2].buffer_type = MYSQL_TYPE_STRING; result_binds[2].buffer = out_code; result_binds[2].buffer_length = sizeof(out_code); result_binds[2].length = &len_code;
    result_binds[3].buffer_type = MYSQL_TYPE_STRING; result_binds[3].buffer = out_desc; result_binds[3].buffer_length = sizeof(out_desc); result_binds[3].length = &len_desc;
    result_binds[4].buffer_type = MYSQL_TYPE_STRING; result_binds[4].buffer = out_status; result_binds[4].buffer_length = sizeof(out_status); result_binds[4].length = &len_status;
    result_binds[5].buffer_type = MYSQL_TYPE_STRING; result_binds[5].buffer = out_created; result_binds[5].buffer_length = sizeof(out_created); result_binds[5].length = &len_created;
    result_binds[6].buffer_type = MYSQL_TYPE_STRING; result_binds[6].buffer = out_completed; result_binds[6].buffer_length = sizeof(out_completed); result_binds[6].length = &len_completed;

    if (mysql_stmt_bind_result(stmt, result_binds) == 0 && mysql_stmt_store_result(stmt) == 0) {
        while (mysql_stmt_fetch(stmt) == 0) {
            MaintenanceLog m;
            m.id = out_id;
            m.device_id = out_dev_id;
            m.device_code = std::string(out_code, len_code);
            m.issue_description = std::string(out_desc, len_desc);
            m.status = std::string(out_status, len_status);
            m.created_at = std::string(out_created, len_created);
            m.completed_at = std::string(out_completed, len_completed);
            list.push_back(m);
            std::memset(out_code, 0, sizeof(out_code));
            std::memset(out_desc, 0, sizeof(out_desc));
            std::memset(out_status, 0, sizeof(out_status));
            std::memset(out_created, 0, sizeof(out_created));
            std::memset(out_completed, 0, sizeof(out_completed));
        }
    }

    mysql_stmt_close(stmt);
    return list;
}

int Database::create_maintenance_log(const MaintenanceLog& log, std::string& error_message) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (!conn_) {
        error_message = "Database connection not initialized";
        return -1;
    }

    const char* sql = "INSERT INTO maintenance_logs (device_id, issue_description, status) VALUES (?, ?, ?)";
    MYSQL_STMT* stmt = mysql_stmt_init(conn_);
    if (!stmt) {
        error_message = "Failed to allocate statement handle";
        return -1;
    }

    if (mysql_stmt_prepare(stmt, sql, std::strlen(sql)) != 0) {
        error_message = mysql_error(conn_);
        mysql_stmt_close(stmt);
        return -1;
    }

    MYSQL_BIND binds[3];
    std::memset(binds, 0, sizeof(binds));

    int dev_id = log.device_id;
    unsigned long l_desc = log.issue_description.length();
    unsigned long l_status = log.status.length();

    binds[0].buffer_type = MYSQL_TYPE_LONG;   binds[0].buffer = &dev_id;
    binds[1].buffer_type = MYSQL_TYPE_STRING; binds[1].buffer = const_cast<char*>(log.issue_description.c_str()); binds[1].length = &l_desc;
    binds[2].buffer_type = MYSQL_TYPE_STRING; binds[2].buffer = const_cast<char*>(log.status.c_str()); binds[2].length = &l_status;

    if (mysql_stmt_bind_param(stmt, binds) != 0 || mysql_stmt_execute(stmt) != 0) {
        error_message = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return -1;
    }

    int inserted_id = static_cast<int>(mysql_stmt_insert_id(stmt));
    mysql_stmt_close(stmt);
    return inserted_id;
}

bool Database::update_maintenance_log(int id, const std::string& status, std::string& error_message) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (!conn_) {
        error_message = "Database connection not initialized";
        return false;
    }

    const char* sql = "UPDATE maintenance_logs SET status = ?, "
                      "completed_at = CASE WHEN ? = 'Completed' THEN CURRENT_TIMESTAMP ELSE NULL END "
                      "WHERE id = ?";
    MYSQL_STMT* stmt = mysql_stmt_init(conn_);
    if (!stmt) {
        error_message = "Failed to allocate statement handle";
        return false;
    }

    if (mysql_stmt_prepare(stmt, sql, std::strlen(sql)) != 0) {
        error_message = mysql_error(conn_);
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND binds[3];
    std::memset(binds, 0, sizeof(binds));

    unsigned long l_status1 = status.length();
    unsigned long l_status2 = status.length();

    binds[0].buffer_type = MYSQL_TYPE_STRING; binds[0].buffer = const_cast<char*>(status.c_str()); binds[0].length = &l_status1;
    binds[1].buffer_type = MYSQL_TYPE_STRING; binds[1].buffer = const_cast<char*>(status.c_str()); binds[1].length = &l_status2;
    binds[2].buffer_type = MYSQL_TYPE_LONG;   binds[2].buffer = &id;

    if (mysql_stmt_bind_param(stmt, binds) != 0 || mysql_stmt_execute(stmt) != 0) {
        error_message = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return false;
    }

    mysql_stmt_close(stmt);
    return true;
}

// -----------------------------------------------------------------------------
// Stats Aggregation
// -----------------------------------------------------------------------------

SystemStats Database::get_stats() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    SystemStats stats;
    if (!conn_) return stats;

    // Device counts
    const char* sql_dev = "SELECT "
                          "COUNT(*), "
                          "COUNT(CASE WHEN status = 'Active' THEN 1 END), "
                          "COUNT(CASE WHEN status = 'Broken' THEN 1 END), "
                          "COUNT(CASE WHEN status = 'Repairing' THEN 1 END) "
                          "FROM devices";
    if (mysql_query(conn_, sql_dev) == 0) {
        MYSQL_RES* res = mysql_store_result(conn_);
        if (res) {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row) {
                stats.total_devices = row[0] ? std::stoi(row[0]) : 0;
                stats.active_devices = row[1] ? std::stoi(row[1]) : 0;
                stats.broken_devices = row[2] ? std::stoi(row[2]) : 0;
                stats.repairing_devices = row[3] ? std::stoi(row[3]) : 0;
            }
            mysql_free_result(res);
        }
    }

    // Pending maintenance count
    const char* sql_maint = "SELECT COUNT(*) FROM maintenance_logs WHERE status = 'Pending'";
    if (mysql_query(conn_, sql_maint) == 0) {
        MYSQL_RES* res = mysql_store_result(conn_);
        if (res) {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row && row[0]) {
                stats.pending_maintenance = std::stoi(row[0]);
            }
            mysql_free_result(res);
        }
    }

    return stats;
}

} // namespace lab
