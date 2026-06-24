#pragma once
#include "IUserRepository.h"
#include <windows.h>
#include <sql.h>
#include <sqlext.h>

class SqlUserRepository : public IUserRepository {
    SQLHENV hEnv = SQL_NULL_HENV;
    SQLHDBC hDbc = SQL_NULL_HDBC;
    bool Connect();
    void Disconnect();
public:
    SqlUserRepository();
    ~SqlUserRepository() override;
    bool RegisterUser(const UserRecord& user) override;
    bool ValidateLogin(const std::string& username, const std::string& password) override;
};
