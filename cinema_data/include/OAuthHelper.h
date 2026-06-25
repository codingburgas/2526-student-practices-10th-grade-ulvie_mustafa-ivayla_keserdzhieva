#pragma once
#include <string>

enum class OAuthProvider { Google, Microsoft };

struct OAuthResult {
    bool        success = false;
    std::string email;
    std::string name;
    std::string error;
};

// Full OAuth 2.0 PKCE flow: opens the system browser, spins up a local
// callback server, exchanges the code for a token, and returns the user's
// email + display name.  Blocks up to 5 minutes — call from a background thread.
OAuthResult DoOAuthFlow(OAuthProvider provider);
