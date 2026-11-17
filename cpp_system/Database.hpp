#ifndef CAMPUS_DATABASE_HPP
#define CAMPUS_DATABASE_HPP

#include <mysql.h>

#include <string>

class Database {
    MYSQL* connection_;

public:
    Database();
    ~Database();
    Database(const Database&) = delete;

    bool connect();
    bool execute(const std::string& sql) const;
    MYSQL_RES* query(const std::string& sql) const;
    std::string escape(const std::string& value) const;
    unsigned long long affectedRows() const;
    unsigned long long lastInsertId() const;
};

#endif
