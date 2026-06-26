#pragma once
#include "../../cinema_data/include/IUserRepository.h"
#include "../../cinema_data/include/SmtpMailer.h"
#include "../../cinema_data/include/OAuthHelper.h"
#include <memory>
#include <string>

class UserService {
    std::shared_ptr<IUserRepository> m_repo;
    SmtpConfig  m_smtp;
    std::string m_lastError;
    std::string m_lastRole;
public:
    explicit UserService(std::shared_ptr<IUserRepository> repo, SmtpConfig smtp = {})
        : m_repo(std::move(repo)), m_smtp(std::move(smtp)) {}

    const std::string& GetLastError()        const { return m_lastError; }
    const std::string& GetLastLoggedInRole() const { return m_lastRole;  }

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
        bool sent = SmtpMailer::Send(m_smtp, email,
            "Cinema Admin Verification Code",
            "Your verification code is: " + code +
            "\r\n\r\nThis code expires in 15 minutes.");
        if (!sent) {
            // Email failed but the code is stored in the DB — registration can still proceed.
            // Surface the code so the UI can display it as a fallback.
            m_lastError = "Email could not be sent. Your code is: " + code;
        }
        return true;
    }

    bool Login(const std::string& username, const std::string& password) {
        m_lastError = "";
        if (username.empty() || password.empty()) {
            m_lastError = "Username and password are required."; return false;
        }
        bool ok = m_repo->ValidateLogin(username, password);
        if (ok) {
            m_lastRole = m_repo->GetLastLoggedInRole();
        } else {
            m_lastError = m_repo->GetLastError();
            if (m_lastError.empty()) m_lastError = "Incorrect username or password.";
        }
        return ok;
    }

    // Opens the browser for Google or Microsoft sign-in (PKCE OAuth 2.0).
    // Blocks until the user completes login or 5 minutes elapse — call from a background thread.
    bool LoginWithOAuth(OAuthProvider provider) {
        m_lastError = "";
        OAuthResult r = DoOAuthFlow(provider);
        if (!r.success) { m_lastError = r.error; return false; }

        // User already has an account — log them in directly
        if (m_repo->LoginByEmail(r.email)) return true;

        // First-time sign-in — create a customer account from the provider profile
        bool ok = m_repo->RegisterOAuthUser(r.name, r.email);
        if (!ok) {
            m_lastError = m_repo->GetLastError();
            if (m_lastError.empty()) m_lastError = "Failed to create account.";
        }
        return ok;
    }
};
