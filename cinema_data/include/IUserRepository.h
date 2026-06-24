#pragma once
#include <string>

struct UserRecord {
    std::string name;
    std::string surname;
    std::string username;
    std::string password;
};

class IUserRepository {
public:
    virtual ~IUserRepository() = default;
    virtual bool RegisterUser(const UserRecord& user) = 0;
    virtual bool ValidateLogin(const std::string& username, const std::string& password) = 0;
};
