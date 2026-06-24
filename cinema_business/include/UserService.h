#pragma once
#include "../../cinema_data/include/IUserRepository.h"
#include "../../cinema_data/include/SmtpMailer.h"
#include <memory>
#include <string>

class UserService {
    std::shared_ptr<IUserRepository> m_repo;
    SmtpConfig  m_smtp;
    std::string m_lastError;
public:
    explicit UserService(std::shared_ptr<IUserRepository> repo, SmtpConfig smtp = {})
        : m_repo(std::move(repo)), m_smtp(std::move(smtp)) {}

    const std::string& GetLastError() const { return m_lastError; }

    bool RegisterCustomer(const std::string& name,     const std::string& surname,
                          const std::string& username, const std::string& password,
                          const std::string& email) {
        m_lastError = "";
        if (name.empty() || surname.empty() || username.empty() || password.empty()) {
            m_lastError = "All fields are required."; return false;
        }
        bool ok = m_repo->RegisterUser({ name, surname, username, password, email, "customer", "", "" });
        if (!ok) m_lastError = m_repo->GetLastError();
        return ok;
    }

    bool RegisterAdmin(const std::string& name,     const std::string& surname,
                       const std::string& username, const std::string& password,
                       const std::string& email,    const std::string& company,
                       const std::string& city,     const std::string& verifyCode) {
        m_lastError = "";
        if (name.empty() || surname.empty() || username.empty() ||
            password.empty() || email.empty() || verifyCode.empty()) {
            m_lastError = "All fields are required."; return false;
        }
        if (!m_repo->ValidateVerificationCode(email, verifyCode)) {
            m_lastError = m_repo->GetLastError();
            if (m_lastError.empty()) m_lastError = "Verification code is invalid or has expired.";
            return false;
        }
        bool ok = m_repo->RegisterUser({ name, surname, username, password, email, "admin", company, city });
        if (!ok) m_lastError = m_repo->GetLastError();
        return ok;
    }

    bool SendVerificationCode(const std::string& email) {
        m_lastError = "";
        if (email.empty()) { m_lastError = "Email is required."; return false; }
        std::string code;
        if (!m_repo->CreateVerificationCode(email, code)) {
            m_lastError = m_repo->GetLastError();
            if (m_lastError.empty()) m_lastError = "Failed to generate verification code.";
            return false;
        }
        bool ok = SmtpMailer::Send(m_smtp, email,
            "Cinema Admin Verification Code",
            "Your verification code is: " + code +
            "\r\n\r\nThis code expires in 15 minutes.");
        if (!ok) m_lastError = "Code generated but email could not be sent. Check SMTP config.";
        return ok;
    }

    bool Login(const std::string& username, const std::string& password) {
        m_lastError = "";
        if (username.empty() || password.empty()) {
            m_lastError = "Username and password are required."; return false;
        }
        bool ok = m_repo->ValidateLogin(username, password);
        if (!ok) {
            m_lastError = m_repo->GetLastError();
            if (m_lastError.empty()) m_lastError = "Incorrect username or password.";
        }
        return ok;
    }
};
