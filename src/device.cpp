#include "device.h"
#include <sstream>
#include <iostream>

bool Device::isValidStatus(const std::string& status) {
    return (status == "Active" || status == "Broken" || status == "Repairing");
}

bool DeviceService::isValidStatus(const std::string& status) {
    return Device::isValidStatus(status);
}

nlohmann::json Device::toJson() const {
    return nlohmann::json{
        {"id", id},
        {"device_code", device_code},
        {"device_name", device_name},
        {"device_type", device_type},
        {"room", room},
        {"status", status},
        {"created_at", created_at},
        {"updated_at", updated_at}
    };
}

bool Device::fromJson(const nlohmann::json& j, Device& d, std::string& errorMsg) {
    if (!j.is_object()) {
        errorMsg = "Du lieu phai la mot JSON object";
        return false;
    }

    if (!j.contains("device_code") || !j["device_code"].is_string() || j["device_code"].get<std::string>().empty()) {
        errorMsg = "Truong 'device_code' la bat buoc va khong duoc de trong";
        return false;
    }
    d.device_code = j["device_code"].get<std::string>();
    if (d.device_code.length() > 30) {
        errorMsg = "Truong 'device_code' khong duoc vuot qua 30 ky tu";
        return false;
    }

    if (!j.contains("device_name") || !j["device_name"].is_string() || j["device_name"].get<std::string>().empty()) {
        errorMsg = "Truong 'device_name' la bat buoc va khong duoc de trong";
        return false;
    }
    d.device_name = j["device_name"].get<std::string>();
    if (d.device_name.length() > 100) {
        errorMsg = "Truong 'device_name' khong duoc vuot qua 100 ky tu";
        return false;
    }

    if (!j.contains("device_type") || !j["device_type"].is_string() || j["device_type"].get<std::string>().empty()) {
        errorMsg = "Truong 'device_type' la bat buoc va khong duoc de trong";
        return false;
    }
    d.device_type = j["device_type"].get<std::string>();
    if (d.device_type.length() > 50) {
        errorMsg = "Truong 'device_type' khong duoc vuot qua 50 ky tu";
        return false;
    }

    if (!j.contains("room") || !j["room"].is_string() || j["room"].get<std::string>().empty()) {
        errorMsg = "Truong 'room' la bat buoc va khong duoc de trong";
        return false;
    }
    d.room = j["room"].get<std::string>();
    if (d.room.length() > 50) {
        errorMsg = "Truong 'room' khong duoc vuot qua 50 ky tu";
        return false;
    }

    if (j.contains("status")) {
        if (!j["status"].is_string()) {
            errorMsg = "Truong 'status' phai la chuoi ky tu";
            return false;
        }
        std::string s = j["status"].get<std::string>();
        if (!isValidStatus(s)) {
            errorMsg = "Gia tri 'status' khong hop le (chi chap nhan: Active, Broken, Repairing)";
            return false;
        }
        d.status = s;
    } else {
        d.status = "Active";
    }

    return true;
}

nlohmann::json DeviceStats::toJson() const {
    return nlohmann::json{
        {"total", total},
        {"active", active},
        {"broken", broken},
        {"repairing", repairing}
    };
}

DeviceService::DeviceService(Database& database)
    : db(database) {
}

bool DeviceService::getAll(
    std::vector<Device>& devices,
    const std::string& roomFilter,
    const std::string& statusFilter,
    const std::string& search
) {
    devices.clear();
    std::string sql = "SELECT id, device_code, device_name, device_type, room, status, created_at, updated_at FROM devices WHERE 1=1";

    if (!roomFilter.empty()) {
        sql += " AND room = '" + db.escape(roomFilter) + "'";
    }
    if (!statusFilter.empty() && isValidStatus(statusFilter)) {
        sql += " AND status = '" + db.escape(statusFilter) + "'";
    }
    if (!search.empty()) {
        std::string escapedSearch = db.escape(search);
        sql += " AND (device_code LIKE '%" + escapedSearch + "%' OR device_name LIKE '%" + escapedSearch + "%')";
    }

    sql += " ORDER BY id ASC";

    std::vector<std::map<std::string, std::string>> rows;
    if (!db.query(sql, rows)) {
        return false;
    }

    for (const auto& r : rows) {
        Device d;
        try { d.id = std::stoi(r.at("id")); } catch (...) { d.id = 0; }
        d.device_code = r.count("device_code") ? r.at("device_code") : "";
        d.device_name = r.count("device_name") ? r.at("device_name") : "";
        d.device_type = r.count("device_type") ? r.at("device_type") : "";
        d.room = r.count("room") ? r.at("room") : "";
        d.status = r.count("status") ? r.at("status") : "Active";
        d.created_at = r.count("created_at") ? r.at("created_at") : "";
        d.updated_at = r.count("updated_at") ? r.at("updated_at") : "";
        devices.push_back(d);
    }
    return true;
}

bool DeviceService::getById(int id, Device& device) {
    std::string sql = "SELECT id, device_code, device_name, device_type, room, status, created_at, updated_at FROM devices WHERE id = " + std::to_string(id);
    std::vector<std::map<std::string, std::string>> rows;
    if (!db.query(sql, rows) || rows.empty()) {
        return false;
    }

    const auto& r = rows[0];
    device.id = id;
    device.device_code = r.at("device_code");
    device.device_name = r.at("device_name");
    device.device_type = r.at("device_type");
    device.room = r.at("room");
    device.status = r.at("status");
    device.created_at = r.at("created_at");
    device.updated_at = r.at("updated_at");
    return true;
}

bool DeviceService::getByCode(const std::string& code, Device& device) {
    std::string sql = "SELECT id, device_code, device_name, device_type, room, status, created_at, updated_at FROM devices WHERE device_code = '" + db.escape(code) + "'";
    std::vector<std::map<std::string, std::string>> rows;
    if (!db.query(sql, rows) || rows.empty()) {
        return false;
    }

    const auto& r = rows[0];
    try { device.id = std::stoi(r.at("id")); } catch (...) { device.id = 0; }
    device.device_code = r.at("device_code");
    device.device_name = r.at("device_name");
    device.device_type = r.at("device_type");
    device.room = r.at("room");
    device.status = r.at("status");
    device.created_at = r.at("created_at");
    device.updated_at = r.at("updated_at");
    return true;
}

bool DeviceService::create(const Device& device, int& newId, std::string& errorMsg, bool& isConflict) {
    isConflict = false;
    errorMsg.clear();

    // Kiem tra ma thiet bi da ton tai chua
    Device existing;
    if (getByCode(device.device_code, existing)) {
        isConflict = true;
        errorMsg = "Ma thiet bi '" + device.device_code + "' da ton tai trong he thong";
        return false;
    }

    std::string sql = "INSERT INTO devices (device_code, device_name, device_type, room, status) VALUES ('"
        + db.escape(device.device_code) + "', '"
        + db.escape(device.device_name) + "', '"
        + db.escape(device.device_type) + "', '"
        + db.escape(device.room) + "', '"
        + db.escape(device.status) + "')";

    if (!db.execute(sql)) {
        std::string err = db.getError();
        if (err.find("Duplicate entry") != std::string::npos) {
            isConflict = true;
            errorMsg = "Ma thiet bi da ton tai";
        } else {
            errorMsg = "Loi co so du lieu: " + err;
        }
        return false;
    }

    newId = static_cast<int>(db.getLastInsertId());
    return true;
}

bool DeviceService::update(int id, const Device& device, std::string& errorMsg, bool& notFound, bool& isConflict) {
    notFound = false;
    isConflict = false;
    errorMsg.clear();

    // Kiem tra thiet bi co ton tai khong
    Device current;
    if (!getById(id, current)) {
        notFound = true;
        errorMsg = "Khong tim thay thiet bi voi ID " + std::to_string(id);
        return false;
    }

    // Neu thay doi device_code, kiem tra co trung voi thiet bi khac khong
    if (device.device_code != current.device_code) {
        Device duplicateCheck;
        if (getByCode(device.device_code, duplicateCheck) && duplicateCheck.id != id) {
            isConflict = true;
            errorMsg = "Ma thiet bi '" + device.device_code + "' da duoc su dung boi thiet bi khac";
            return false;
        }
    }

    std::string sql = "UPDATE devices SET "
        "device_code = '" + db.escape(device.device_code) + "', "
        "device_name = '" + db.escape(device.device_name) + "', "
        "device_type = '" + db.escape(device.device_type) + "', "
        "room = '" + db.escape(device.room) + "', "
        "status = '" + db.escape(device.status) + "' "
        "WHERE id = " + std::to_string(id);

    if (!db.execute(sql)) {
        std::string err = db.getError();
        if (err.find("Duplicate entry") != std::string::npos) {
            isConflict = true;
            errorMsg = "Ma thiet bi da ton tai";
        } else {
            errorMsg = "Loi cap nhat thiet bi: " + err;
        }
        return false;
    }

    return true;
}

bool DeviceService::remove(int id, std::string& errorMsg, bool& notFound, bool& hasConflict) {
    notFound = false;
    hasConflict = false;
    errorMsg.clear();

    Device current;
    if (!getById(id, current)) {
        notFound = true;
        errorMsg = "Khong tim thay thiet bi voi ID " + std::to_string(id);
        return false;
    }

    // Kiem tra chinh sach bao toan lich su bao tri (ON DELETE RESTRICT)
    std::string checkSql = "SELECT COUNT(*) AS cnt FROM maintenance_logs WHERE device_id = " + std::to_string(id);
    std::vector<std::map<std::string, std::string>> rows;
    if (db.query(checkSql, rows) && !rows.empty()) {
        int count = 0;
        try { count = std::stoi(rows[0].at("cnt")); } catch (...) { count = 0; }
        if (count > 0) {
            hasConflict = true;
            errorMsg = "Khong the xoa thiet bi vi da co " + std::to_string(count) + 
                       " phieu bao tri lien ket (chinh sach bao toan lich su bao tri).";
            return false;
        }
    }

    std::string sql = "DELETE FROM devices WHERE id = " + std::to_string(id);
    if (!db.execute(sql)) {
        errorMsg = "Loi xoa thiet bi: " + db.getError();
        return false;
    }

    return true;
}

bool DeviceService::getStats(DeviceStats& stats, std::string& errorMsg) {
    errorMsg.clear();
    stats = DeviceStats{};

    std::string sql = 
        "SELECT "
        "COUNT(*) AS total, "
        "SUM(CASE WHEN status = 'Active' THEN 1 ELSE 0 END) AS active_cnt, "
        "SUM(CASE WHEN status = 'Broken' THEN 1 ELSE 0 END) AS broken_cnt, "
        "SUM(CASE WHEN status = 'Repairing' THEN 1 ELSE 0 END) AS repairing_cnt "
        "FROM devices";

    std::vector<std::map<std::string, std::string>> rows;
    if (!db.query(sql, rows) || rows.empty()) {
        errorMsg = "Loi truy van thong ke: " + db.getError();
        return false;
    }

    const auto& r = rows[0];
    try {
        stats.total = r.count("total") && !r.at("total").empty() ? std::stoi(r.at("total")) : 0;
        stats.active = r.count("active_cnt") && !r.at("active_cnt").empty() ? std::stoi(r.at("active_cnt")) : 0;
        stats.broken = r.count("broken_cnt") && !r.at("broken_cnt").empty() ? std::stoi(r.at("broken_cnt")) : 0;
        stats.repairing = r.count("repairing_cnt") && !r.at("repairing_cnt").empty() ? std::stoi(r.at("repairing_cnt")) : 0;
    } catch (...) {
        errorMsg = "Loi phan tich ket qua thong ke";
        return false;
    }

    return true;
}
