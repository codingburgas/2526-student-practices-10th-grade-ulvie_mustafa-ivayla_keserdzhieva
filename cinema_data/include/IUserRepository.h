#pragma once
#include <string>

struct UserRecord {
    std::string name;
    std::string surname;
    std::string username;
    std::string password;
    std::string email;
    std::string role;      // "customer" | "admin"
    std::string company;   // admin only
    std::string city;      // admin only
};

class IUserRepository {
public:
    virtual ~IUserRepository() = default;
    virtual bool RegisterUser(const UserRecord& user) = 0;
    virtual bool ValidateLogin(const std::string& username, const std::string& password) = 0;
    virtual bool CreateVerificationCode(const std::string& email, std::string& outCode) = 0;
    virtual bool ValidateVerificationCode(const std::string& email, const std::string& code) = 0;
    virtual const std::string& GetLastError() const = 0;
};
