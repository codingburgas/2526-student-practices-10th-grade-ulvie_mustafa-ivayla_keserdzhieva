#define WIN32_LEAN_AND_MEAN
#include "../include/OAuthHelper.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <windows.h>
#include <shellapi.h>

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "bcrypt.lib")

// ── Fill these in after registering your OAuth apps ──────────────────────────
// Google : https://console.cloud.google.com  → APIs & Services → Credentials
//          → Create OAuth Client ID → Desktop app
//          Add http://127.0.0.1 as an authorized redirect URI.
// Microsoft: https://portal.azure.com → App registrations → New registration
//          → Redirect URI: http://localhost  (public client / PKCE)
static const char* GOOGLE_CLIENT_ID    = "YOUR_GOOGLE_CLIENT_ID.apps.googleusercontent.com";
static const char* MICROSOFT_CLIENT_ID = "YOUR_MICROSOFT_CLIENT_ID";

// ── PKCE helpers ──────────────────────────────────────────────────────────────

static std::string Base64UrlEncode(const uint8_t* data, size_t len) {
    static const char T[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    for (size_t i = 0; i < len; i += 3) {
        uint32_t v = (uint32_t)data[i] << 16;
        if (i + 1 < len) v |= (uint32_t)data[i + 1] << 8;
        if (i + 2 < len) v |= data[i + 2];
        out += T[(v >> 18) & 63];
        out += T[(v >> 12) & 63];
        if (i + 1 < len) out += T[(v >> 6) & 63];
        if (i + 2 < len) out += T[(v     ) & 63];
    }
    for (auto& c : out) { if (c == '+') c = '-'; if (c == '/') c = '_'; }
    return out;
}

static std::vector<uint8_t> Sha256Of(const uint8_t* data, size_t len) {
    std::vector<uint8_t> hash(32);
    BCRYPT_ALG_HANDLE  hAlg  = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    if (BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0)) &&
        BCRYPT_SUCCESS(BCryptCreateHash(hAlg, &hHash, nullptr, 0, nullptr, 0, 0)) &&
        BCRYPT_SUCCESS(BCryptHashData(hHash, const_cast<uint8_t*>(data), (ULONG)len, 0)))
        BCryptFinishHash(hHash, hash.data(), 32, 0);
    if (hHash) BCryptDestroyHash(hHash);
    if (hAlg)  BCryptCloseAlgorithmProvider(hAlg, 0);
    return hash;
}

static std::pair<std::string, std::string> MakePkce() {
    uint8_t raw[32];
    BCryptGenRandom(nullptr, raw, 32, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    std::string verifier  = Base64UrlEncode(raw, 32);
    auto        sha       = Sha256Of((const uint8_t*)verifier.data(), verifier.size());
    std::string challenge = Base64UrlEncode(sha.data(), sha.size());
    return { verifier, challenge };
}

// ── URL encoding ──────────────────────────────────────────────────────────────

static std::string UrlEncode(const std::string& s) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase;
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            oss << (char)c;
        else
            oss << '%' << std::setw(2) << std::setfill('0') << (int)c;
    }
    return oss.str();
}

// ── Simple JSON string-field extractor ───────────────────────────────────────

static std::string JsonStr(const std::string& json, const std::string& key) {
    std::string k = "\"" + key + "\":\"";
    auto p = json.find(k);
    if (p == std::string::npos) return {};
    p += k.size();
    auto e = json.find('"', p);
    return e == std::string::npos ? std::string{} : json.substr(p, e - p);
}

// ── Local HTTP callback server ────────────────────────────────────────────────

struct LocalServer {
    SOCKET sock = INVALID_SOCKET;
    int    port = 0;

    bool Start() {
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) return false;
        sockaddr_in addr = {};
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port        = 0;
        if (bind(sock, (sockaddr*)&addr, sizeof(addr)) != 0) { Close(); return false; }
        int len = sizeof(addr);
        getsockname(sock, (sockaddr*)&addr, &len);
        port = ntohs(addr.sin_port);
        return listen(sock, 1) == 0;
    }

    std::string WaitForCode(int timeoutSecs) {
        fd_set fds; FD_ZERO(&fds); FD_SET(sock, &fds);
        timeval tv = { timeoutSecs, 0 };
        if (select(0, &fds, nullptr, nullptr, &tv) <= 0) return {};

        SOCKET client = accept(sock, nullptr, nullptr);
        if (client == INVALID_SOCKET) return {};

        char buf[4096] = {};
        recv(client, buf, sizeof(buf) - 1, 0);
        std::string req(buf);

        auto extract = [&](const std::string& param) -> std::string {
            std::string k = param + "=";
            auto p = req.find(k);
            if (p == std::string::npos) return {};
            p += k.size();
            auto e = req.find_first_of("& \r\nH", p);
            return req.substr(p, e == std::string::npos ? std::string::npos : e - p);
        };
        std::string code  = extract("code");
        std::string error = extract("error");

        const char* body = error.empty()
            ? "<!DOCTYPE html><html><body style='font-family:sans-serif;text-align:center;margin-top:80px'>"
              "<h2 style='color:#4CAF50'>&#10003; Logged in!</h2>"
              "<p>You can close this tab and return to the app.</p></body></html>"
            : "<!DOCTYPE html><html><body style='font-family:sans-serif;text-align:center;margin-top:80px'>"
              "<h2 style='color:#f44336'>&#10007; Login cancelled</h2>"
              "<p>You can close this tab.</p></body></html>";
        std::string resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
        resp += body;
        send(client, resp.c_str(), (int)resp.size(), 0);
        closesocket(client);
        return code;
    }

    void Close() {
        if (sock != INVALID_SOCKET) { closesocket(sock); sock = INVALID_SOCKET; }
    }
};

// ── WinHTTP wrappers ──────────────────────────────────────────────────────────

static std::string HttpsPost(const wchar_t* host, const wchar_t* path, const std::string& body) {
    std::string result;
    HINTERNET hSess = WinHttpOpen(L"CinemaApp/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                   WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSess) return result;
    HINTERNET hConn = WinHttpConnect(hSess, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
    HINTERNET hReq  = hConn ? WinHttpOpenRequest(hConn, L"POST", path, nullptr,
                                   WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                   WINHTTP_FLAG_SECURE) : nullptr;
    if (hReq) {
        if (WinHttpSendRequest(hReq,
                L"Content-Type: application/x-www-form-urlencoded", (DWORD)-1,
                (LPVOID)body.data(), (DWORD)body.size(), (DWORD)body.size(), 0) &&
            WinHttpReceiveResponse(hReq, nullptr)) {
            DWORD avail = 0;
            while (WinHttpQueryDataAvailable(hReq, &avail) && avail) {
                std::vector<char> buf(avail + 1);
                DWORD read = 0;
                WinHttpReadData(hReq, buf.data(), avail, &read);
                result.append(buf.data(), read);
            }
        }
        WinHttpCloseHandle(hReq);
    }
    if (hConn) WinHttpCloseHandle(hConn);
    WinHttpCloseHandle(hSess);
    return result;
}

static std::string HttpsGet(const wchar_t* host, const wchar_t* path, const std::string& bearer) {
    std::string result;
    HINTERNET hSess = WinHttpOpen(L"CinemaApp/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                   WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSess) return result;
    HINTERNET hConn = WinHttpConnect(hSess, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
    HINTERNET hReq  = hConn ? WinHttpOpenRequest(hConn, L"GET", path, nullptr,
                                   WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                   WINHTTP_FLAG_SECURE) : nullptr;
    if (hReq) {
        std::wstring authHdr = L"Authorization: Bearer " +
                               std::wstring(bearer.begin(), bearer.end());
        if (WinHttpSendRequest(hReq, authHdr.c_str(), (DWORD)-1, nullptr, 0, 0, 0) &&
            WinHttpReceiveResponse(hReq, nullptr)) {
            DWORD avail = 0;
            while (WinHttpQueryDataAvailable(hReq, &avail) && avail) {
                std::vector<char> buf(avail + 1);
                DWORD read = 0;
                WinHttpReadData(hReq, buf.data(), avail, &read);
                result.append(buf.data(), read);
            }
        }
        WinHttpCloseHandle(hReq);
    }
    if (hConn) WinHttpCloseHandle(hConn);
    WinHttpCloseHandle(hSess);
    return result;
}

// ── Public entry point ────────────────────────────────────────────────────────

OAuthResult DoOAuthFlow(OAuthProvider provider) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return { false, {}, {}, "Winsock init failed." };

    LocalServer srv;
    if (!srv.Start()) {
        WSACleanup();
        return { false, {}, {}, "Could not start local callback server." };
    }

    std::string redirectUri = "http://127.0.0.1:" + std::to_string(srv.port) + "/callback";
    auto [verifier, challenge] = MakePkce();

    std::string       clientId, authUrl;
    const wchar_t    *tokenHost, *tokenPath, *userHost, *userPath;

    if (provider == OAuthProvider::Google) {
        clientId  = GOOGLE_CLIENT_ID;
        authUrl   = "https://accounts.google.com/o/oauth2/v2/auth"
                    "?client_id="            + UrlEncode(clientId) +
                    "&redirect_uri="         + UrlEncode(redirectUri) +
                    "&response_type=code"
                    "&scope=openid%20email%20profile"
                    "&code_challenge="       + challenge +
                    "&code_challenge_method=S256"
                    "&access_type=online";
        tokenHost = L"oauth2.googleapis.com";
        tokenPath = L"/token";
        userHost  = L"www.googleapis.com";
        userPath  = L"/oauth2/v3/userinfo";
    } else { // Microsoft
        clientId  = MICROSOFT_CLIENT_ID;
        authUrl   = "https://login.microsoftonline.com/common/oauth2/v2.0/authorize"
                    "?client_id="            + UrlEncode(clientId) +
                    "&redirect_uri="         + UrlEncode(redirectUri) +
                    "&response_type=code"
                    "&scope=openid%20email%20profile%20User.Read"
                    "&code_challenge="       + challenge +
                    "&code_challenge_method=S256";
        tokenHost = L"login.microsoftonline.com";
        tokenPath = L"/common/oauth2/v2.0/token";
        userHost  = L"graph.microsoft.com";
        userPath  = L"/v1.0/me";
    }

    ShellExecuteA(nullptr, "open", authUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);

    std::string code = srv.WaitForCode(300);
    srv.Close();
    WSACleanup();

    if (code.empty())
        return { false, {}, {}, "Login cancelled or timed out." };

    // Exchange authorization code for access token
    std::string body =
        "grant_type=authorization_code"
        "&code="          + UrlEncode(code) +
        "&redirect_uri="  + UrlEncode(redirectUri) +
        "&client_id="     + UrlEncode(clientId) +
        "&code_verifier=" + UrlEncode(verifier);

    std::string tokenResp = HttpsPost(tokenHost, tokenPath, body);
    std::string accessToken = JsonStr(tokenResp, "access_token");
    if (accessToken.empty())
        return { false, {}, {}, "Token exchange failed." };

    // Fetch user profile
    std::string userResp = HttpsGet(userHost, userPath, accessToken);

    std::string email = JsonStr(userResp, "email");
    std::string name  = JsonStr(userResp, "name");
    // Microsoft Graph uses different field names
    if (email.empty()) email = JsonStr(userResp, "mail");
    if (email.empty()) email = JsonStr(userResp, "userPrincipalName");
    if (name.empty())  name  = JsonStr(userResp, "displayName");

    if (email.empty())
        return { false, {}, {}, "Could not retrieve email from provider." };

    return { true, email, name, {} };
}
