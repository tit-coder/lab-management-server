#include "database.h"
#include <iostream>
#include <vector>

Database::Database()
    : connection(nullptr), dbPort(3306) {
}

Database::~Database() {
    close();
}

bool Database::connect(
    const std::string& host,
    unsigned int port,
    const std::string& username,
    const std::string& password,
    const std::string& databaseName
) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    dbHost = host;
    dbPort = port;
    dbUser = username;
    dbPass = password;
    dbName = databaseName;

    if (connection != nullptr) {
        mysql_close(connection);
        connection = nullptr;
    }
    lastError.clear();

    connection = mysql_init(nullptr);
    if (connection == nullptr) {
        lastError = "mysql_init failed: Khong the khoi tao MariaDB client structure";
        return false;
    }

    // Set connection options
    my_bool reconnect = 1;
    mysql_options(connection, MYSQL_OPT_RECONNECT, &reconnect);
    unsigned int timeout = 5;
    mysql_options(connection, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    if (mysql_real_connect(
            connection,
            host.c_str(),
            username.c_str(),
            password.c_str(),
            databaseName.c_str(),
            port,
            nullptr,
            0
        ) == nullptr) {
        lastError = mysql_error(connection);
        mysql_close(connection);
        connection = nullptr;
        return false;
    }

    mysql_set_character_set(connection, "utf8mb4");
    return true;
}

void Database::close() {
    std::lock_guard<std::mutex> lock(dbMutex);
    if (connection != nullptr) {
        mysql_close(connection);
        connection = nullptr;
    }
}

bool Database::isConnected() {
    std::lock_guard<std::mutex> lock(dbMutex);
    if (connection == nullptr) return false;
    return mysql_ping(connection) == 0;
}

bool Database::ping() {
    std::lock_guard<std::mutex> lock(dbMutex);
    if (connection == nullptr) {
        return reconnectIfNeeded();
    }
    if (mysql_ping(connection) != 0) {
        return reconnectIfNeeded();
    }
    return true;
}

bool Database::reconnectIfNeeded() {
    // Note: caller already holds dbMutex
    if (dbHost.empty() || dbUser.empty() || dbName.empty()) {
        return false;
    }
    if (connection != nullptr) {
        mysql_close(connection);
        connection = nullptr;
    }
    connection = mysql_init(nullptr);
    if (connection == nullptr) {
        lastError = "mysql_init failed during reconnect";
        return false;
    }

    my_bool reconnect = 1;
    mysql_options(connection, MYSQL_OPT_RECONNECT, &reconnect);

    if (mysql_real_connect(
            connection,
            dbHost.c_str(),
            dbUser.c_str(),
            dbPass.c_str(),
            dbName.c_str(),
            dbPort,
            nullptr,
            0
        ) == nullptr) {
        lastError = mysql_error(connection);
        mysql_close(connection);
        connection = nullptr;
        return false;
    }
    mysql_set_character_set(connection, "utf8mb4");
    return true;
}

std::string Database::getError() {
    std::lock_guard<std::mutex> lock(dbMutex);
    return lastError;
}

bool Database::execute(const std::string& sql) {
    std::lock_guard<std::mutex> lock(dbMutex);
    if (connection == nullptr && !reconnectIfNeeded()) {
        lastError = "Database chua duoc ket noi";
        return false;
    }

    if (mysql_query(connection, sql.c_str()) != 0) {
        lastError = mysql_error(connection);
        return false;
    }
    return true;
}

bool Database::query(const std::string& sql, std::vector<std::map<std::string, std::string>>& results) {
    std::lock_guard<std::mutex> lock(dbMutex);
    results.clear();

    if (connection == nullptr && !reconnectIfNeeded()) {
        lastError = "Database chua duoc ket noi";
        return false;
    }

    if (mysql_query(connection, sql.c_str()) != 0) {
        lastError = mysql_error(connection);
        return false;
    }

    MYSQL_RES* res = mysql_store_result(connection);
    if (res == nullptr) {
        // Query did not return a result set or an error occurred
        if (mysql_field_count(connection) == 0) {
            return true; // Not a SELECT query
        } else {
            lastError = mysql_error(connection);
            return false;
        }
    }

    int numFields = mysql_num_fields(res);
    MYSQL_FIELD* fields = mysql_fetch_fields(res);

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        std::map<std::string, std::string> rowMap;
        for (int i = 0; i < numFields; ++i) {
            std::string fieldName = fields[i].name;
            std::string fieldValue = (row[i] != nullptr) ? row[i] : "";
            rowMap[fieldName] = fieldValue;
        }
        results.push_back(rowMap);
    }

    mysql_free_result(res);
    return true;
}

uint64_t Database::getLastInsertId() {
    std::lock_guard<std::mutex> lock(dbMutex);
    if (connection == nullptr) return 0;
    return mysql_insert_id(connection);
}

uint64_t Database::getAffectedRows() {
    std::lock_guard<std::mutex> lock(dbMutex);
    if (connection == nullptr) return 0;
    my_ulonglong aff = mysql_affected_rows(connection);
    return (aff == (my_ulonglong)-1) ? 0 : aff;
}

bool Database::beginTransaction() {
    std::lock_guard<std::mutex> lock(dbMutex);
    if (connection == nullptr && !reconnectIfNeeded()) {
        lastError = "Database chua duoc ket noi";
        return false;
    }
    if (mysql_autocommit(connection, 0) != 0) {
        lastError = mysql_error(connection);
        return false;
    }
    return true;
}

bool Database::commit() {
    std::lock_guard<std::mutex> lock(dbMutex);
    if (connection == nullptr) return false;
    if (mysql_commit(connection) != 0) {
        lastError = mysql_error(connection);
        mysql_autocommit(connection, 1);
        return false;
    }
    mysql_autocommit(connection, 1);
    return true;
}

bool Database::rollback() {
    std::lock_guard<std::mutex> lock(dbMutex);
    if (connection == nullptr) return false;
    mysql_rollback(connection);
    mysql_autocommit(connection, 1);
    return true;
}

std::string Database::escape(const std::string& input) {
    std::lock_guard<std::mutex> lock(dbMutex);
    if (connection == nullptr) {
        // Fallback simple escape
        std::string res;
        for (char c : input) {
            if (c == '\'' || c == '\"' || c == '\\') res += '\\';
            res += c;
        }
        return res;
    }
    std::vector<char> buffer(input.length() * 2 + 1);
    unsigned long len = mysql_real_escape_string(connection, buffer.data(), input.c_str(), input.length());
    return std::string(buffer.data(), len);
}
