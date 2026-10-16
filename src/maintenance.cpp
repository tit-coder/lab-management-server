#include "maintenance.h"
#include <iostream>

bool MaintenanceService::isValidStatus(const std::string& status) {
    return (status == "Pending" || status == "InProgress" || status == "Completed");
}

nlohmann::json MaintenanceLog::toJson() const {
    nlohmann::json j{
        {"id", id},
        {"device_id", device_id},
        {"device_code", device_code},
        {"device_name", device_name},
        {"room", room},
        {"description", description},
        {"status", status},
        {"created_at", created_at}
    };

    if (completed_at.empty()) {
        j["completed_at"] = nullptr;
    } else {
        j["completed_at"] = completed_at;
    }

    return j;
}

MaintenanceService::MaintenanceService(Database& database)
    : db(database) {
}

bool MaintenanceService::getAll(
    std::vector<MaintenanceLog>& logs,
    int deviceIdFilter,
    const std::string& statusFilter
) {
    logs.clear();
    std::string sql = 
        "SELECT m.id, m.device_id, COALESCE(d.device_code, '') AS device_code, "
        "COALESCE(d.device_name, '') AS device_name, COALESCE(d.room, '') AS room, "
        "m.description, m.status, m.created_at, m.completed_at "
        "FROM maintenance_logs m "
        "LEFT JOIN devices d ON m.device_id = d.id "
        "WHERE 1=1";

    if (deviceIdFilter > 0) {
        sql += " AND m.device_id = " + std::to_string(deviceIdFilter);
    }
    if (!statusFilter.empty() && isValidStatus(statusFilter)) {
        sql += " AND m.status = '" + db.escape(statusFilter) + "'";
    }

    sql += " ORDER BY m.id DESC";

    std::vector<std::map<std::string, std::string>> rows;
    if (!db.query(sql, rows)) {
        return false;
    }

    for (const auto& r : rows) {
        MaintenanceLog log;
        try { log.id = std::stoi(r.at("id")); } catch (...) { log.id = 0; }
        try { log.device_id = std::stoi(r.at("device_id")); } catch (...) { log.device_id = 0; }
        log.device_code = r.count("device_code") ? r.at("device_code") : "";
        log.device_name = r.count("device_name") ? r.at("device_name") : "";
        log.room = r.count("room") ? r.at("room") : "";
        log.description = r.count("description") ? r.at("description") : "";
        log.status = r.count("status") ? r.at("status") : "Pending";
        log.created_at = r.count("created_at") ? r.at("created_at") : "";
        log.completed_at = r.count("completed_at") ? r.at("completed_at") : "";
        logs.push_back(log);
    }

    return true;
}

bool MaintenanceService::getById(int id, MaintenanceLog& log) {
    std::string sql = 
        "SELECT m.id, m.device_id, COALESCE(d.device_code, '') AS device_code, "
        "COALESCE(d.device_name, '') AS device_name, COALESCE(d.room, '') AS room, "
        "m.description, m.status, m.created_at, m.completed_at "
        "FROM maintenance_logs m "
        "LEFT JOIN devices d ON m.device_id = d.id "
        "WHERE m.id = " + std::to_string(id);

    std::vector<std::map<std::string, std::string>> rows;
    if (!db.query(sql, rows) || rows.empty()) {
        return false;
    }

    const auto& r = rows[0];
    log.id = id;
    try { log.device_id = std::stoi(r.at("device_id")); } catch (...) { log.device_id = 0; }
    log.device_code = r.count("device_code") ? r.at("device_code") : "";
    log.device_name = r.count("device_name") ? r.at("device_name") : "";
    log.room = r.count("room") ? r.at("room") : "";
    log.description = r.count("description") ? r.at("description") : "";
    log.status = r.count("status") ? r.at("status") : "Pending";
    log.created_at = r.count("created_at") ? r.at("created_at") : "";
    log.completed_at = r.count("completed_at") ? r.at("completed_at") : "";

    return true;
}

bool MaintenanceService::create(
    int deviceId,
    const std::string& description,
    const std::string& initialStatus,
    int& newId,
    std::string& errorMsg,
    bool& deviceNotFound
) {
    deviceNotFound = false;
    errorMsg.clear();

    if (description.empty()) {
        errorMsg = "Mo ta su co bao tri khong duoc de trong";
        return false;
    }

    std::string status = initialStatus.empty() ? "Pending" : initialStatus;
    if (!isValidStatus(status)) {
        errorMsg = "Trang thai phieu bao tri khong hop le (chi chap nhan: Pending, InProgress, Completed)";
        return false;
    }

    // Kiem tra thiet bi co ton tai khong
    std::string checkDevSql = "SELECT id, status FROM devices WHERE id = " + std::to_string(deviceId);
    std::vector<std::map<std::string, std::string>> devRows;
    if (!db.query(checkDevSql, devRows) || devRows.empty()) {
        deviceNotFound = true;
        errorMsg = "Khong tim thay thiet bi voi ID " + std::to_string(deviceId);
        return false;
    }

    // Bat dau Transaction
    if (!db.beginTransaction()) {
        errorMsg = "Khong the bat dau transaction: " + db.getError();
        return false;
    }

    // 1. Chen ban ghi vao maintenance_logs
    std::string insertSql = 
        "INSERT INTO maintenance_logs (device_id, description, status) VALUES ("
        + std::to_string(deviceId) + ", '"
        + db.escape(description) + "', '"
        + db.escape(status) + "')";

    if (!db.execute(insertSql)) {
        errorMsg = "Loi tao phieu bao tri: " + db.getError();
        db.rollback();
        return false;
    }

    newId = static_cast<int>(db.getLastInsertId());

    // 2. Cap nhat trang thai thiet bi theo quy tac nhat quan
    // Neu tao phieu Pending -> thiet bi Broken
    // Neu tao phieu InProgress -> thiet bi Repairing
    // Neu tao phieu Completed -> neu khong con phieu nao chua xong thi thiet bi Active
    std::string newDeviceStatus;
    if (status == "Pending") {
        newDeviceStatus = "Broken";
    } else if (status == "InProgress") {
        newDeviceStatus = "Repairing";
    } else {
        newDeviceStatus = "Active";
    }

    std::string updateDevSql = 
        "UPDATE devices SET status = '" + newDeviceStatus + "' WHERE id = " + std::to_string(deviceId);

    if (!db.execute(updateDevSql)) {
        errorMsg = "Loi cap nhat trang thai thiet bi: " + db.getError();
        db.rollback();
        return false;
    }

    if (!db.commit()) {
        errorMsg = "Loi commit transaction: " + db.getError();
        db.rollback();
        return false;
    }

    return true;
}

bool MaintenanceService::update(
    int id,
    const std::string& newStatus,
    const std::string& newDescription,
    std::string& errorMsg,
    bool& notFound
) {
    notFound = false;
    errorMsg.clear();

    if (!newStatus.empty() && !isValidStatus(newStatus)) {
        errorMsg = "Trang thai phieu khong hop le (chi chap nhan: Pending, InProgress, Completed)";
        return false;
    }

    MaintenanceLog current;
    if (!getById(id, current)) {
        notFound = true;
        errorMsg = "Khong tim thay phieu bao tri voi ID " + std::to_string(id);
        return false;
    }

    std::string statusToSet = newStatus.empty() ? current.status : newStatus;
    std::string descToSet = newDescription.empty() ? current.description : newDescription;

    // Bat dau Transaction
    if (!db.beginTransaction()) {
        errorMsg = "Khong the khoi dong transaction: " + db.getError();
        return false;
    }

    // 1. Cap nhat ban ghi maintenance_logs
    std::string updateLogSql = "UPDATE maintenance_logs SET status = '" + db.escape(statusToSet) + "', "
        "description = '" + db.escape(descToSet) + "' ";

    if (statusToSet == "Completed") {
        updateLogSql += ", completed_at = CURRENT_TIMESTAMP ";
    } else {
        updateLogSql += ", completed_at = NULL ";
    }
    updateLogSql += "WHERE id = " + std::to_string(id);

    if (!db.execute(updateLogSql)) {
        errorMsg = "Loi cap nhat phieu bao tri: " + db.getError();
        db.rollback();
        return false;
    }

    // 2. Cap nhat trang thai thiet bi theo nghiep vu quy dinh
    int deviceId = current.device_id;
    if (statusToSet == "Completed") {
        // Kiem tra con phieu nao chua hoan thanh (Pending hoac InProgress) doi voi thiet bi nay khong
        std::string checkSql = 
            "SELECT COUNT(*) AS remaining FROM maintenance_logs "
            "WHERE device_id = " + std::to_string(deviceId) + " "
            "AND status IN ('Pending', 'InProgress') "
            "AND id != " + std::to_string(id);

        std::vector<std::map<std::string, std::string>> rows;
        int remaining = 0;
        if (db.query(checkSql, rows) && !rows.empty()) {
            try { remaining = std::stoi(rows[0].at("remaining")); } catch (...) { remaining = 0; }
        }

        if (remaining == 0) {
            // Khong con phieu ton dong nao: chuyen thiet bi ve Active
            std::string setDevSql = "UPDATE devices SET status = 'Active' WHERE id = " + std::to_string(deviceId);
            if (!db.execute(setDevSql)) {
                errorMsg = "Loi cap nhat trang thai thiet bi ve Active: " + db.getError();
                db.rollback();
                return false;
            }
        }
    } else if (statusToSet == "InProgress") {
        // Dang sua chua -> chuyen thiet bi ve Repairing
        std::string setDevSql = "UPDATE devices SET status = 'Repairing' WHERE id = " + std::to_string(deviceId);
        if (!db.execute(setDevSql)) {
            errorMsg = "Loi cap nhat trang thai thiet bi ve Repairing: " + db.getError();
            db.rollback();
            return false;
        }
    } else if (statusToSet == "Pending") {
        // Cho xu ly -> chuyen thiet bi ve Broken
        std::string setDevSql = "UPDATE devices SET status = 'Broken' WHERE id = " + std::to_string(deviceId);
        if (!db.execute(setDevSql)) {
            errorMsg = "Loi cap nhat trang thai thiet bi ve Broken: " + db.getError();
            db.rollback();
            return false;
        }
    }

    if (!db.commit()) {
        errorMsg = "Loi commit transaction: " + db.getError();
        db.rollback();
        return false;
    }

    return true;
}
