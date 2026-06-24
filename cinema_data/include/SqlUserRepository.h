#pragma once
#include "IUserRepository.h"
#include <windows.h>
#include <sql.h>
#include <sqlext.h>

class SqlUserRepository : public IUserRepository {
    SQLHENV hEnv = SQL_NULL_HENV;
    SQLHDBC hDbc = SQL_NULL_HDBC;
    std::string m_lastError;
    bool Connect();
    void Disconnect();
public:
    SqlUserRepository();
    ~SqlUserRepository() override;
    bool RegisterUser(const UserRecord& user) override;
    bool ValidateLogin(const std::string& username, const std::string& password) override;
    bool CreateVerificationCode(const std::string& email, std::string& outCode) override;
    bool ValidateVerificationCode(const std::string& email, const std::string& code) override;
    const std::string& GetLastError() const override { return m_lastError; }
};
