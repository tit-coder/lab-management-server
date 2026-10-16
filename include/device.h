#ifndef DEVICE_H
#define DEVICE_H

#include "database.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

struct Device {
    int id = 0;
    std::string device_code;
    std::string device_name;
    std::string device_type;
    std::string room;
    std::string status = "Active"; // Active, Broken, Repairing
    std::string created_at;
    std::string updated_at;

    nlohmann::json toJson() const;
    static bool fromJson(const nlohmann::json& j, Device& d, std::string& errorMsg);
    static bool isValidStatus(const std::string& status);
};

struct DeviceStats {
    int total = 0;
    int active = 0;
    int broken = 0;
    int repairing = 0;

    nlohmann::json toJson() const;
};

class DeviceService {
public:
    explicit DeviceService(Database& database);

    bool getAll(
        std::vector<Device>& devices,
        const std::string& roomFilter = "",
        const std::string& statusFilter = "",
        const std::string& search = ""
    );

    bool getById(int id, Device& device);
    bool getByCode(const std::string& code, Device& device);

    bool create(const Device& device, int& newId, std::string& errorMsg, bool& isConflict);
    bool update(int id, const Device& device, std::string& errorMsg, bool& notFound, bool& isConflict);
    bool remove(int id, std::string& errorMsg, bool& notFound, bool& hasConflict);

    bool getStats(DeviceStats& stats, std::string& errorMsg);

    static bool isValidStatus(const std::string& status);

private:
    Database& db;
};

#endif // DEVICE_H
