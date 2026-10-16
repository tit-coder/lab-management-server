#ifndef MAINTENANCE_H
#define MAINTENANCE_H

#include "database.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

struct MaintenanceLog {
    int id = 0;
    int device_id = 0;
    std::string device_code;
    std::string device_name;
    std::string room;
    std::string description;
    std::string status = "Pending"; // Pending, InProgress, Completed
    std::string created_at;
    std::string completed_at; // Empty if null

    nlohmann::json toJson() const;
};

class MaintenanceService {
public:
    explicit MaintenanceService(Database& database);

    bool getAll(
        std::vector<MaintenanceLog>& logs,
        int deviceIdFilter = 0,
        const std::string& statusFilter = ""
    );

    bool getById(int id, MaintenanceLog& log);

    bool create(
        int deviceId,
        const std::string& description,
        const std::string& initialStatus,
        int& newId,
        std::string& errorMsg,
        bool& deviceNotFound
    );

    bool update(
        int id,
        const std::string& newStatus,
        const std::string& newDescription,
        std::string& errorMsg,
        bool& notFound
    );

    static bool isValidStatus(const std::string& status);

private:
    Database& db;
};

#endif // MAINTENANCE_H
