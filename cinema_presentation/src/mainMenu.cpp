#include "imgui.h"
#include <d3d11.h>
#include <string>
#include <ctime>
#include <cmath>
#include "UserService.h"
#include <future>
#include <chrono>

// Palette.
static constexpr ImU32 C_BG     = IM_COL32( 21,  21,  21, 255); // #151515
static constexpr ImU32 C_LOGO   = IM_COL32(163,  49,  82, 255); // #A33152
static constexpr ImU32 C_MUTED  = IM_COL32(144, 136, 144, 255); // #908890
static constexpr ImU32 C_ACCENT = IM_COL32(233, 131, 160, 255); // #E983A0
static constexpr ImU32 C_TITLE  = IM_COL32(236,  64, 113, 255); // #EC4071
static constexpr ImU32 C_UDLINE = IM_COL32(142,  84, 100, 255); // #8E5464
static constexpr ImU32 C_SRCHBG = IM_COL32( 28,  28,  28, 255); // #1C1C1C

// Smooth gradient shorthand (wraps AddRectFilledMultiColor)
static void GradRect(ImDrawList* dl, ImVec2 tl, ImVec2 br,
                     ImU32 cTL, ImU32 cTR, ImU32 cBR, ImU32 cBL) {
    dl->AddRectFilledMultiColor(tl, br, cTL, cTR, cBR, cBL);
}

// ── Fallback icon helpers ─────────────────────────────────────────────────────

static void DrawHeart(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    float s  = r * 0.52f;
    // Shift up by s*0.22 so the heart's bounding box is centred on c
    float cy = c.y - s * 0.22f;
    dl->AddCircleFilled({ c.x - s * 0.52f, cy - s * 0.15f }, s * 0.56f, col, 20);
    dl->AddCircleFilled({ c.x + s * 0.52f, cy - s * 0.15f }, s * 0.56f, col, 20);
    dl->AddTriangleFilled(
        { c.x - s * 1.05f, cy + s * 0.05f },
        { c.x + s * 1.05f, cy + s * 0.05f },
        { c.x,             cy + s * 1.15f }, col);
}

static void DrawPlay(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    // Triangle sized so its bounding box matches DrawHeart at the same r value
    // Centroid = c: (cx-0.45r + cx-0.45r + cx+0.69r)/3 = cx - 0.07r = c.x  ✓
    float h  = r * 0.50f;
    float cx = c.x + r * 0.07f;
    dl->AddTriangleFilled(
        { cx - r * 0.45f, c.y - h },
        { cx - r * 0.45f, c.y + h },
        { cx + r * 0.69f, c.y }, col);
}

static void DrawChevronLeft(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    float s = r * 0.42f;
    dl->AddLine({ c.x + s * 0.35f, c.y - s }, { c.x - s * 0.35f, c.y }, col, 2.5f);
    dl->AddLine({ c.x - s * 0.35f, c.y },     { c.x + s * 0.35f, c.y + s }, col, 2.5f);
}

static void DrawChevronRight(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    float s = r * 0.42f;
    dl->AddLine({ c.x - s * 0.35f, c.y - s }, { c.x + s * 0.35f, c.y }, col, 2.5f);
    dl->AddLine({ c.x + s * 0.35f, c.y },     { c.x - s * 0.35f, c.y + s }, col, 2.5f);
}

static void DrawSearch(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    ImVec2 oc = { c.x - r * 0.08f, c.y - r * 0.08f };
    dl->AddCircle(oc, r * 0.58f, col, 24, 1.8f);
    float a = 0.785f;
    ImVec2 ls = { oc.x + r * 0.58f * cosf(a), oc.y + r * 0.58f * sinf(a) };
    dl->AddLine(ls, { ls.x + r * 0.42f * cosf(a), ls.y + r * 0.42f * sinf(a) }, col, 1.8f);
}

static void DrawPin(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    dl->AddCircleFilled({ c.x, c.y - r * 0.18f }, r * 0.42f, col, 20);
    dl->AddTriangleFilled(
        { c.x - r * 0.35f, c.y - r * 0.05f },
        { c.x + r * 0.35f, c.y - r * 0.05f },
        { c.x,             c.y + r * 0.92f }, col);
    dl->AddCircleFilled({ c.x, c.y - r * 0.18f }, r * 0.18f, C_BG, 16);
}

static void DrawProfile(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    dl->AddCircleFilled(c, r, IM_COL32(48, 48, 56, 255), 32);
    dl->AddCircle(c, r, IM_COL32(72, 72, 82, 255), 32, 1.0f);
    dl->AddCircleFilled({ c.x, c.y - r * 0.24f }, r * 0.34f, col, 20);
    dl->PushClipRect({ c.x - r, c.y - r }, { c.x + r, c.y + r }, true);
    dl->AddCircleFilled({ c.x, c.y + r * 0.62f }, r * 0.52f, col, 24);
    dl->PopClipRect();
}

// ── Poster card with smooth hover animation ───────────────────────────────────
static bool DrawPoster(ImDrawList* dl, ImVec2 pos, ImVec2 sz,
                       ID3D11ShaderResourceView* tex, const char* title,
                       ID3D11ShaderResourceView* playTex, float& hoverT) {
    constexpr float CR = 7.0f;

    bool playClicked = false;

    if (tex)
        dl->AddImageRounded((ImTextureID)tex, pos, { pos.x + sz.x, pos.y + sz.y },
            { 0, 0 }, { 1, 1 }, IM_COL32(255, 255, 255, 255), CR);
    else
        dl->AddRectFilled(pos, { pos.x + sz.x, pos.y + sz.y }, IM_COL32(34, 34, 44, 255), CR);

    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton(("p_" + std::string(title)).c_str(), sz);
    bool hov = ImGui::IsItemHovered();

    float target = hov ? 1.0f : 0.0f;
    hoverT += (target - hoverT) * 0.15f;
    if (hoverT < 0.004f) hoverT = 0.0f;
    if (hoverT > 0.996f) hoverT = 1.0f;

    if (hoverT > 0.001f) {
        float t = hoverT;
        int   a = (int)(t * 255);

        // Overlay tint
        dl->AddRectFilled(pos, { pos.x + sz.x, pos.y + sz.y },
            IM_COL32(0, 0, 0, (int)(t * 130)), CR);

        // Smooth gradient: transparent → deep black, bottom 65%
        GradRect(dl,
            { pos.x, pos.y + sz.y * 0.35f }, { pos.x + sz.x, pos.y + sz.y },
            IM_COL32(0,0,0,0), IM_COL32(0,0,0,0),
            IM_COL32(0,0,0,(int)(t*245)), IM_COL32(0,0,0,(int)(t*245)));

        // Buttons and title scale up smoothly as hover progresses
        float bR  = 14.0f + t * 5.0f;   // 14 → 19 px radius
        float FS  = 14.0f + t * 12.0f;  // 14 → 26 px font
        float bY  = pos.y + sz.y - bR - 14.0f;
        ImVec2 hC = { pos.x + sz.x * 0.32f, bY };
        ImVec2 pC = { pos.x + sz.x * 0.68f, bY };
        ImU32  bA = IM_COL32(236, 64, 113, a);
        dl->AddCircleFilled(hC, bR, bA, 32);
        dl->AddCircleFilled(pC, bR, bA, 32);

        // Both icons use the same 0.58f factor — DrawPlay is shaped to match DrawHeart visually
        ImU32 iconCol = IM_COL32(255, 255, 255, a);
        DrawHeart(dl, hC, bR * 0.58f, iconCol);
        if (playTex)
            dl->AddImage((ImTextureID)playTex,
                { pC.x - bR * 0.58f, pC.y - bR * 0.58f },
                { pC.x + bR * 0.58f, pC.y + bR * 0.58f });
        else
            DrawPlay(dl, pC, bR * 0.58f, iconCol);

        // Title text above buttons, also scales
        ImVec2 tSz = ImGui::GetFont()->CalcTextSizeA(FS, sz.x - 16.0f, 0.0f, title);
        float  tX  = pos.x + (sz.x - tSz.x) * 0.5f;
        float  tY  = bY - bR - 8.0f - FS;
        dl->AddText(ImGui::GetFont(), FS, { tX, tY },
            IM_COL32(233, 131, 160, a), title);

        // Accent border
        dl->AddRect(pos, { pos.x + sz.x, pos.y + sz.y },
            IM_COL32(233, 131, 160, (int)(t * 185)), CR, 0, 1.5f);

        // Invisible button over the play circle to catch clicks
        ImGui::SetCursorScreenPos({ pC.x - bR, pC.y - bR });
        ImGui::InvisibleButton(("pp_" + std::string(title)).c_str(), { bR * 2, bR * 2 });
        if (ImGui::IsItemClicked()) playClicked = true;
    }
    return playClicked;
}

static std::string TodayStr() {
    time_t t = time(nullptr);
    struct tm tm; localtime_s(&tm, &t);
    char buf[64]; strftime(buf, sizeof(buf), "%B %d", &tm);
    return buf;
}

// ── Auth modal icon helpers ───────────────────────────────────────────────────

static void DrawGoogleIcon(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    const int N = 18;
    ImVec2 pts[18];
    for (int i = 0; i < N; i++) {
        float a = (80.0f + i * 295.0f / (N - 1)) * 3.14159f / 180.0f;
        pts[i] = { c.x + cosf(a) * r, c.y + sinf(a) * r };
    }
    dl->AddPolyline(pts, N, col, ImDrawFlags_None, 1.5f);
    dl->AddLine({ c.x, c.y }, { c.x + r, c.y }, col, 1.5f);
    dl->AddLine({ c.x, c.y - r * 0.45f }, { c.x, c.y + r * 0.45f }, col, 1.5f);
}

static void DrawAppleIcon(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    ImVec2 pts[8] = {
        { c.x - r * 0.62f, c.y - r * 0.22f },
        { c.x - r * 0.78f, c.y + r * 0.28f },
        { c.x - r * 0.52f, c.y + r * 0.88f },
        { c.x,             c.y + r * 0.98f  },
        { c.x + r * 0.52f, c.y + r * 0.88f },
        { c.x + r * 0.78f, c.y + r * 0.28f },
        { c.x + r * 0.62f, c.y - r * 0.22f },
        { c.x,             c.y - r * 0.12f  },
    };
    dl->AddConvexPolyFilled(pts, 8, col);
    dl->AddLine({ c.x + r * 0.18f, c.y - r * 0.12f },
                { c.x + r * 0.42f, c.y - r * 0.72f }, col, 1.5f);
}

static void DrawWindowsIcon(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    float s = r * 0.38f, g = r * 0.10f;
    dl->AddRectFilled({ c.x - s - g, c.y - s - g }, { c.x - g,     c.y - g     }, col);
    dl->AddRectFilled({ c.x + g,     c.y - s - g }, { c.x + s + g, c.y - g     }, col);
    dl->AddRectFilled({ c.x - s - g, c.y + g     }, { c.x - g,     c.y + s + g }, col);
    dl->AddRectFilled({ c.x + g,     c.y + g     }, { c.x + s + g, c.y + s + g }, col);
}

// ═════════════════════════════════════════════════════════════════════════════
// MAIN RENDER
// ═════════════════════════════════════════════════════════════════════════════
// Center-crop an image to fill a rectangle (like CSS background-size: cover)
static void AddImageCover(ImDrawList* dl, ImTextureID tex,
                          ImVec2 tl, ImVec2 br, int imgW, int imgH, ImU32 col) {
    if (imgW <= 0 || imgH <= 0) { dl->AddImage(tex, tl, br, {0,0}, {1,1}, col); return; }
    float dw = br.x - tl.x, dh = br.y - tl.y;
    float sx = dw / imgW, sy = dh / imgH;
    float s  = sx > sy ? sx : sy;
    float u0 = (imgW - dw / s) * 0.5f / imgW, v0 = (imgH - dh / s) * 0.5f / imgH;
    dl->AddImage(tex, tl, br, {u0, v0}, {1.0f - u0, 1.0f - v0}, col);
}

void RenderMainMenu(
    ID3D11ShaderResourceView** heroBanners, int* heroWidths, int* heroHeights, int& currentHeroIndex,
    ID3D11ShaderResourceView** posters,     int* /*posterWidths*/, int* /*posterHeights*/, int& selectedMovieIndex,
    ID3D11ShaderResourceView*  playIconTex,     int /*playIconW*/,  int /*playIconH*/,
    ID3D11ShaderResourceView*  favoriteIconTex, int /*favIconW*/,   int /*favIconH*/,
    ID3D11ShaderResourceView*  leftArrowTex,    int /*leftArrW*/,   int /*leftArrH*/,
    ID3D11ShaderResourceView*  rightArrowTex,   int /*rightArrW*/,  int /*rightArrH*/,
    bool& outShowModal,
    ID3D11ShaderResourceView* blurBgSrv,
    ID3D11ShaderResourceView* googleIconTex,
    ID3D11ShaderResourceView* appleIconTex,
    ID3D11ShaderResourceView* msIconTex,
    UserService* userSvc) {

    // Login modal state
    static bool s_showLoginModal  = false;
    static int  s_modalOpenFrame  = -9999;
    static int  s_loginTab        = 0;   // 0 = Log In, 1 = Sign Up
    static char s_nameBuf[256]    = {};
    static char s_surnameBuf[256] = {};
    static char s_unameBuf[256]    = {};
    static char s_passBuf[256]     = {};
    static char s_emailBuf[256]    = {};
    static char s_companyBuf[256]  = {};
    static char s_cityBuf[256]     = {};
    static char s_verifyBuf[32]    = {};
    static int               s_signupRole  = 0;    // 0=customer  1=admin
    static bool              s_codeSent    = false;
    static int               s_authAction  = 0;    // 1=sendCode  2=register/login
    static int               s_resultCode  = 0;    // 0=none  1=ok  -1=fail  -2=smtp fail  -3=validation
    static char              s_validMsg[256] = {};
    static bool              s_authPending = false;
    static std::future<bool> s_authFuture;
    static bool              s_loggedIn    = false;

    // Per-poster hover animation values (persistent across frames)
    static float s_hoverT[9] = {};

    // Banner fade-in (smooth step, 400 ms)
    static int   s_animIdx  = -1;
    static float s_bannerA  = 1.0f;
    if (s_animIdx != currentHeroIndex) { s_animIdx = currentHeroIndex; s_bannerA = 0.0f; }
    s_bannerA += ImGui::GetIO().DeltaTime / 0.40f;
    if (s_bannerA > 1.0f) s_bannerA = 1.0f;
    float bannerT = s_bannerA * s_bannerA * (3.0f - 2.0f * s_bannerA); // smoothstep

    ImGuiIO& io  = ImGui::GetIO();
    ImFont** F   = io.Fonts->Fonts.Data;
    float    winW = io.DisplaySize.x;

    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(21/255.f, 21/255.f, 21/255.f, 1.f));
    ImGui::Begin("CinemaSystem", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize   |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove     |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings);

    ImVec2 orig = { ImGui::GetWindowPos().x,
                    ImGui::GetWindowPos().y - ImGui::GetScrollY() };
    ImDrawList* dl = ImGui::GetWindowDrawList();

    if (!s_showLoginModal) {
    // ─────────────────────────────────────────────────────────────────────────
    // NAVBAR
    // ─────────────────────────────────────────────────────────────────────────
    constexpr float NAV_H = 64.0f;
    dl->AddRectFilled(orig, { orig.x + winW, orig.y + NAV_H }, C_BG);
    dl->AddLine({ orig.x, orig.y + NAV_H }, { orig.x + winW, orig.y + NAV_H },
                IM_COL32(52, 52, 58, 255), 1.0f);

    // Logo
    ImGui::PushFont(F[3]); // Geologica 24px
    float logoY = orig.y + (NAV_H - 24.0f) * 0.5f;
    ImVec2 logoPos = { orig.x + 36.0f, logoY };
    dl->AddText(logoPos, C_LOGO, "CINEMA");
    float logoW = ImGui::CalcTextSize("CINEMA").x;
    ImGui::PopFont();

    // Nav items
    ImGui::PushFont(F[0]); // Inter 18px
    const char* navItems[] = { "Home", "Your Tickets", "Schedule" };
    int activeNav = 0;
    float nx = logoPos.x + logoW + 32.0f;
    for (int i = 0; i < 3; i++) {
        ImVec2 tSz = ImGui::CalcTextSize(navItems[i]);
        float  ty  = orig.y + (NAV_H - tSz.y) * 0.5f;
        bool   actv = (i == activeNav);

        dl->AddText({ nx, ty }, actv ? IM_COL32(255,255,255,255) : C_MUTED, navItems[i]);

        // Underline 4 px below text baseline — tight, no large gap
        float ulY = ty + tSz.y + 4.0f;
        if (actv) {
            dl->AddLine({ nx, ulY }, { nx + tSz.x, ulY }, C_UDLINE, 1.5f);
        } else {
            ImGui::SetCursorScreenPos({ nx - 4.0f, orig.y + 8.0f });
            ImGui::InvisibleButton(("nav_" + std::string(navItems[i])).c_str(),
                                   { tSz.x + 8.0f, NAV_H - 16.0f });
            if (ImGui::IsItemHovered())
                dl->AddLine({ nx, ulY }, { nx + tSz.x, ulY }, IM_COL32(142,84,100,85), 1.0f);
        }
        nx += tSz.x + 42.0f;
    }
    ImGui::PopFont();

    // Right side: Profile icon (logged in) or Sign In button
    constexpr float SIGNIN_W = 90.0f, SIGNIN_H = 36.0f;
    float snRight = orig.x + winW - 28.0f;
    float snLeft  = snRight - SIGNIN_W;
    float snTop   = orig.y + (NAV_H - SIGNIN_H) * 0.5f;

    if (s_loggedIn) {
        constexpr float PROF_R = 18.0f;
        ImVec2 profC = { snRight - PROF_R, orig.y + NAV_H * 0.5f };
        ImGui::SetCursorScreenPos({ profC.x - PROF_R, profC.y - PROF_R });
        ImGui::InvisibleButton("profileBtn", { PROF_R * 2, PROF_R * 2 });
        bool profHov = ImGui::IsItemHovered();
        DrawProfile(dl, profC, PROF_R,
            profHov ? IM_COL32(233, 131, 160, 255) : IM_COL32(200, 200, 210, 255));
    } else {
        ImVec2 snTL = { snLeft, snTop };
        ImVec2 snBR = { snRight, snTop + SIGNIN_H };
        ImGui::SetCursorScreenPos(snTL);
        ImGui::InvisibleButton("signInBtn", { SIGNIN_W, SIGNIN_H });
        bool signInHov = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked()) { s_showLoginModal = true; s_modalOpenFrame = ImGui::GetFrameCount(); }
        dl->AddRectFilled(snTL, snBR, signInHov ? C_TITLE : C_LOGO, SIGNIN_H * 0.5f);
        ImGui::PushFont(F[1]); // Inter 15px
        ImVec2 snTxtSz = ImGui::CalcTextSize("Sign In");
        dl->AddText({ snLeft + (SIGNIN_W - snTxtSz.x) * 0.5f, snTop + (SIGNIN_H - snTxtSz.y) * 0.5f },
                    IM_COL32(255, 255, 255, 255), "Sign In");
        ImGui::PopFont();
    }

    // Search bar
    constexpr float SB_W = 300.0f, SB_H = 38.0f;
    float sbX = snLeft - 20.0f - SB_W;
    float sbY = orig.y + (NAV_H - SB_H) * 0.5f;
    dl->AddRectFilled({ sbX, sbY }, { sbX + SB_W, sbY + SB_H }, C_SRCHBG, 19.0f);
    dl->AddRect(      { sbX, sbY }, { sbX + SB_W, sbY + SB_H }, IM_COL32(55,55,62,255), 19.0f, 0, 1.0f);
    ImGui::PushFont(F[1]); // Inter 15px
    dl->AddText({ sbX + 16.0f, sbY + (SB_H - 15.0f) * 0.5f },
                IM_COL32(144, 136, 144, 115), "Search a movie");
    ImGui::PopFont();
    float srchR = 8.0f;
    DrawSearch(dl, { sbX + SB_W - srchR * 2.8f, sbY + SB_H * 0.5f }, srchR, IM_COL32(144,136,144,145));

    // ─────────────────────────────────────────────────────────────────────────
    // SECTION HEADER
    // ─────────────────────────────────────────────────────────────────────────
    constexpr float LEFT_PAD = 60.0f;
    float hdrY = orig.y + NAV_H + 34.0f;

    ImGui::PushFont(F[4]); // Geologica 20px
    dl->AddText({ LEFT_PAD, hdrY }, C_ACCENT, "Today's top picks");
    float picksW = ImGui::CalcTextSize("Today's top picks").x;
    ImGui::PopFont();

    ImGui::PushFont(F[2]); // Inter 11px
    std::string dateStr = "Updated " + TodayStr();
    dl->AddText({ LEFT_PAD + picksW + 14.0f, hdrY + 6.0f }, C_UDLINE, dateStr.c_str());
    ImGui::PopFont();

    // ─────────────────────────────────────────────────────────────────────────
    // HERO BANNER
    // ─────────────────────────────────────────────────────────────────────────
    constexpr float HERO_W = 1180.0f, HERO_H = 428.0f;
    float heroX = (winW - HERO_W) * 0.5f;
    float heroY = hdrY + 30.0f;
    ImVec2 heroTL = { heroX,          heroY          };
    ImVec2 heroBR = { heroX + HERO_W, heroY + HERO_H };

    constexpr int BANNERS    = 3;
    const char*   titles[3]  = { "La La Land", "Little Women", "The Little Prince" };

    // Side peeks
    constexpr float PEEK_W = 108.0f;
    float peekH = HERO_H * 0.70f;
    float peekY = heroY + (HERO_H - peekH) * 0.5f;

    auto drawPeek = [&](int idx, float px, bool leftSide) {
        ImVec2 ptl = { px, peekY };
        ImVec2 pbr = { px + PEEK_W, peekY + peekH };
        if (heroBanners && heroBanners[idx])
            AddImageCover(dl, (ImTextureID)heroBanners[idx], ptl, pbr,
                heroWidths[idx], heroHeights[idx], IM_COL32(255,255,255,255));
        else
            dl->AddRectFilled(ptl, pbr, IM_COL32(26, 26, 36, 255));
        dl->AddRectFilled(ptl, pbr, IM_COL32(0, 0, 0, 168)); // darken
        // Fade inner edge into page background
        if (leftSide)
            GradRect(dl, ptl, pbr,
                IM_COL32(21,21,21,255), IM_COL32(0,0,0,0),
                IM_COL32(0,0,0,0),     IM_COL32(21,21,21,255));
        else
            GradRect(dl, ptl, pbr,
                IM_COL32(0,0,0,0),     IM_COL32(21,21,21,255),
                IM_COL32(21,21,21,255),IM_COL32(0,0,0,0));
    };

    int prevIdx = (currentHeroIndex - 1 + BANNERS) % BANNERS;
    int nextIdx = (currentHeroIndex + 1) % BANNERS;
    drawPeek(prevIdx, heroX - PEEK_W - 10.0f, true);
    drawPeek(nextIdx, heroX + HERO_W + 10.0f,  false);

    // Main banner
    dl->PushClipRect(heroTL, heroBR, true);

    if (heroBanners && heroBanners[currentHeroIndex]) {
        ImU32 col = IM_COL32(255, 255, 255, (int)(bannerT * 255));
        AddImageCover(dl, (ImTextureID)heroBanners[currentHeroIndex], heroTL, heroBR,
            heroWidths[currentHeroIndex], heroHeights[currentHeroIndex], col);
    } else {
        dl->AddRectFilled(heroTL, heroBR, IM_COL32(26, 26, 36, 255));
    }

    // Smooth gradient: transparent → rich black over bottom 70%
    GradRect(dl,
        { heroX, heroY + HERO_H * 0.30f }, heroBR,
        IM_COL32(0,0,0,0),   IM_COL32(0,0,0,0),
        IM_COL32(0,0,0,222), IM_COL32(0,0,0,222));

    // Thin side vignettes
    GradRect(dl, heroTL, { heroX + 55.0f, heroY + HERO_H },
        IM_COL32(21,21,21,195), IM_COL32(0,0,0,0),
        IM_COL32(0,0,0,0),     IM_COL32(21,21,21,195));
    GradRect(dl, { heroX + HERO_W - 55.0f, heroY }, heroBR,
        IM_COL32(0,0,0,0),     IM_COL32(21,21,21,195),
        IM_COL32(21,21,21,195),IM_COL32(0,0,0,0));

    // Eclipse curve at top
    dl->AddEllipseFilled({ heroX + HERO_W * 0.5f, heroY },
        { HERO_W * 0.58f, HERO_H * 0.042f }, C_BG, 0, 40);

    // Play + heart buttons
    constexpr float BTN_R = 24.0f;
    float btnBaseY = heroY + HERO_H - 148.0f;
    ImVec2 playC  = { heroX + 48.0f + BTN_R,               btnBaseY };
    ImVec2 heartC = { heroX + 48.0f + BTN_R * 3 + 14.0f,  btnBaseY };

    // Both icons use the same size factor (0.58f) — DrawPlay is tuned to match DrawHeart visually
    dl->AddCircleFilled(playC,  BTN_R, IM_COL32(233,131,160,138), 32);
    if (playIconTex)
        dl->AddImage((ImTextureID)playIconTex,
            { playC.x - BTN_R * 0.58f, playC.y - BTN_R * 0.58f },
            { playC.x + BTN_R * 0.58f, playC.y + BTN_R * 0.58f });
    else
        DrawPlay(dl, playC, BTN_R * 0.58f, IM_COL32(255,255,255,255));
    // hero index → poster/movie index mapping: La La Land=2, Little Women=3, Little Prince=6
    static const int s_heroToMovieIdx[3] = { 2, 3, 6 };

    ImGui::SetCursorScreenPos({ playC.x - BTN_R, playC.y - BTN_R });
    ImGui::InvisibleButton("playBtn", { BTN_R * 2, BTN_R * 2 });
    if (ImGui::IsItemHovered())
        dl->AddCircleFilled(playC, BTN_R, IM_COL32(233,131,160,210), 32);
    if (ImGui::IsItemClicked()) {
        if (s_loggedIn) selectedMovieIndex = s_heroToMovieIdx[currentHeroIndex];
        else { s_showLoginModal = true; s_modalOpenFrame = ImGui::GetFrameCount(); }
    }

    dl->AddCircleFilled(heartC, BTN_R, IM_COL32(233,131,160,138), 32);
    if (favoriteIconTex)
        dl->AddImage((ImTextureID)favoriteIconTex,
            { heartC.x - BTN_R * 0.58f, heartC.y - BTN_R * 0.58f },
            { heartC.x + BTN_R * 0.58f, heartC.y + BTN_R * 0.58f });
    else
        DrawHeart(dl, heartC, BTN_R * 0.58f, IM_COL32(255,255,255,255));
    ImGui::SetCursorScreenPos({ heartC.x - BTN_R, heartC.y - BTN_R });
    ImGui::InvisibleButton("heartBtn", { BTN_R * 2, BTN_R * 2 });
    if (ImGui::IsItemHovered())
        dl->AddCircleFilled(heartC, BTN_R, IM_COL32(233,131,160,210), 32);

    // Movie title
    ImGui::PushFont(F[5]); // Geologica 58px
    dl->AddText({ heroX + 48.0f, heroY + HERO_H - 84.0f }, C_TITLE, titles[currentHeroIndex]);
    ImGui::PopFont();

    dl->PopClipRect();

    // Arrow buttons (drawn after PopClipRect so they render over peeks)
    constexpr float ARR_R = 24.0f;
    ImVec2 leftAC  = { heroX - ARR_R - 4.0f,          heroY + HERO_H * 0.5f };
    ImVec2 rightAC = { heroX + HERO_W + ARR_R + 4.0f, heroY + HERO_H * 0.5f };

    auto drawArrow = [&](ImVec2 c, const char* id, bool left,
                         ID3D11ShaderResourceView* tex) -> bool {
        ImGui::SetCursorScreenPos({ c.x - ARR_R, c.y - ARR_R });
        ImGui::InvisibleButton(id, { ARR_R * 2, ARR_R * 2 });
        bool hov = ImGui::IsItemHovered();
        bool clk = ImGui::IsItemClicked();
        dl->AddCircleFilled(c, ARR_R,
            hov ? IM_COL32(236,64,113,215) : IM_COL32(236,64,113,165), 32);
        if (tex)
            dl->AddImage((ImTextureID)tex,
                { c.x - ARR_R * 0.55f, c.y - ARR_R * 0.55f },
                { c.x + ARR_R * 0.55f, c.y + ARR_R * 0.55f });
        else if (left) DrawChevronLeft(dl,  c, ARR_R, IM_COL32(255,255,255,255));
        else           DrawChevronRight(dl, c, ARR_R, IM_COL32(255,255,255,255));
        return clk;
    };

    if (drawArrow(leftAC,  "leftArrow",  true,  leftArrowTex))
        currentHeroIndex = (currentHeroIndex - 1 + BANNERS) % BANNERS;
    if (drawArrow(rightAC, "rightArrow", false, rightArrowTex))
        currentHeroIndex = (currentHeroIndex + 1) % BANNERS;

    // ─────────────────────────────────────────────────────────────────────────
    // LATEST RELEASES
    // ─────────────────────────────────────────────────────────────────────────
    float latY = heroY + HERO_H + 28.0f;
    ImGui::PushFont(F[4]); // Geologica 20px
    dl->AddText({ LEFT_PAD, latY }, C_ACCENT, "Latest releases");
    ImGui::PopFont();

    // ─────────────────────────────────────────────────────────────────────────
    // POSTER GRID
    // ─────────────────────────────────────────────────────────────────────────
    constexpr float P_W   = 252.0f;
    constexpr float P_H   = 372.0f;
    constexpr float P_GAP = 22.0f;
    constexpr int   COLS  = 5;
    constexpr int   TOTAL = 9;

    float gridW = COLS * P_W + (COLS - 1) * P_GAP;
    float gridX = (winW - gridW) * 0.5f;
    float gridY = latY + 26.0f;

    const char* ptitles[TOTAL] = {
        "10 Things I Hate About You", "Dead Poets Society", "La La Land",
        "Little Women",               "Matilda",            "The Drama",
        "The Little Prince",          "The Princess Diaries","The Terminal"
    };

    for (int i = 0; i < TOTAL; i++) {
        int   row = i / COLS, col = i % COLS;
        float px  = gridX + col * (P_W + P_GAP);
        float py  = gridY + row * (P_H + P_GAP);
        if (DrawPoster(dl, { px, py }, { P_W, P_H },
                (posters && i < TOTAL) ? posters[i] : nullptr,
                ptitles[i], playIconTex, s_hoverT[i])) {
            if (s_loggedIn) selectedMovieIndex = i;
            else { s_showLoginModal = true; s_modalOpenFrame = ImGui::GetFrameCount(); }
        }
    }

    int   rows   = (TOTAL + COLS - 1) / COLS;
    float bottom = gridY + rows * (P_H + P_GAP) + 80.0f;
    ImGui::SetCursorScreenPos({ 0, bottom });
    ImGui::Dummy({ 1, 1 });
    } // end !s_showLoginModal

    // ─────────────────────────────────────────────────────────────────────────
    // LOGIN MODAL
    // ─────────────────────────────────────────────────────────────────────────
    if (s_showLoginModal) {
        float winH = io.DisplaySize.y;

        // Blurred background: captured frame drawn with a 5×5 box-blur spread
        if (blurBgSrv) {
            dl->AddImage((ImTextureID)blurBgSrv,
                { orig.x, orig.y }, { orig.x + winW, orig.y + winH },
                { 0.f, 0.f }, { 1.f, 1.f }, IM_COL32(255, 255, 255, 210));
            constexpr float bR = 3.5f;
            for (int bx = -2; bx <= 2; bx++) {
                for (int by = -2; by <= 2; by++) {
                    if (bx == 0 && by == 0) continue;
                    float u0 = bx * bR / winW, v0 = by * bR / winH;
                    dl->AddImage((ImTextureID)blurBgSrv,
                        { orig.x, orig.y }, { orig.x + winW, orig.y + winH },
                        { u0, v0 }, { 1.f + u0, 1.f + v0 },
                        IM_COL32(255, 255, 255, 16));
                }
            }
        }
        // Dark tint over the blurred background
        dl->AddRectFilled({ orig.x, orig.y }, { orig.x + winW, orig.y + winH },
                          IM_COL32(0, 0, 0, 155));

        // Modal dimensions
        constexpr float M_W = 480.0f;
        float M_H = (s_loginTab == 0) ? 504.0f
                  : (s_signupRole == 0) ? 760.0f   // customer
                  :                       880.0f;   // admin (extra company/city/code/verify)
        float mx = orig.x + (winW - M_W) * 0.5f;
        float my = orig.y + (winH - M_H) * 0.5f;
        ImVec2 mTL = { mx, my }, mBR = { mx + M_W, my + M_H };

        dl->AddRectFilled(mTL, mBR, IM_COL32(22, 22, 26, 255), 14.0f);
        dl->AddRect(mTL, mBR, IM_COL32(55, 55, 62, 255), 14.0f, 0, 1.0f);

        // Close when clicking outside (guard: skip the frame the modal was opened)
        if (ImGui::IsMouseClicked(0) && ImGui::GetFrameCount() > s_modalOpenFrame) {
            ImVec2 mp = ImGui::GetMousePos();
            if (mp.x < mx || mp.x > mx + M_W || mp.y < my || mp.y > my + M_H) {
                s_showLoginModal = false;
                s_resultCode     = 0;
                s_authPending    = false;
                s_codeSent       = false;
                s_authAction     = 0;
                s_validMsg[0]    = 0;
            }
        }

        // ── Tabs ─────────────────────────────────────────────────────────────
        float tabY = my + 32.0f;
        ImGui::PushFont(F[0]); // Inter 18px
        ImVec2 liSz = ImGui::CalcTextSize("Log In");
        ImVec2 suSz = ImGui::CalcTextSize("Sign Up");
        float  liX  = mx + M_W * 0.36f - liSz.x * 0.5f;
        float  suX  = mx + M_W * 0.64f - suSz.x * 0.5f;

        ImGui::SetCursorScreenPos({ liX - 8.0f, tabY - 4.0f });
        ImGui::InvisibleButton("tabLI", { liSz.x + 16.0f, liSz.y + 8.0f });
        if (ImGui::IsItemClicked()) { s_loginTab = 0; s_resultCode = 0; s_codeSent = false; s_validMsg[0] = 0; }
        dl->AddText({ liX, tabY },
            (s_loginTab == 0 || ImGui::IsItemHovered()) ? IM_COL32(255,255,255,255) : C_MUTED,
            "Log In");
        if (s_loginTab == 0)
            dl->AddLine({ liX, tabY + liSz.y + 5.0f },
                        { liX + liSz.x, tabY + liSz.y + 5.0f }, C_TITLE, 2.0f);

        ImGui::SetCursorScreenPos({ suX - 8.0f, tabY - 4.0f });
        ImGui::InvisibleButton("tabSU", { suSz.x + 16.0f, suSz.y + 8.0f });
        if (ImGui::IsItemClicked()) { s_loginTab = 1; s_resultCode = 0; s_codeSent = false; s_validMsg[0] = 0; }
        dl->AddText({ suX, tabY },
            (s_loginTab == 1 || ImGui::IsItemHovered()) ? IM_COL32(255,255,255,255) : C_MUTED,
            "Sign Up");
        if (s_loginTab == 1)
            dl->AddLine({ suX, tabY + suSz.y + 5.0f },
                        { suX + suSz.x, tabY + suSz.y + 5.0f }, C_TITLE, 2.0f);
        ImGui::PopFont();

        // ── Input fields ──────────────────────────────────────────────────────
        float fieldX = mx + 28.0f;
        float fieldW = M_W - 56.0f;
        float curY   = tabY + liSz.y + 28.0f;

        ImGui::PushFont(F[1]); // Inter 15px
        ImGui::PushStyleColor(ImGuiCol_FrameBg,        ImVec4(38/255.f, 38/255.f, 44/255.f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(48/255.f, 48/255.f, 54/255.f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  ImVec4(48/255.f, 48/255.f, 54/255.f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_Border,         ImVec4(75/255.f, 75/255.f, 85/255.f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_Text,           ImVec4(0.90f, 0.90f, 0.92f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_TextDisabled,   ImVec4(0.44f, 0.44f, 0.50f, 1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(12.0f, 10.0f));

        // Sign Up only: Name | Surname side by side
        if (s_loginTab == 1) {
            float halfW = (fieldW - 12.0f) * 0.5f;
            dl->AddText({ fieldX,                 curY }, C_MUTED, "Name");
            dl->AddText({ fieldX + halfW + 12.0f, curY }, C_MUTED, "Surname");
            curY += ImGui::CalcTextSize("Name").y + 6.0f;
            ImGui::SetCursorScreenPos({ fieldX, curY });
            ImGui::SetNextItemWidth(halfW);
            ImGui::InputTextWithHint("##name", "Enter your Name", s_nameBuf, sizeof(s_nameBuf));
            ImGui::SetCursorScreenPos({ fieldX + halfW + 12.0f, curY });
            ImGui::SetNextItemWidth(halfW);
            ImGui::InputTextWithHint("##surname", "Enter your Surname", s_surnameBuf, sizeof(s_surnameBuf));
            curY += 40.0f + 14.0f;
        }

        dl->AddText({ fieldX, curY }, C_MUTED, "Username");
        curY += ImGui::CalcTextSize("Username").y + 6.0f;
        ImGui::SetCursorScreenPos({ fieldX, curY });
        ImGui::SetNextItemWidth(fieldW);
        ImGui::InputTextWithHint("##uname", "Enter your username",
                                 s_unameBuf, sizeof(s_unameBuf));
        curY += 40.0f + 14.0f;

        dl->AddText({ fieldX, curY }, C_MUTED, "Password");
        curY += ImGui::CalcTextSize("Password").y + 6.0f;
        ImGui::SetCursorScreenPos({ fieldX, curY });
        ImGui::SetNextItemWidth(fieldW);
        ImGui::InputTextWithHint("##pass", "Enter your password",
                                 s_passBuf, sizeof(s_passBuf),
                                 ImGuiInputTextFlags_Password);
        curY += 40.0f + 14.0f;

        // ── Sign Up extra fields ───────────────────────────────────────────────
        if (s_loginTab == 1) {
            // Email
            dl->AddText({ fieldX, curY }, C_MUTED, "Email");
            curY += ImGui::CalcTextSize("Email").y + 6.0f;
            ImGui::SetCursorScreenPos({ fieldX, curY });
            ImGui::SetNextItemWidth(fieldW);
            ImGui::InputTextWithHint("##email", "Enter your email", s_emailBuf, sizeof(s_emailBuf));
            curY += 40.0f + 14.0f;

            // Role selector
            dl->AddText({ fieldX, curY }, C_MUTED, "Choose your role");
            curY += ImGui::CalcTextSize("Role").y + 6.0f;
            float halfW = (fieldW - 12.0f) * 0.5f;

            // Customer button
            ImVec2 rcTL = { fieldX, curY }, rcBR = { fieldX + halfW, curY + 36.0f };
            ImGui::SetCursorScreenPos(rcTL);
            ImGui::InvisibleButton("roleCustomer", { halfW, 36.0f });
            if (ImGui::IsItemClicked()) { s_signupRole = 0; s_resultCode = 0; s_codeSent = false; s_validMsg[0] = 0; }
            bool rcHov = ImGui::IsItemHovered();
            dl->AddRectFilled(rcTL, rcBR,
                (s_signupRole == 0) ? C_TITLE : (rcHov ? IM_COL32(50,50,58,255) : IM_COL32(38,38,44,255)), 8.0f);
            dl->AddRect(rcTL, rcBR, (s_signupRole == 0) ? C_TITLE : IM_COL32(75,75,85,255), 8.0f, 0, 1.0f);
            {
                ImVec2 lsz = ImGui::CalcTextSize("Customer");
                dl->AddText({ rcTL.x + (halfW - lsz.x) * 0.5f, rcTL.y + (36.0f - lsz.y) * 0.5f },
                             IM_COL32(255,255,255,255), "Customer");
            }

            // Admin button
            ImVec2 raTL = { fieldX + halfW + 12.0f, curY }, raBR = { fieldX + fieldW, curY + 36.0f };
            ImGui::SetCursorScreenPos(raTL);
            ImGui::InvisibleButton("roleAdmin", { halfW, 36.0f });
            if (ImGui::IsItemClicked()) { s_signupRole = 1; s_resultCode = 0; s_codeSent = false; s_validMsg[0] = 0; }
            bool raHov = ImGui::IsItemHovered();
            dl->AddRectFilled(raTL, raBR,
                (s_signupRole == 1) ? C_TITLE : (raHov ? IM_COL32(50,50,58,255) : IM_COL32(38,38,44,255)), 8.0f);
            dl->AddRect(raTL, raBR, (s_signupRole == 1) ? C_TITLE : IM_COL32(75,75,85,255), 8.0f, 0, 1.0f);
            {
                ImVec2 lsz = ImGui::CalcTextSize("Admin");
                dl->AddText({ raTL.x + (halfW - lsz.x) * 0.5f, raTL.y + (36.0f - lsz.y) * 0.5f },
                             IM_COL32(255,255,255,255), "Admin");
            }
            curY += 36.0f + 14.0f;

            // Admin-only: Company | City, Send Code, Verification ID
            if (s_signupRole == 1) {
                dl->AddText({ fieldX,                 curY }, C_MUTED, "Company");
                dl->AddText({ fieldX + halfW + 12.0f, curY }, C_MUTED, "City");
                curY += ImGui::CalcTextSize("Company").y + 6.0f;
                ImGui::SetCursorScreenPos({ fieldX, curY });
                ImGui::SetNextItemWidth(halfW);
                ImGui::InputTextWithHint("##company", "Company name", s_companyBuf, sizeof(s_companyBuf));
                ImGui::SetCursorScreenPos({ fieldX + halfW + 12.0f, curY });
                ImGui::SetNextItemWidth(halfW);
                ImGui::InputTextWithHint("##city", "City", s_cityBuf, sizeof(s_cityBuf));
                curY += 40.0f + 12.0f;

                // Send Verification Code button
                ImVec2 scTL = { fieldX, curY }, scBR = { fieldX + fieldW, curY + 36.0f };
                ImGui::SetCursorScreenPos(scTL);
                ImGui::InvisibleButton("sendCodeBtn", { fieldW, 36.0f });
                bool scHov = ImGui::IsItemHovered();
                if (ImGui::IsItemClicked() && userSvc && !s_authPending) {
                    s_resultCode  = 0;
                    s_validMsg[0] = 0;
                    if (!s_emailBuf[0]) {
                        strcpy_s(s_validMsg, "Enter your email before requesting a code.");
                        s_resultCode = -3;
                    } else if (!strchr(s_emailBuf, '@') || !strchr(s_emailBuf, '.')) {
                        strcpy_s(s_validMsg, "Enter a valid email address (e.g. name@company.com).");
                        s_resultCode = -3;
                    } else {
                        s_codeSent    = false;
                        s_authPending = true;
                        s_authAction  = 1;
                        std::string em = s_emailBuf;
                        s_authFuture = std::async(std::launch::async, [=]() -> bool {
                            return userSvc->SendVerificationCode(em);
                        });
                    }
                }
                ImU32 scBg = (s_authPending && s_authAction == 1)
                           ? IM_COL32(55,55,65,255)
                           : scHov ? IM_COL32(50,50,58,255) : IM_COL32(38,38,44,255);
                dl->AddRectFilled(scTL, scBR, scBg, 8.0f);
                dl->AddRect(scTL, scBR, IM_COL32(75,75,85,255), 8.0f, 0, 1.0f);
                {
                    const char* scLbl = (s_authPending && s_authAction==1) ? "Sending..." : "Send Verification Code";
                    ImVec2 lsz = ImGui::CalcTextSize(scLbl);
                    dl->AddText({ scTL.x + (fieldW - lsz.x) * 0.5f, scTL.y + (36.0f - lsz.y) * 0.5f },
                                IM_COL32(200,200,210,255), scLbl);
                }
                curY += 36.0f + 8.0f;

                // Code sent confirmation
                if (s_codeSent) {
                    char sentTxt[300];
                    snprintf(sentTxt, sizeof(sentTxt), "Code sent to %s (valid 15 min)", s_emailBuf);
                    ImVec2 stSz = ImGui::CalcTextSize(sentTxt);
                    dl->AddText({ fieldX + (fieldW - stSz.x) * 0.5f, curY },
                                IM_COL32(100, 220, 130, 255), sentTxt);
                    curY += stSz.y + 8.0f;
                }

                // Verification ID field
                dl->AddText({ fieldX, curY }, C_MUTED, "Verification ID");
                curY += ImGui::CalcTextSize("Verification ID").y + 6.0f;
                ImGui::SetCursorScreenPos({ fieldX, curY });
                ImGui::SetNextItemWidth(fieldW);
                ImGui::InputTextWithHint("##verify", "Enter the code from your email",
                                         s_verifyBuf, sizeof(s_verifyBuf));
                curY += 40.0f + 14.0f;
            }
        }

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(6);
        ImGui::PopFont();

        // ── Submit button ─────────────────────────────────────────────────────
        // Poll background task (send-code OR register/login)
        if (s_authPending && s_authFuture.valid() &&
            s_authFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
            bool ok = s_authFuture.get();
            if (s_authAction == 1) {        // send-code result
                s_codeSent = ok;
                if (!ok) {
                    const auto& e = userSvc ? userSvc->GetLastError() : std::string{};
                    strcpy_s(s_validMsg, e.empty() ? "Failed to send code. Check SMTP config." : e.c_str());
                    s_resultCode = -2;
                } else {
                    s_resultCode = 0;
                }
            } else if (s_authAction == 2) {  // register / login result
                s_resultCode = ok ? 1 : -1;
                if (ok && s_loginTab == 0) {
                    s_loggedIn       = true;
                    s_showLoginModal = false;
                }
                if (!ok && userSvc) {
                    const auto& e = userSvc->GetLastError();
                    strcpy_s(s_validMsg, e.empty()
                        ? (s_loginTab == 0 ? "Incorrect username or password."
                                           : "Registration failed. Username may already be taken.")
                        : e.c_str());
                }
            } else if (s_authAction == 3) {  // OAuth result
                s_resultCode = ok ? 1 : -1;
                if (ok) {
                    s_loggedIn       = true;
                    s_showLoginModal = false;
                } else if (userSvc) {
                    const auto& e = userSvc->GetLastError();
                    strcpy_s(s_validMsg, e.empty() ? "OAuth login failed." : e.c_str());
                }
            }
            s_authPending = false;
            s_authAction  = 0;
        }

        curY += 6.0f;
        ImVec2 subTL = { fieldX, curY }, subBR = { fieldX + fieldW, curY + 44.0f };
        ImGui::SetCursorScreenPos(subTL);
        ImGui::InvisibleButton("submitBtn", { fieldW, 44.0f });
        bool subHov = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked() && userSvc && !s_authPending) {
            s_resultCode  = 0;
            s_validMsg[0] = 0;

            // Per-field validation — sets s_resultCode = -3 and s_validMsg on first error found
            const char* err = nullptr;
            if (s_loginTab == 0) {  // Log In
                if      (!s_unameBuf[0]) err = "Username is required.";
                else if (!s_passBuf[0])  err = "Password is required.";
            } else {                // Sign Up
                if      (!s_nameBuf[0])                              err = "Name is required.";
                else if (!s_surnameBuf[0])                           err = "Surname is required.";
                else if (!s_unameBuf[0])                             err = "Username is required.";
                else if (strlen(s_passBuf) < 6)                      err = "Password must be at least 6 characters.";
                else if (!s_emailBuf[0])                             err = "Email is required.";
                else if (!strchr(s_emailBuf,'@')||!strchr(s_emailBuf,'.')) err = "Enter a valid email address (e.g. name@company.com).";
                else if (s_signupRole == 1 && !s_companyBuf[0])     err = "Company name is required for admin accounts.";
                else if (s_signupRole == 1 && !s_cityBuf[0])        err = "City is required for admin accounts.";
                else if (s_signupRole == 1 && !s_codeSent)          err = "Request and receive a verification code first.";
                else if (s_signupRole == 1 && !s_verifyBuf[0])      err = "Enter the verification code sent to your email.";
            }

            if (err) {
                strcpy_s(s_validMsg, err);
                s_resultCode = -3;
            } else {
                s_authPending = true;
                s_authAction  = 2;
                std::string nm = s_nameBuf, sn = s_surnameBuf;
                std::string un = s_unameBuf, pw = s_passBuf;
                std::string em = s_emailBuf, co = s_companyBuf, ci = s_cityBuf, vc = s_verifyBuf;
                int tab  = s_loginTab;
                int role = s_signupRole;
                s_authFuture = std::async(std::launch::async, [=]() -> bool {
                    if (tab == 0)  return userSvc->Login(un, pw);
                    if (role == 0) return userSvc->RegisterCustomer(nm, sn, un, pw, em);
                    return userSvc->RegisterAdmin(nm, sn, un, pw, em, co, ci, vc);
                });
            }
        }
        ImU32 subCol = (s_authPending && s_authAction == 2) ? IM_COL32(120, 35, 75, 255)
                     : subHov                               ? IM_COL32(245, 70, 120, 255)
                                                            : C_TITLE;
        dl->AddRectFilled(subTL, subBR, subCol, 8.0f);
        ImGui::PushFont(F[0]);
        const char* subLabel = (s_authPending && s_authAction == 2) ? "..."
                             : (s_loginTab == 0) ? "Log In" : "Sign Up";
        ImVec2 subLblSz = ImGui::CalcTextSize(subLabel);
        dl->AddText({ subTL.x + (fieldW - subLblSz.x) * 0.5f,
                      subTL.y + (44.0f  - subLblSz.y) * 0.5f },
                    IM_COL32(255, 255, 255, 255), subLabel);
        ImGui::PopFont();
        curY += 44.0f + 8.0f;

        // Waiting-for-browser feedback (OAuth in progress)
        if (s_authPending && s_authAction == 3) {
            ImGui::PushFont(F[1]);
            const char* wMsg = "Waiting for browser login...";
            ImVec2 wSz = ImGui::CalcTextSize(wMsg);
            dl->AddText({ fieldX + (fieldW - wSz.x) * 0.5f, curY },
                        IM_COL32(200, 200, 210, 200), wMsg);
            ImGui::PopFont();
            curY += wSz.y + 8.0f;
        }

        // Result feedback
        if (s_resultCode != 0) {
            ImGui::PushFont(F[1]);
            const char* msg;
            ImU32 msgCol;
            if (s_resultCode == 1) {
                msg    = (s_loginTab == 0) ? "Logged in successfully!" : "Account created successfully!";
                msgCol = IM_COL32(100, 220, 130, 255);   // green
            } else if (s_resultCode == -3) {
                msg    = s_validMsg;                     // validation — amber
                msgCol = IM_COL32(255, 190, 60, 255);
            } else {
                msg    = s_validMsg[0] ? s_validMsg      // server / SMTP error — red
                       : (s_resultCode == -2) ? "Failed to send code. Check SMTP config."
                       : (s_loginTab == 0)    ? "Incorrect username or password."
                       :                        "Registration failed.";
                msgCol = IM_COL32(220, 80, 80, 255);
            }
            ImVec2 msgSz = ImGui::CalcTextSize(msg);
            dl->AddText({ fieldX + (fieldW - msgSz.x) * 0.5f, curY }, msgCol, msg);
            ImGui::PopFont();
            curY += msgSz.y + 8.0f;
        } else {
            curY += 8.0f;
        }

        // ── OR divider + social buttons (Log In or Customer Sign Up only) ─────
        if (s_loginTab == 0 || s_signupRole == 0) {
            ImGui::PushFont(F[2]);
            ImVec2 orSz  = ImGui::CalcTextSize("or");
            float  orX   = mx + (M_W - orSz.x) * 0.5f;
            float  lineY = curY + 6.0f;
            dl->AddLine({ fieldX, lineY }, { orX - 8.0f, lineY }, IM_COL32(65, 65, 72, 255), 1.0f);
            dl->AddText({ orX, curY + (12.0f - orSz.y) * 0.5f }, C_MUTED, "or");
            dl->AddLine({ orX + orSz.x + 8.0f, lineY }, { fieldX + fieldW, lineY }, IM_COL32(65, 65, 72, 255), 1.0f);
            ImGui::PopFont();
            curY += 24.0f;

            auto startOAuth = [&](OAuthProvider prov) {
                if (!userSvc || s_authPending) return;
                s_resultCode  = 0;
                s_validMsg[0] = 0;
                s_authPending = true;
                s_authAction  = 3;
                s_authFuture  = std::async(std::launch::async, [=]() -> bool {
                    return userSvc->LoginWithOAuth(prov);
                });
            };

            // Google
            {
                ImVec2 sTL = { fieldX, curY }, sBR = { fieldX + fieldW, curY + 44.0f };
                ImGui::SetCursorScreenPos(sTL);
                ImGui::InvisibleButton("googleBtn", { fieldW, 44.0f });
                bool hov = ImGui::IsItemHovered() && !s_authPending;
                if (ImGui::IsItemClicked()) startOAuth(OAuthProvider::Google);
                dl->AddRectFilled(sTL, sBR,
                    hov ? IM_COL32(50,50,58,255) : IM_COL32(38,38,44,255), 8.0f);
                dl->AddRect(sTL, sBR, IM_COL32(75,75,85,255), 8.0f, 0, 1.0f);
                float icCX = sTL.x + 52.0f, icCY = sTL.y + 22.0f;
                if (googleIconTex)
                    dl->AddImage((ImTextureID)googleIconTex,
                        { icCX - 10.0f, icCY - 10.0f }, { icCX + 10.0f, icCY + 10.0f });
                else
                    DrawGoogleIcon(dl, { icCX, icCY }, 9.0f, IM_COL32(255,255,255,220));
                ImGui::PushFont(F[1]);
                ImVec2 lsz = ImGui::CalcTextSize("Continue with Google");
                dl->AddText({ sTL.x + 70.0f, sTL.y + (44.0f - lsz.y) * 0.5f },
                            IM_COL32(255,255,255,220), "Continue with Google");
                ImGui::PopFont();
                curY += 44.0f + 8.0f;
            }

            // Apple — not supported on Windows (Apple Sign In requires macOS/iOS SDK)
            {
                ImVec2 sTL = { fieldX, curY }, sBR = { fieldX + fieldW, curY + 44.0f };
                ImGui::SetCursorScreenPos(sTL);
                ImGui::InvisibleButton("appleBtn", { fieldW, 44.0f });
                if (ImGui::IsItemClicked()) {
                    strcpy_s(s_validMsg, "Apple Sign In is not supported on Windows.");
                    s_resultCode = -3;
                }
                bool hov = ImGui::IsItemHovered() && !s_authPending;
                dl->AddRectFilled(sTL, sBR,
                    hov ? IM_COL32(50,50,58,255) : IM_COL32(38,38,44,255), 8.0f);
                dl->AddRect(sTL, sBR, IM_COL32(75,75,85,255), 8.0f, 0, 1.0f);
                float icCX = sTL.x + 52.0f, icCY = sTL.y + 22.0f;
                if (appleIconTex)
                    dl->AddImage((ImTextureID)appleIconTex,
                        { icCX - 10.0f, icCY - 10.0f }, { icCX + 10.0f, icCY + 10.0f });
                else
                    DrawAppleIcon(dl, { icCX, icCY }, 9.0f, IM_COL32(255,255,255,220));
                ImGui::PushFont(F[1]);
                ImVec2 lsz = ImGui::CalcTextSize("Continue with Apple");
                dl->AddText({ sTL.x + 70.0f, sTL.y + (44.0f - lsz.y) * 0.5f },
                            IM_COL32(255,255,255,100), "Continue with Apple");
                ImGui::PopFont();
                curY += 44.0f + 8.0f;
            }

            // Microsoft
            {
                ImVec2 sTL = { fieldX, curY }, sBR = { fieldX + fieldW, curY + 44.0f };
                ImGui::SetCursorScreenPos(sTL);
                ImGui::InvisibleButton("teamsBtn", { fieldW, 44.0f });
                bool hov = ImGui::IsItemHovered() && !s_authPending;
                if (ImGui::IsItemClicked()) startOAuth(OAuthProvider::Microsoft);
                dl->AddRectFilled(sTL, sBR,
                    hov ? IM_COL32(50,50,58,255) : IM_COL32(38,38,44,255), 8.0f);
                dl->AddRect(sTL, sBR, IM_COL32(75,75,85,255), 8.0f, 0, 1.0f);
                float icCX = sTL.x + 52.0f, icCY = sTL.y + 22.0f;
                if (msIconTex)
                    dl->AddImage((ImTextureID)msIconTex,
                        { icCX - 10.0f, icCY - 10.0f }, { icCX + 10.0f, icCY + 10.0f });
                else
                    DrawWindowsIcon(dl, { icCX, icCY }, 9.0f, IM_COL32(255,255,255,220));
                ImGui::PushFont(F[1]);
                ImVec2 lsz = ImGui::CalcTextSize("Continue with Microsoft");
                dl->AddText({ sTL.x + 70.0f, sTL.y + (44.0f - lsz.y) * 0.5f },
                            IM_COL32(255,255,255,220), "Continue with Microsoft");
                ImGui::PopFont();
                curY += 44.0f + 8.0f;
            }
        }
    }

    outShowModal = s_showLoginModal;
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}
