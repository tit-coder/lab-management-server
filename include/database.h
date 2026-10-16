#ifndef DATABASE_H
#define DATABASE_H

#if __has_include(<mariadb/mysql.h>)
#include <mariadb/mysql.h>
#elif __has_include(<mysql/mysql.h>)
#include <mysql/mysql.h>
#else
#include <mysql.h>
#endif

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <cstdint>

class Database {
public:
    Database();
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool connect(
        const std::string& host,
        unsigned int port,
        const std::string& username,
        const std::string& password,
        const std::string& databaseName
    );

    void close();
    bool isConnected();
    bool ping();
    std::string getError();

    // Query execution helpers with thread-safety
    bool execute(const std::string& sql);
    bool query(const std::string& sql, std::vector<std::map<std::string, std::string>>& results);
    uint64_t getLastInsertId();
    uint64_t getAffectedRows();

    // Transaction management
    bool beginTransaction();
    bool commit();
    bool rollback();

    // String escaping for SQL injection prevention
    std::string escape(const std::string& input);

private:
    MYSQL* connection;
    std::string lastError;
    mutable std::mutex dbMutex;

    // Credentials for auto-reconnect
    std::string dbHost;
    unsigned int dbPort;
    std::string dbUser;
    std::string dbPass;
    std::string dbName;

    bool reconnectIfNeeded();
};

#endif // DATABASE_H
