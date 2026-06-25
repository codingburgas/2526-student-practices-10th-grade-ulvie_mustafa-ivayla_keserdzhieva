#include "imgui.h"
#include <d3d11.h>
#include <string>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <cstring>

// ── Palette ───────────────────────────────────────────────────────────────────
static constexpr ImU32 MD_BG      = IM_COL32( 21,  21,  21, 255);
static constexpr ImU32 MD_TITLE   = IM_COL32(236,  64, 113, 255);
static constexpr ImU32 MD_ACCENT  = IM_COL32(233, 131, 160, 255);
static constexpr ImU32 MD_MUTED   = IM_COL32(144, 136, 144, 255);
static constexpr ImU32 MD_WHITE   = IM_COL32(255, 255, 255, 255);
static constexpr ImU32 MD_LOGO    = IM_COL32(163,  49,  82, 255);
static constexpr ImU32 MD_BORDER  = IM_COL32( 52,  52,  58, 255);
static constexpr ImU32 MD_CARD    = IM_COL32( 32,  32,  38, 255);

static constexpr ImU32 SEAT_AVAIL    = IM_COL32( 72,  72,  82, 255);
static constexpr ImU32 SEAT_BOOKED   = IM_COL32( 50,  50,  58, 255);
static constexpr ImU32 SEAT_RESERVED = IM_COL32(183,  90, 124, 220);
static constexpr ImU32 SEAT_SELECTED = IM_COL32(236,  64, 113, 255);

// Seat state values (avoid BOOKED/RESERVED/SELECTED — collide with Windows macros)
static constexpr uint8_t ST_AVAIL    = 0;
static constexpr uint8_t ST_BOOKED   = 1;
static constexpr uint8_t ST_RESERVED = 2;
static constexpr uint8_t ST_SELECTED = 3;

// ── Icon helpers ──────────────────────────────────────────────────────────────
static void MDChevL(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    float s = r * 0.45f;
    dl->AddLine({ c.x + s, c.y - s }, { c.x - s, c.y }, col, 2.0f);
    dl->AddLine({ c.x - s, c.y     }, { c.x + s, c.y + s }, col, 2.0f);
}
static void MDChevR(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    float s = r * 0.45f;
    dl->AddLine({ c.x - s, c.y - s }, { c.x + s, c.y }, col, 2.0f);
    dl->AddLine({ c.x + s, c.y     }, { c.x - s, c.y + s }, col, 2.0f);
}
static void MDProfile(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    dl->AddCircleFilled(c, r, IM_COL32(48, 48, 56, 255), 32);
    dl->AddCircle(c, r, IM_COL32(72, 72, 82, 255), 32, 1.0f);
    dl->AddCircleFilled({ c.x, c.y - r * 0.24f }, r * 0.34f, col, 20);
    dl->PushClipRect({ c.x - r, c.y - r }, { c.x + r, c.y + r }, true);
    dl->AddCircleFilled({ c.x, c.y + r * 0.62f }, r * 0.52f, col, 24);
    dl->PopClipRect();
}
static void MDSearch(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    ImVec2 oc = { c.x - r * 0.08f, c.y - r * 0.08f };
    dl->AddCircle(oc, r * 0.58f, col, 24, 1.8f);
    float a = 0.785f;
    ImVec2 ls = { oc.x + r * 0.58f * cosf(a), oc.y + r * 0.58f * sinf(a) };
    dl->AddLine(ls, { ls.x + r * 0.42f * cosf(a), ls.y + r * 0.42f * sinf(a) }, col, 1.8f);
}
static void MDPin(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    dl->AddCircleFilled({ c.x, c.y - r * 0.18f }, r * 0.42f, col, 20);
    dl->AddTriangleFilled(
        { c.x - r * 0.35f, c.y - r * 0.05f },
        { c.x + r * 0.35f, c.y - r * 0.05f },
        { c.x, c.y + r * 0.92f }, col);
    dl->AddCircleFilled({ c.x, c.y - r * 0.18f }, r * 0.18f, MD_BG, 16);
}
static void MDClock(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    dl->AddCircle(c, r, col, 24, 1.5f);
    dl->AddLine(c, { c.x, c.y - r * 0.55f }, col, 1.5f);
    dl->AddLine(c, { c.x + r * 0.42f, c.y + r * 0.25f }, col, 1.5f);
}
static void MDTicketIcon(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    float hw = r, hh = r * 0.62f;
    dl->AddRect({ c.x - hw, c.y - hh }, { c.x + hw, c.y + hh }, col, 3.0f, 0, 1.5f);
    float lx = c.x - hw * 0.18f;
    dl->AddLine({ lx, c.y - hh }, { lx, c.y + hh }, col, 1.5f);
    dl->AddCircle({ lx, c.y - hh }, 2.5f, col, 12);
    dl->AddCircle({ lx, c.y + hh }, 2.5f, col, 12);
}

// ── Movie data ────────────────────────────────────────────────────────────────
struct MovieInfo {
    const char* title;
    int         year;
    const char* duration;
    const char* synopsis;
    float       imdb;
    float       letterbox;
    int         criticScore;
    const char* times[4];
};

static const MovieInfo g_movies[9] = {
    { "10 Things I Hate About You", 1999, "1h 37m",
      "A high-school girl can only date after her sharp-tongued older sister does. "
      "A charming schemer is hired to woo the difficult sister, but finds something "
      "far more genuine than anyone planned.",
      7.3f, 3.9f, 84, { "10:00 AM", "1:15 PM", "4:30 PM", "8:00 PM" } },
    { "Dead Poets Society", 1989, "2h 8m",
      "An unorthodox English teacher at an elite prep school inspires his students "
      "to seize the day, question authority, and discover the transformative "
      "power of poetry. Carpe diem.",
      8.1f, 4.3f, 85, { "11:00 AM", "2:00 PM", "5:15 PM", "9:00 PM" } },
    { "La La Land", 2016, "2h 8m",
      "A jazz pianist and an aspiring actress fall in love while chasing dreams "
      "across Los Angeles. A dazzling ode to ambition, artistry, and the "
      "bittersweet cost of following your heart.",
      8.0f, 4.1f, 91, { "10:30 AM", "1:45 PM", "5:00 PM", "8:30 PM" } },
    { "Little Women", 2019, "2h 15m",
      "The four March sisters navigate love, loss, and independence in post-Civil "
      "War New England. A timeless story of family, creativity, and the quiet "
      "courage it takes to live life on your own terms.",
      7.8f, 3.8f, 95, { "11:15 AM", "2:30 PM", "5:45 PM", "9:15 PM" } },
    { "Matilda", 1996, "1h 38m",
      "A brilliant girl with extraordinary telekinetic powers escapes her neglectful "
      "parents and a terrifying headmistress, finding solace in books, kindness, "
      "and the magic she has always carried within herself.",
      6.9f, 3.5f, 90, { "10:00 AM", "12:30 PM", "3:00 PM", "6:00 PM" } },
    { "The Drama", 2026, "1h 45m",
      "The Drama (2026) is a dark romantic comedy written and directed by Kristoffer "
      "Borgli. It stars Zendaya as Emma and Robert Pattinson as Charlie, a happily "
      "engaged couple whose seemingly perfect life is derailed by a jaw-dropping "
      "confession just one week before their wedding.",
      7.2f, 3.7f, 81, { "11:45 AM", "1:20 PM", "4:45 PM", "10:20 PM" } },
    { "The Little Prince", 2015, "1h 48m",
      "A little girl in a rigidly scheduled world befriends an eccentric aviator "
      "who tells her the story of the Little Prince — a boy who traveled from "
      "his tiny planet to learn what truly matters in life.",
      7.7f, 3.9f, 93, { "10:15 AM", "1:00 PM", "4:15 PM", "7:30 PM" } },
    { "The Princess Diaries", 2001, "1h 51m",
      "An awkward San Francisco teenager discovers she is the heir to the throne "
      "of a small European kingdom. A charming coming-of-age story about identity, "
      "grace under pressure, and daring to believe in yourself.",
      6.3f, 3.3f, 70, { "10:30 AM", "1:30 PM", "4:00 PM", "7:00 PM" } },
    { "The Terminal", 2004, "2h 8m",
      "A man from Eastern Europe arrives at JFK airport only to find his country "
      "no longer exists, leaving him stranded. He turns the terminal into an unlikely "
      "home — proving human warmth can thrive anywhere.",
      7.4f, 3.6f, 61, { "11:00 AM", "2:15 PM", "5:30 PM", "9:00 PM" } },
};

// ── Seat map ──────────────────────────────────────────────────────────────────
// 9 rows × 14 cols.  Col groups: 0-2 = left, 3-10 = center, 11-13 = right
// States: 0=available 1=booked 2=reserved 3=selected
static uint8_t s_seats[9][14];
static int     s_seatMovie = -1;

static void ResetSeats(int movieIdx) {
    if (s_seatMovie == movieIdx) return;
    s_seatMovie = movieIdx;
    static const uint8_t base[9][14] = {
        { 1,1,0,  1,0,2,2,1,0,1,0,  0,1,1 },
        { 0,1,1,  0,2,2,1,2,0,0,1,  1,0,1 },
        { 1,0,1,  1,2,0,1,0,2,1,1,  0,0,1 },
        { 0,0,1,  0,0,1,1,2,2,0,1,  1,0,0 },
        { 1,1,0,  2,0,0,0,0,1,0,0,  0,1,0 },
        { 0,1,0,  0,1,1,2,1,0,1,1,  1,0,1 },
        { 1,0,0,  0,0,0,1,0,0,0,0,  0,0,1 },
        { 0,0,1,  0,1,0,0,1,0,0,0,  1,0,0 },
        { 0,0,0,  0,0,0,0,0,0,0,0,  0,0,0 },
    };
    memcpy(s_seats, base, sizeof(s_seats));
}

static int CountSelected() {
    int n = 0;
    for (int r = 0; r < 9; r++)
        for (int c = 0; c < 14; c++)
            if (s_seats[r][c] == ST_SELECTED) n++;
    return n;
}

// ── Date helpers ──────────────────────────────────────────────────────────────
static const char* s_dayNames[] = { "Sun","Mon","Tue","Wed","Thu","Fri","Sat" };
static const char* s_monthNames[] = {
    "Jan","Feb","Mar","Apr","May","Jun",
    "Jul","Aug","Sep","Oct","Nov","Dec"
};

void RenderMovieDetail(
    int movieIndex, bool& goBack,
    ID3D11ShaderResourceView** posters, int* /*posterWidths*/, int* /*posterHeights*/)
{
    ImGuiIO&    io   = ImGui::GetIO();
    ImFont**    F    = io.Fonts->Fonts.Data;
    float       winW = io.DisplaySize.x;
    float       winH = io.DisplaySize.y;

    int idx = (movieIndex >= 0 && movieIndex < 9) ? movieIndex : 0;
    const MovieInfo& mv = g_movies[idx];

    ResetSeats(idx);

    // Per-movie time selection (persists while on this page)
    static int s_timeIdx[9] = { 0,0,0,0,0,0,0,0,0 };
    int& selTime = s_timeIdx[idx];

    // Date strip state
    static int s_dayOffset = 0; // which day in the 7-day window is selected
    static int s_weekStart = 0; // arrow navigation

    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(21/255.f, 21/255.f, 21/255.f, 1.f));
    ImGui::Begin("MovieDetail", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize   |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove     |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2      orig = ImGui::GetWindowPos();
    ImDrawList* dl   = ImGui::GetWindowDrawList();

    // ── Navbar ────────────────────────────────────────────────────────────────
    constexpr float NAV_H = 64.0f;
    dl->AddRectFilled(orig, { orig.x + winW, orig.y + NAV_H }, MD_BG);
    dl->AddLine({ orig.x, orig.y + NAV_H }, { orig.x + winW, orig.y + NAV_H }, MD_BORDER, 1.0f);

    // Logo
    ImGui::PushFont(F[3]); // Geologica 24px
    float logoY = orig.y + (NAV_H - 24.0f) * 0.5f;
    dl->AddText({ orig.x + 36.0f, logoY }, MD_LOGO, "CINEMA");
    float logoW = ImGui::CalcTextSize("CINEMA").x;
    ImGui::PopFont();

    // Nav items
    ImGui::PushFont(F[0]); // Inter 18px
    const char* navItems[] = { "Home", "Your Tickets", "Schedule" };
    float nx = orig.x + 36.0f + logoW + 32.0f;
    for (int i = 0; i < 3; i++) {
        ImVec2 tsz = ImGui::CalcTextSize(navItems[i]);
        float  ty  = orig.y + (NAV_H - tsz.y) * 0.5f;
        dl->AddText({ nx, ty }, IM_COL32(144,136,144,255), navItems[i]);
        ImGui::SetCursorScreenPos({ nx - 4.0f, orig.y + 8.0f });
        ImGui::InvisibleButton(("nav_d_" + std::string(navItems[i])).c_str(),
                               { tsz.x + 8.0f, NAV_H - 16.0f });
        if (ImGui::IsItemClicked() && i == 0) goBack = true;
        if (ImGui::IsItemHovered())
            dl->AddLine({ nx, ty + tsz.y + 4.0f }, { nx + tsz.x, ty + tsz.y + 4.0f },
                        IM_COL32(142,84,100,85), 1.0f);
        nx += tsz.x + 42.0f;
    }
    ImGui::PopFont();

    // Profile icon (right side — only logged-in users reach this page)
    float profRight = orig.x + winW - 28.0f;
    constexpr float PROF_R = 18.0f;
    ImVec2 profC = { profRight - PROF_R, orig.y + NAV_H * 0.5f };
    ImGui::SetCursorScreenPos({ profC.x - PROF_R, profC.y - PROF_R });
    ImGui::InvisibleButton("profileD", { PROF_R * 2, PROF_R * 2 });
    MDProfile(dl, profC, PROF_R, IM_COL32(200, 200, 210, 255));

    // City selector
    constexpr float CITY_W = 90.0f, CITY_H = 36.0f;
    float cityRight = profC.x - PROF_R - 16.0f;
    float cityLeft  = cityRight - CITY_W;
    float cityTop   = orig.y + (NAV_H - CITY_H) * 0.5f;
    ImGui::SetCursorScreenPos({ cityLeft, cityTop });
    ImGui::InvisibleButton("cityBtn", { CITY_W, CITY_H });
    bool cityHov = ImGui::IsItemHovered();
    ImGui::PushFont(F[1]);
    dl->AddText({ cityLeft + 20.0f, cityTop + (CITY_H - 15.0f) * 0.5f },
                cityHov ? MD_WHITE : MD_MUTED, "City");
    ImGui::PopFont();
    MDPin(dl, { cityLeft + 11.0f, cityTop + CITY_H * 0.5f }, 7.0f, MD_ACCENT);

    // Search bar
    constexpr float SB_W = 280.0f, SB_H = 38.0f;
    float sbX = cityLeft - 16.0f - SB_W;
    float sbY = orig.y + (NAV_H - SB_H) * 0.5f;
    dl->AddRectFilled({ sbX, sbY }, { sbX + SB_W, sbY + SB_H }, IM_COL32(28,28,28,255), 19.0f);
    dl->AddRect(      { sbX, sbY }, { sbX + SB_W, sbY + SB_H }, IM_COL32(55,55,62,255), 19.0f, 0, 1.0f);
    ImGui::PushFont(F[1]);
    dl->AddText({ sbX + 16.0f, sbY + (SB_H - 15.0f) * 0.5f },
                IM_COL32(144,136,144,115), "Search a movie");
    ImGui::PopFont();
    MDSearch(dl, { sbX + SB_W - 14.0f, sbY + SB_H * 0.5f }, 8.0f, IM_COL32(144,136,144,145));

    // ── Date strip ────────────────────────────────────────────────────────────
    constexpr float DS_H = 78.0f;
    float dsY = orig.y + NAV_H;
    dl->AddRectFilled({ orig.x, dsY }, { orig.x + winW, dsY + DS_H }, MD_BG);
    dl->AddLine({ orig.x, dsY + DS_H }, { orig.x + winW, dsY + DS_H }, MD_BORDER, 1.0f);

    // Get today's date
    time_t now = time(nullptr); struct tm today; localtime_s(&today, &now);

    // Calendar icon
    float calX = orig.x + 36.0f, calCY = dsY + DS_H * 0.5f;
    dl->AddRectFilled({ calX, calCY - 12.0f }, { calX + 28.0f, calCY + 12.0f },
                      MD_CARD, 5.0f);
    dl->AddRect({ calX, calCY - 12.0f }, { calX + 28.0f, calCY + 12.0f },
                MD_BORDER, 5.0f, 0, 1.0f);
    dl->AddLine({ calX, calCY - 4.0f }, { calX + 28.0f, calCY - 4.0f }, MD_BORDER, 1.0f);
    dl->AddLine({ calX + 8.0f,  calCY - 16.0f }, { calX + 8.0f,  calCY - 8.0f }, MD_ACCENT, 1.5f);
    dl->AddLine({ calX + 20.0f, calCY - 16.0f }, { calX + 20.0f, calCY - 8.0f }, MD_ACCENT, 1.5f);

    // Day pills
    constexpr float DAY_W = 104.0f, DAY_H = 54.0f, DAY_GAP = 8.0f;
    float dayStartX = orig.x + 80.0f;
    float dayPillY  = dsY + (DS_H - DAY_H) * 0.5f;

    for (int i = 0; i < 7; i++) {
        // Compute date for this slot
        time_t t = now + (time_t)(s_weekStart + i) * 86400;
        struct tm d; localtime_s(&d, &t);

        float px = dayStartX + i * (DAY_W + DAY_GAP);
        bool  sel = (s_dayOffset == i);

        ImVec2 ptl = { px, dayPillY }, pbr = { px + DAY_W, dayPillY + DAY_H };
        ImGui::SetCursorScreenPos(ptl);
        char btnId[16]; snprintf(btnId, sizeof(btnId), "day_%d", i);
        ImGui::InvisibleButton(btnId, { DAY_W, DAY_H });
        if (ImGui::IsItemClicked()) s_dayOffset = i;
        bool hov = ImGui::IsItemHovered();

        ImU32 pillBg  = sel ? MD_CARD : (hov ? IM_COL32(32,32,38,255) : IM_COL32(0,0,0,0));
        ImU32 pillBdr = sel ? MD_TITLE : MD_BORDER;
        dl->AddRectFilled(ptl, pbr, pillBg, 10.0f);
        dl->AddRect(ptl, pbr, pillBdr, 10.0f, 0, sel ? 1.5f : 1.0f);

        // Day name
        ImGui::PushFont(F[1]); // Inter 15px
        const char* dn = s_dayNames[d.tm_wday];
        ImVec2 dnSz = ImGui::CalcTextSize(dn);
        dl->AddText({ px + (DAY_W - dnSz.x) * 0.5f, dayPillY + 8.0f },
                    sel ? MD_MUTED : MD_MUTED, dn);
        ImGui::PopFont();

        // Day number
        ImGui::PushFont(F[3]); // Geologica 24px
        char dnum[4]; snprintf(dnum, sizeof(dnum), "%d", d.tm_mday);
        ImVec2 dnumSz = ImGui::CalcTextSize(dnum);
        dl->AddText({ px + (DAY_W - dnumSz.x) * 0.5f, dayPillY + 26.0f },
                    sel ? MD_WHITE : MD_MUTED, dnum);
        ImGui::PopFont();
    }

    // Week nav arrows
    float arrRightX = dayStartX + 7 * (DAY_W + DAY_GAP) + 8.0f;
    constexpr float ARR_S = 22.0f;
    float arrCY1 = dsY + DS_H * 0.5f - 14.0f;
    float arrCY2 = dsY + DS_H * 0.5f + 14.0f;

    ImGui::SetCursorScreenPos({ arrRightX, arrCY1 - ARR_S * 0.5f });
    ImGui::InvisibleButton("wkR", { ARR_S, ARR_S });
    bool wkRHov = ImGui::IsItemHovered();
    if (ImGui::IsItemClicked()) { s_weekStart++; s_dayOffset = 0; }
    dl->AddRectFilled({ arrRightX, arrCY1 - ARR_S * 0.5f },
                      { arrRightX + ARR_S, arrCY1 + ARR_S * 0.5f },
                      wkRHov ? MD_TITLE : MD_CARD, 5.0f);
    MDChevR(dl, { arrRightX + ARR_S * 0.5f, arrCY1 }, 6.0f, MD_WHITE);

    ImGui::SetCursorScreenPos({ arrRightX, arrCY2 - ARR_S * 0.5f });
    ImGui::InvisibleButton("wkL", { ARR_S, ARR_S });
    bool wkLHov = ImGui::IsItemHovered();
    if (ImGui::IsItemClicked() && s_weekStart > 0) { s_weekStart--; s_dayOffset = 0; }
    dl->AddRectFilled({ arrRightX, arrCY2 - ARR_S * 0.5f },
                      { arrRightX + ARR_S, arrCY2 + ARR_S * 0.5f },
                      wkLHov ? MD_TITLE : MD_CARD, 5.0f);
    MDChevL(dl, { arrRightX + ARR_S * 0.5f, arrCY2 }, 6.0f, MD_WHITE);

    // ── Content area ──────────────────────────────────────────────────────────
    constexpr float BOTTOM_H  = 70.0f;
    constexpr float SIDE_PAD  = 36.0f;
    float contentTop = orig.y + NAV_H + DS_H;
    float contentH   = winH - NAV_H - DS_H - BOTTOM_H;
    float splitX     = winW * 0.55f;  // left panel / right panel boundary

    // Vertical divider
    dl->AddLine({ orig.x + splitX, contentTop },
                { orig.x + splitX, contentTop + contentH }, MD_BORDER, 1.0f);

    // ── LEFT PANEL ────────────────────────────────────────────────────────────
    float lx = orig.x + SIDE_PAD;
    float ly = contentTop + 24.0f;

    // Poster
    constexpr float POST_W = 198.0f, POST_H = 292.0f;
    if (posters && posters[idx])
        dl->AddImageRounded((ImTextureID)posters[idx],
            { lx, ly }, { lx + POST_W, ly + POST_H },
            { 0,0 }, { 1,1 }, MD_WHITE, 10.0f);
    else
        dl->AddRectFilled({ lx, ly }, { lx + POST_W, ly + POST_H },
                          IM_COL32(34,34,44,255), 10.0f);
    dl->AddRect({ lx, ly }, { lx + POST_W, ly + POST_H },
                IM_COL32(233,131,160,55), 10.0f, 0, 1.5f);

    // Info block beside the poster
    float ix  = lx + POST_W + 32.0f;
    float iw  = splitX - SIDE_PAD - POST_W - 32.0f - 16.0f;
    float iy  = ly;

    // Title
    ImGui::PushFont(F[5]); // Geologica 58px
    dl->AddText(ImGui::GetFont(), 44.0f, { ix, iy },
                MD_TITLE, mv.title, nullptr, iw);
    float titleH = ImGui::GetFont()->CalcTextSizeA(44.0f, iw, iw, mv.title).y;
    iy += titleH + 10.0f;
    ImGui::PopFont();

    // Year + duration on one row
    ImGui::PushFont(F[1]); // Inter 15px
    char meta[64]; snprintf(meta, sizeof(meta), "%d", mv.year);
    dl->AddText({ ix, iy }, MD_MUTED, meta);
    float durX = ix + iw - ImGui::CalcTextSize(mv.duration).x;
    dl->AddText({ durX, iy }, MD_MUTED, mv.duration);
    iy += 18.0f + 14.0f;
    ImGui::PopFont();

    // Divider
    dl->AddLine({ ix, iy }, { ix + iw, iy }, MD_BORDER, 1.0f);
    iy += 14.0f;

    // Synopsis
    ImGui::PushFont(F[1]); // Inter 15px
    dl->AddText(ImGui::GetFont(), 13.5f, { ix, iy },
                IM_COL32(185,180,185,255), mv.synopsis, nullptr, iw);
    float synH = ImGui::GetFont()->CalcTextSizeA(13.5f, iw, iw, mv.synopsis).y;
    iy += synH + 18.0f;
    ImGui::PopFont();

    // Ratings row: IMDb | Letterbox | Critic Score
    float ratW = iw / 3.0f;
    const char* ratLabels[] = { "IMDb", "Letterbox", "Critic Score" };
    char ratVals[3][16];
    snprintf(ratVals[0], 16, "%.1f/10", mv.imdb);
    snprintf(ratVals[1], 16, "%.1f",    mv.letterbox);
    snprintf(ratVals[2], 16, "%d%%",    mv.criticScore);

    for (int i = 0; i < 3; i++) {
        float rx = ix + i * ratW;
        ImGui::PushFont(F[1]);
        dl->AddText({ rx, iy }, MD_ACCENT, ratLabels[i]);
        iy += 16.0f + 4.0f;
        ImGui::PushFont(F[3]); // bigger for value
        dl->AddText({ rx, iy }, MD_WHITE, ratVals[i]);
        ImGui::PopFont();
        iy -= 16.0f + 4.0f; // reset for next column
        ImGui::PopFont();
    }
    iy += 16.0f + 26.0f + 12.0f; // advance past ratings block

    // Below-poster area (time + tickets span full left panel width)
    float fullLW = splitX - SIDE_PAD * 2.0f;

    // Divider
    dl->AddLine({ lx, iy }, { lx + fullLW, iy }, MD_BORDER, 1.0f);
    iy += 18.0f;

    // "Selected Time" label with clock icon
    MDClock(dl, { lx + 8.0f, iy + 9.0f }, 8.0f, MD_ACCENT);
    ImGui::PushFont(F[1]);
    dl->AddText({ lx + 22.0f, iy }, MD_ACCENT, "Selected Time");
    iy += 18.0f + 12.0f;

    // Time slot buttons
    constexpr int   N_TIMES = 4;
    float timeW = (fullLW - 3.0f * 10.0f) / N_TIMES;
    constexpr float TIME_H  = 44.0f;

    for (int i = 0; i < N_TIMES; i++) {
        float tx = lx + i * (timeW + 10.0f);
        bool  sel = (selTime == i);
        ImVec2 tTL = { tx, iy }, tBR = { tx + timeW, iy + TIME_H };
        ImGui::SetCursorScreenPos(tTL);
        char tid[16]; snprintf(tid, sizeof(tid), "time_%d", i);
        ImGui::InvisibleButton(tid, { timeW, TIME_H });
        if (ImGui::IsItemClicked()) selTime = i;
        bool hov = ImGui::IsItemHovered();
        dl->AddRectFilled(tTL, tBR,
            sel ? MD_CARD : (hov ? IM_COL32(40,40,46,255) : IM_COL32(0,0,0,0)), 10.0f);
        dl->AddRect(tTL, tBR,
            sel ? MD_TITLE : MD_BORDER, 10.0f, 0, sel ? 2.0f : 1.0f);
        ImVec2 lsz = ImGui::CalcTextSize(mv.times[i]);
        dl->AddText({ tx + (timeW - lsz.x) * 0.5f, iy + (TIME_H - lsz.y) * 0.5f },
                    sel ? MD_WHITE : MD_MUTED, mv.times[i]);
    }
    iy += TIME_H + 18.0f;
    ImGui::PopFont();

    // Divider
    dl->AddLine({ lx, iy }, { lx + fullLW, iy }, MD_BORDER, 1.0f);
    iy += 18.0f;

    // "Select Tickets" label with icon
    MDTicketIcon(dl, { lx + 8.0f, iy + 9.0f }, 9.0f, MD_ACCENT);
    ImGui::PushFont(F[1]);
    dl->AddText({ lx + 22.0f, iy }, MD_ACCENT, "Select Tickets");
    iy += 18.0f + 12.0f;

    // Ticket card — shows first selected seat (if any)
    int selCount = CountSelected();
    if (selCount > 0) {
        // Find first selected seat
        int selRow = -1, selCol = -1;
        for (int r = 0; r < 9 && selRow < 0; r++)
            for (int c = 0; c < 14 && selRow < 0; c++)
                if (s_seats[r][c] == ST_SELECTED) { selRow = r; selCol = c; }

        // Map column to display seat number (skip aisle positions)
        int dispSeat = selCol < 3 ? selCol + 1
                     : selCol < 11 ? selCol - 2
                     : selCol - 7;

        constexpr float CARD_H = 58.0f;
        float cardW = fullLW - 56.0f; // leave room for + button
        ImVec2 cTL = { lx, iy }, cBR = { lx + cardW, iy + CARD_H };
        dl->AddRectFilled(cTL, cBR, MD_CARD, 10.0f);
        dl->AddRect(cTL, cBR, MD_BORDER, 10.0f, 0, 1.0f);

        // Ticket content: BURGAS city | A hall | B row | 08 seat | $15
        float tx   = lx + 14.0f;
        float midY = iy + CARD_H * 0.5f;

        char rowStr[4];  snprintf(rowStr,  sizeof(rowStr),  "%c",  'A' + selRow);
        char seatStr[4]; snprintf(seatStr, sizeof(seatStr), "%02d", dispSeat);

        // Each pair: large text in F[3], subscript label in F[2]
        const char* bigTxts[] = { "BURGAS", "A",      rowStr, seatStr };
        const char* subTxts[] = { " city",  " hall",  " row", " seat" };
        for (int pi = 0; pi < 4; pi++) {
            ImGui::PushFont(F[3]);
            dl->AddText({ tx, midY - 14.0f }, MD_WHITE, bigTxts[pi]);
            float bw = ImGui::CalcTextSize(bigTxts[pi]).x;
            ImGui::PopFont();
            ImGui::PushFont(F[2]);
            dl->AddText({ tx + bw + 2.0f, midY - 4.0f }, MD_MUTED, subTxts[pi]);
            float sw = ImGui::CalcTextSize(subTxts[pi]).x;
            ImGui::PopFont();
            tx += bw + sw + 16.0f;
        }

        // Price
        ImGui::PushFont(F[3]);
        dl->AddText({ tx, midY - 14.0f }, MD_ACCENT, "$");
        float dollarW = ImGui::CalcTextSize("$").x;
        dl->AddText({ tx + dollarW, midY - 14.0f }, MD_WHITE, "15");
        ImGui::PopFont(); // F[3] for price

        // × button inside card
        constexpr float X_R = 12.0f;
        ImVec2 xC = { lx + cardW - X_R - 10.0f, midY };
        ImGui::SetCursorScreenPos({ xC.x - X_R, xC.y - X_R });
        ImGui::InvisibleButton("removeTicket", { X_R * 2, X_R * 2 });
        if (ImGui::IsItemClicked()) {
            // Deselect all selected seats
            for (int r = 0; r < 9; r++)
                for (int c = 0; c < 14; c++)
                    if (s_seats[r][c] == ST_SELECTED) s_seats[r][c] = ST_AVAIL;
        }
        bool xHov = ImGui::IsItemHovered();
        dl->AddText({ xC.x - 4.0f, xC.y - 8.0f },
                    xHov ? MD_TITLE : MD_MUTED, "x");

        // + button (outside card, to the right)
        constexpr float PLUS_R = 20.0f;
        float plusX = lx + cardW + 10.0f;
        ImVec2 pTL = { plusX, iy + (CARD_H - PLUS_R * 2.0f) * 0.5f };
        ImGui::SetCursorScreenPos(pTL);
        ImGui::InvisibleButton("addTicket", { PLUS_R * 2, PLUS_R * 2 });
        bool pHov = ImGui::IsItemHovered();
        dl->AddCircleFilled({ plusX + PLUS_R, iy + CARD_H * 0.5f }, PLUS_R,
            pHov ? MD_TITLE : MD_CARD, 32);
        dl->AddRect({ plusX, iy + (CARD_H - PLUS_R * 2.0f) * 0.5f },
                    { plusX + PLUS_R * 2, iy + (CARD_H + PLUS_R * 2.0f) * 0.5f },
                    MD_BORDER, PLUS_R, 0, 1.0f);
        dl->AddLine({ plusX + PLUS_R, iy + CARD_H * 0.5f - 7.0f },
                    { plusX + PLUS_R, iy + CARD_H * 0.5f + 7.0f }, MD_WHITE, 2.0f);
        dl->AddLine({ plusX + PLUS_R - 7.0f, iy + CARD_H * 0.5f },
                    { plusX + PLUS_R + 7.0f, iy + CARD_H * 0.5f }, MD_WHITE, 2.0f);
    } else {
        // No seats selected — hint text
        dl->AddText({ lx, iy + 14.0f }, MD_MUTED, "Select seats from the map on the right.");
    }
    ImGui::PopFont(); // matches the PushFont(F[1]) before "Select Tickets" label

    // ── RIGHT PANEL — Seat map ────────────────────────────────────────────────
    float rx  = orig.x + splitX + SIDE_PAD;
    float rw  = winW - splitX - SIDE_PAD * 2.0f;
    float ry  = contentTop + 20.0f;

    // "Hall A" title
    ImGui::PushFont(F[5]); // Geologica 58px
    const char* hallLabel = "Hall A";
    ImVec2 hlSz = ImGui::GetFont()->CalcTextSizeA(38.0f, 9999.0f, 0.0f, hallLabel);
    dl->AddText(ImGui::GetFont(), 38.0f,
                { rx + (rw - hlSz.x) * 0.5f, ry }, MD_WHITE, hallLabel);
    ry += hlSz.y + 16.0f;
    ImGui::PopFont();

    // Screen arc
    float arcCX = rx + rw * 0.5f;
    float arcW  = rw * 0.78f;
    float arcH  = 22.0f;
    dl->AddBezierCubic(
        { arcCX - arcW * 0.5f, ry + arcH },
        { arcCX - arcW * 0.3f, ry        },
        { arcCX + arcW * 0.3f, ry        },
        { arcCX + arcW * 0.5f, ry + arcH },
        MD_TITLE, 2.5f, 32);
    ry += arcH + 24.0f;

    // Seat grid
    // Cols 0-2 = left group, 3-10 = center group (8 seats), 11-13 = right group
    constexpr float SW = 26.0f, SH = 26.0f, SG = 4.0f, AISLE = 18.0f;

    float leftGrpW   = 3  * SW + 2  * SG;
    float centerGrpW = 8  * SW + 7  * SG;
    float rightGrpW  = 3  * SW + 2  * SG;
    float totalRowW  = leftGrpW + AISLE + centerGrpW + AISLE + rightGrpW;
    float gridStartX = rx + (rw - totalRowW) * 0.5f;

    constexpr float ROW_H = SH + SG;
    // Upper block: rows 0-5, lower block: rows 6-8 with extra gap
    constexpr float BLOCK_GAP = 18.0f;

    for (int r = 0; r < 9; r++) {
        float extraY = (r >= 6) ? BLOCK_GAP : 0.0f;
        float rowY   = ry + r * ROW_H + extraY;

        for (int c = 0; c < 14; c++) {
            // Map column to pixel x, accounting for aisle gaps
            float colX;
            if      (c < 3)  colX = gridStartX + c * (SW + SG);
            else if (c < 11) colX = gridStartX + leftGrpW + AISLE + (c - 3) * (SW + SG);
            else             colX = gridStartX + leftGrpW + AISLE + centerGrpW + AISLE + (c - 11) * (SW + SG);

            ImVec2 sTL = { colX, rowY }, sBR = { colX + SW, rowY + SH };
            uint8_t state = s_seats[r][c];

            ImU32 fill = (state == ST_BOOKED)   ? SEAT_BOOKED
                       : (state == ST_RESERVED) ? SEAT_RESERVED
                       : (state == ST_SELECTED) ? SEAT_SELECTED
                       :                       SEAT_AVAIL;
            dl->AddRectFilled(sTL, sBR, fill, 4.0f);

            if (state == ST_AVAIL || state == ST_SELECTED) {
                char btnId[16]; snprintf(btnId, sizeof(btnId), "s%d_%d", r, c);
                ImGui::SetCursorScreenPos(sTL);
                ImGui::InvisibleButton(btnId, { SW, SH });
                if (ImGui::IsItemClicked()) {
                    s_seats[r][c] = (state == ST_SELECTED) ? ST_AVAIL : ST_SELECTED;
                }
                if (ImGui::IsItemHovered() && state == ST_AVAIL)
                    dl->AddRectFilled(sTL, sBR, IM_COL32(100,100,115,255), 4.0f);
            }
        }
    }

    // Legend
    float totalSeatRows = 9 * ROW_H + BLOCK_GAP;
    float legendY = ry + totalSeatRows + 16.0f;
    float legX    = rx + (rw - 360.0f) * 0.5f;

    struct LegEntry { ImU32 col; const char* label; };
    LegEntry legs[] = {
        { SEAT_BOOKED,   "BOOKED"   },
        { SEAT_RESERVED, "RESERVED" },
        { SEAT_SELECTED, "SELECTED" },
    };
    ImGui::PushFont(F[1]);
    for (int i = 0; i < 3; i++) {
        float lox = legX + i * 120.0f;
        dl->AddRectFilled({ lox, legendY }, { lox + 18.0f, legendY + 18.0f }, legs[i].col, 3.0f);
        dl->AddText({ lox + 24.0f, legendY + 1.0f }, MD_MUTED, legs[i].label);
    }
    ImGui::PopFont();

    // ── Bottom bar ────────────────────────────────────────────────────────────
    float barY = orig.y + winH - BOTTOM_H;
    dl->AddRectFilled({ orig.x, barY }, { orig.x + winW, orig.y + winH },
                      IM_COL32(24, 24, 28, 255));
    dl->AddLine({ orig.x, barY }, { orig.x + winW, barY }, MD_BORDER, 1.0f);

    // Total price
    ImGui::PushFont(F[3]); // Geologica 24px
    char priceStr[16];
    snprintf(priceStr, sizeof(priceStr), "$%d", selCount * 15);
    dl->AddText({ orig.x + winW * 0.38f, barY + (BOTTOM_H - 24.0f) * 0.5f },
                MD_WHITE, priceStr);
    ImGui::PopFont();

    // Buy button
    constexpr float BUY_W = 160.0f, BUY_H = 44.0f;
    float buyX = orig.x + winW * 0.38f + 80.0f;
    float buyY = barY + (BOTTOM_H - BUY_H) * 0.5f;
    ImGui::SetCursorScreenPos({ buyX, buyY });
    ImGui::InvisibleButton("buyBtn", { BUY_W, BUY_H });
    bool buyHov = ImGui::IsItemHovered();
    bool buyOk  = selCount > 0;
    dl->AddRectFilled({ buyX, buyY }, { buyX + BUY_W, buyY + BUY_H },
        buyOk ? (buyHov ? IM_COL32(245,70,120,255) : MD_TITLE)
              : IM_COL32(80,40,55,255), BUY_H * 0.5f);
    ImGui::PushFont(F[0]); // Inter 18px
    ImVec2 buySz = ImGui::CalcTextSize("Buy");
    dl->AddText({ buyX + (BUY_W - buySz.x) * 0.5f, buyY + (BUY_H - buySz.y) * 0.5f },
                buyOk ? MD_WHITE : IM_COL32(144,90,110,255), "Buy");
    ImGui::PopFont();

    ImGui::SetCursorScreenPos({ 0.0f, orig.y + winH - 1.0f });
    ImGui::Dummy({ 1.0f, 1.0f });

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}
