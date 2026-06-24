#pragma once
#include <string>

struct SmtpConfig {
    std::string host     = "localhost";
    int         port     = 1025;               // MailHog default; use 587 for real SMTP
    std::string from     = "noreply@cinema.local";
    std::string username = "";                 // leave empty for no auth
    std::string password = "";
};

namespace SmtpMailer {
    bool Send(const SmtpConfig& cfg,
              const std::string& to,
              const std::string& subject,
              const std::string& body);
}
