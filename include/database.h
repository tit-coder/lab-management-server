#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <optional>
#include <mutex>

#if __has_include(<mariadb/mysql.h>)
#include <mariadb/mysql.h>
#elif __has_include(<mysql/mysql.h>)
#include <mysql/mysql.h>
#else
#include <mysql.h>
#endif

namespace lab {

// Represents an equipment device in lab_management
struct Device {
    int id = 0;
    std::string device_code;
    std::string device_name;
    std::string device_type;
    std::string room;
    std::string status = "Active"; // Active, Broken, Repairing
    std::string created_at;

    std::string to_json() const;
};

// Represents an equipment maintenance log entry
struct MaintenanceLog {
    int id = 0;
    int device_id = 0;
    std::string device_code; // Populated via JOIN if available
    std::string issue_description;
    std::string status = "Pending"; // Pending, InProgress, Completed
    std::string created_at;
    std::string completed_at;

    std::string to_json() const;
};

// Aggregated statistical summary for lab dashboard
struct SystemStats {
    int total_devices = 0;
    int active_devices = 0;
    int broken_devices = 0;
    int repairing_devices = 0;
    int pending_maintenance = 0;

    std::string to_json() const;
};

class Database {
public:
    Database();
    ~Database();

    // Establish connection to MariaDB server
    bool connect(const std::string& host,
                 int port,
                 const std::string& user,
                 const std::string& password,
                 const std::string& database_name);

    // Disconnect and release resources
    void disconnect();

    // Check connection health (ping MariaDB)
    bool is_healthy();

    // Device CRUD operations using Prepared Statements
    std::vector<Device> get_devices(const std::string& search = "",
                                    const std::string& room = "",
                                    const std::string& status = "");
    std::optional<Device> get_device_by_id(int id);
    int create_device(const Device& device, std::string& error_message);
    bool update_device(int id, const Device& device, std::string& error_message);
    bool delete_device(int id, std::string& error_message);

    // Maintenance log operations
    std::vector<MaintenanceLog> get_maintenance_logs(const std::string& status = "");
    int create_maintenance_log(const MaintenanceLog& log, std::string& error_message);
    bool update_maintenance_log(int id, const std::string& status, std::string& error_message);
;
    public:
    // Dashboard analytics
    SystemStats get_stats();

    static std::string escape_json(const std::string& input);

    private:
    MYSQL* conn_ = nullptr;
    std::mutex db_mutex_;

    void log_error(const std::string& context);
};

} // namespace lab

#endif // DATABASE_H
