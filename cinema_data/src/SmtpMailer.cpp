#include "../include/SmtpMailer.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#include <string>

static std::string base64Encode(const std::string& in) {
    static const char* T = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int i = 0;
    unsigned char b[3]; unsigned char c[4];
    size_t pos = 0, len = in.size();
    while (pos < len) {
        int n = (int)min((size_t)3, len - pos);
        for (int k = 0; k < 3; k++) b[k] = (k < n) ? (unsigned char)in[pos + k] : 0;
        pos += n;
        c[0] = (b[0] & 0xfc) >> 2;
        c[1] = ((b[0] & 0x03) << 4) | ((b[1] & 0xf0) >> 4);
        c[2] = ((b[1] & 0x0f) << 2) | ((b[2] & 0xc0) >> 6);
        c[3] = b[2] & 0x3f;
        for (int k = 0; k < n + 1; k++) out += T[c[k]];
        while (n++ < 3) out += '=';
    }
    return out;
}

static bool smtpSend(SOCKET s, const std::string& cmd) {
    std::string msg = cmd + "\r\n";
    return send(s, msg.c_str(), (int)msg.size(), 0) != SOCKET_ERROR;
}

static std::string smtpRecv(SOCKET s) {
    char buf[4096] = {};
    int n = recv(s, buf, sizeof(buf) - 1, 0);
    return (n > 0) ? std::string(buf, n) : "";
}

bool SmtpMailer::Send(const SmtpConfig& cfg,
                      const std::string& to,
                      const std::string& subject,
                      const std::string& body)
{
    WSADATA wd;
    if (WSAStartup(MAKEWORD(2, 2), &wd) != 0) return false;

    addrinfo hints{}, *res = nullptr;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    std::string portStr = std::to_string(cfg.port);
    if (getaddrinfo(cfg.host.c_str(), portStr.c_str(), &hints, &res) != 0) {
        WSACleanup(); return false;
    }

    SOCKET s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    bool connected = (connect(s, res->ai_addr, (int)res->ai_addrlen) == 0);
    freeaddrinfo(res);
    if (!connected) { closesocket(s); WSACleanup(); return false; }

    smtpRecv(s);                            // 220 greeting
    smtpSend(s, "EHLO localhost");
    smtpRecv(s);

    if (!cfg.username.empty()) {
        // AUTH PLAIN: base64(\0username\0password)
        std::string cred = std::string("\0", 1) + cfg.username
                         + std::string("\0", 1) + cfg.password;
        smtpSend(s, "AUTH PLAIN " + base64Encode(cred));
        smtpRecv(s);
    }

    smtpSend(s, "MAIL FROM:<" + cfg.from + ">"); smtpRecv(s);
    smtpSend(s, "RCPT TO:<"   + to        + ">"); smtpRecv(s);
    smtpSend(s, "DATA");                          smtpRecv(s);

    std::string msg = "From: Cinema App <" + cfg.from + ">\r\n"
                    + "To: " + to + "\r\n"
                    + "Subject: " + subject + "\r\n"
                    + "MIME-Version: 1.0\r\n"
                    + "Content-Type: text/plain; charset=UTF-8\r\n"
                    + "\r\n"
                    + body + "\r\n.\r\n";
    bool ok = (send(s, msg.c_str(), (int)msg.size(), 0) != SOCKET_ERROR);
    smtpRecv(s);

    smtpSend(s, "QUIT");
    closesocket(s);
    WSACleanup();
    return ok;
}
