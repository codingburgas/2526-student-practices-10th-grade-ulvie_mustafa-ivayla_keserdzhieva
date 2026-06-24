#pragma once
#include "../../cinema_data/include/IUserRepository.h"
#include <memory>
#include <string>

class UserService {
    std::shared_ptr<IUserRepository> m_repo;
public:
    explicit UserService(std::shared_ptr<IUserRepository> repo)
        : m_repo(std::move(repo)) {}

    bool RegisterUser(const std::string& name, const std::string& surname,
                      const std::string& username, const std::string& password) {
        if (name.empty() || surname.empty() || username.empty() || password.empty())
            return false;
        return m_repo->RegisterUser({ name, surname, username, password });
    }

    bool Login(const std::string& username, const std::string& password) {
        if (username.empty() || password.empty()) return false;
        return m_repo->ValidateLogin(username, password);
    }
};
