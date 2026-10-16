#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

struct AppConfig {
    std::string dbHost = "localhost";
    unsigned int dbPort = 3306;
    std::string dbName = "lab_management";
    std::string dbUser = "labapp";
    std::string dbPassword = "";
    std::string serverHost = "0.0.0.0";
    int serverPort = 8080;

    static void trim(std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    static void loadEnvFile(const std::string& path = ".env") {
        std::ifstream file(path);
        if (!file.is_open()) return;

        std::string line;
        while (std::getline(file, line)) {
            trim(line);
            if (line.empty() || line[0] == '#') continue;

            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string key = line.substr(0, eqPos);
                std::string val = line.substr(eqPos + 1);
                trim(key);
                trim(val);
                if (val.size() >= 2 && ((val.front() == '"' && val.back() == '"') || 
                                        (val.front() == '\'' && val.back() == '\''))) {
                    val = val.substr(1, val.size() - 2);
                }
                // Neu bien moi truong chua ton tai thi set tu file .env
                if (getenv(key.c_str()) == nullptr) {
                    setenv(key.c_str(), val.c_str(), 1);
                }
            }
        }
    }

    static AppConfig load() {
        loadEnvFile(".env");
        AppConfig cfg;

        if (const char* h = getenv("DB_HOST")) cfg.dbHost = h;
        if (const char* p = getenv("DB_PORT")) {
            try { cfg.dbPort = static_cast<unsigned int>(std::stoul(p)); } catch (...) {}
        }
        if (const char* n = getenv("DB_NAME")) cfg.dbName = n;
        if (const char* u = getenv("DB_USER")) cfg.dbUser = u;
        if (const char* pw = getenv("DB_PASSWORD")) cfg.dbPassword = pw;
        if (const char* sh = getenv("SERVER_HOST")) cfg.serverHost = sh;
        if (const char* sp = getenv("SERVER_PORT")) {
            try { cfg.serverPort = std::stoi(sp); } catch (...) {}
        }

        return cfg;
    }
};

#endif // CONFIG_H
