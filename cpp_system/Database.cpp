#include "Database.hpp"

#include <iostream>

Database::Database() : connection_(mysql_init(nullptr)) {}

Database::~Database()
{
    if (connection_) mysql_close(connection_);
}

bool Database::connect()
{
    return connection_ &&
           mysql_real_connect(connection_, "localhost", "root", "@1234",
                              "campus_store", 3306, nullptr, 0);
}

bool Database::execute(const std::string& sql) const
{
    if (mysql_query(connection_, sql.c_str()) != 0) {
        std::cerr << "MySQL error: " << mysql_error(connection_) << "\n";
        return false;
    }
    return true;
}

MYSQL_RES* Database::query(const std::string& sql) const
{
    if (!execute(sql)) return nullptr;
    return mysql_store_result(connection_);
}

std::string Database::escape(const std::string& value) const
{
    std::string result(value.size() * 2 + 1, '\0');
    const unsigned long length = mysql_real_escape_string(
        connection_, &result[0], value.c_str(),
        static_cast<unsigned long>(value.size()));
    result.resize(length);
    return result;
}

unsigned long long Database::affectedRows() const
{
    return mysql_affected_rows(connection_);
}

unsigned long long Database::lastInsertId() const
{
    return mysql_insert_id(connection_);
}
