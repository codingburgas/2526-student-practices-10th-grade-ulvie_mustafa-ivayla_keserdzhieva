#include "imgui.h"
#include <d3d11.h>
#include <string>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <cstring>

// ── Palette (mirrors movieDetail.cpp) ────────────────────────────────────────
static constexpr ImU32 SP_BG     = IM_COL32( 21,  21,  21, 255);
static constexpr ImU32 SP_TITLE  = IM_COL32(236,  64, 113, 255);
static constexpr ImU32 SP_ACCENT = IM_COL32(233, 131, 160, 255);
static constexpr ImU32 SP_MUTED  = IM_COL32(144, 136, 144, 255);
static constexpr ImU32 SP_WHITE  = IM_COL32(255, 255, 255, 255);
static constexpr ImU32 SP_LOGO   = IM_COL32(163,  49,  82, 255);
static constexpr ImU32 SP_BORDER = IM_COL32( 52,  52,  58, 255);
static constexpr ImU32 SP_CARD   = IM_COL32( 32,  32,  38, 255);

// ── Icon helpers ──────────────────────────────────────────────────────────────
static void SPChevL(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    float s = r * 0.45f;
    dl->AddLine({ c.x + s, c.y - s }, { c.x - s, c.y }, col, 2.0f);
    dl->AddLine({ c.x - s, c.y     }, { c.x + s, c.y + s }, col, 2.0f);
}
static void SPChevR(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    float s = r * 0.45f;
    dl->AddLine({ c.x - s, c.y - s }, { c.x + s, c.y }, col, 2.0f);
    dl->AddLine({ c.x + s, c.y     }, { c.x - s, c.y + s }, col, 2.0f);
}
static void SPProfile(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    dl->AddCircleFilled(c, r, IM_COL32(48, 48, 56, 255), 32);
    dl->AddCircle(c, r, IM_COL32(72, 72, 82, 255), 32, 1.0f);
    dl->AddCircleFilled({ c.x, c.y - r * 0.24f }, r * 0.34f, col, 20);
    dl->PushClipRect({ c.x - r, c.y - r }, { c.x + r, c.y + r }, true);
    dl->AddCircleFilled({ c.x, c.y + r * 0.62f }, r * 0.52f, col, 24);
    dl->PopClipRect();
}
static void SPSearch(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    ImVec2 oc = { c.x - r * 0.08f, c.y - r * 0.08f };
    dl->AddCircle(oc, r * 0.58f, col, 24, 1.8f);
    float a = 0.785f;
    ImVec2 ls = { oc.x + r * 0.58f * cosf(a), oc.y + r * 0.58f * sinf(a) };
    dl->AddLine(ls, { ls.x + r * 0.42f * cosf(a), ls.y + r * 0.42f * sinf(a) }, col, 1.8f);
}
static void SPPin(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    dl->AddCircleFilled({ c.x, c.y - r * 0.18f }, r * 0.42f, col, 20);
    dl->AddTriangleFilled(
        { c.x - r * 0.35f, c.y - r * 0.05f },
        { c.x + r * 0.35f, c.y - r * 0.05f },
        { c.x, c.y + r * 0.92f }, col);
    dl->AddCircleFilled({ c.x, c.y - r * 0.18f }, r * 0.18f, SP_BG, 16);
}

// ── Schedule data ─────────────────────────────────────────────────────────────
struct SPHall {
    const char* hall;
    const char* fmt;
    const char* times[4];
};

struct SPEntry {
    int         posterIdx;
    const char* title;
    SPHall      halls[2];
};

static const SPEntry g_schedule[] = {
    { 5, "The Drama",
      { { "B", "3D", { "11:45 AM", "2:15 PM",  "3:45 PM", "9:00 PM"  } },
        { "C", "2D", { "9:25 AM",  "11:30 AM",  "5:15 PM", "8:20 PM"  } } } },
    { 2, "La La Land",
      { { "A", "3D", { "11:45 AM", "1:20 PM",  "4:45 PM", "10:20 PM" } },
        { "C", "2D", { "10:45 AM", "1:45 PM",  "6:30 PM", "11:00 PM" } } } },
    { 3, "Little Women",
      { { "B", "2D", { "10:00 AM", "1:00 PM",  "4:00 PM", "7:30 PM"  } },
        { "A", "3D", { "12:15 PM", "3:00 PM",  "6:15 PM", "9:30 PM"  } } } },
    { 1, "Dead Poets Society",
      { { "A", "2D", { "11:00 AM", "2:00 PM",  "5:00 PM", "8:00 PM"  } },
        { "C", "3D", { "10:30 AM", "1:30 PM",  "4:30 PM", "9:15 PM"  } } } },
    { 6, "The Little Prince",
      { { "B", "2D", { "10:15 AM", "1:00 PM",  "4:15 PM", "7:30 PM"  } },
        { "A", "3D", { "12:00 PM", "2:45 PM",  "5:30 PM", "8:45 PM"  } } } },
};
static constexpr int SP_ENTRY_COUNT = 5;

// ── Day-strip state (separate from movieDetail's) ─────────────────────────────
static int s_spDayOffset = 0;
static int s_spWeekStart = 0;
// Selected time slot per [entry][hall], -1 = none
static int s_selTime[SP_ENTRY_COUNT][2] = {
    {-1,-1}, {-1,-1}, {-1,-1}, {-1,-1}, {-1,-1}
};

static const char* sp_dayNames[]   = { "Sun","Mon","Tue","Wed","Thu","Fri","Sat" };

void RenderSchedule(
    bool& goHome, int& selectedMovieIndex,
    bool loggedIn,
    ID3D11ShaderResourceView** posters,
    int* /*posterWidths*/, int* /*posterHeights*/)
{
    ImGuiIO&    io   = ImGui::GetIO();
    ImFont**    F    = io.Fonts->Fonts.Data;
    float       winW = io.DisplaySize.x;
    float       winH = io.DisplaySize.y;

    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::PushStyleColor(ImGuiCol_WindowBg,
        ImVec4(21/255.f, 21/255.f, 21/255.f, 1.f));
    ImGui::Begin("SchedulePage", nullptr,
        ImGuiWindowFlags_NoTitleBar   | ImGuiWindowFlags_NoResize  |
        ImGuiWindowFlags_NoCollapse   | ImGuiWindowFlags_NoMove    |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar  | ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2      orig = ImGui::GetWindowPos();
    ImDrawList* dl   = ImGui::GetWindowDrawList();

    // ── Navbar ────────────────────────────────────────────────────────────────
    constexpr float NAV_H = 64.0f;
    dl->AddRectFilled(orig, { orig.x + winW, orig.y + NAV_H }, SP_BG);
    dl->AddLine({ orig.x, orig.y + NAV_H },
                { orig.x + winW, orig.y + NAV_H }, SP_BORDER, 1.0f);

    // Logo
    ImGui::PushFont(F[3]);
    float logoY = orig.y + (NAV_H - 24.0f) * 0.5f;
    dl->AddText({ orig.x + 36.0f, logoY }, SP_LOGO, "CINEMA");
    float logoW = ImGui::CalcTextSize("CINEMA").x;
    ImGui::PopFont();

    // Nav items (Schedule is active/underlined)
    ImGui::PushFont(F[0]);
    const char* navItems[]   = { "Home", "Your Tickets", "Schedule" };
    bool        navActive[]  = { false,  false,          true       };
    float nx = orig.x + 36.0f + logoW + 32.0f;
    for (int i = 0; i < 3; i++) {
        ImVec2 tsz = ImGui::CalcTextSize(navItems[i]);
        float  ty  = orig.y + (NAV_H - tsz.y) * 0.5f;
        ImU32  tc  = navActive[i] ? SP_WHITE : SP_MUTED;
        dl->AddText({ nx, ty }, tc, navItems[i]);
        if (navActive[i])
            dl->AddLine({ nx, ty + tsz.y + 3.0f },
                        { nx + tsz.x, ty + tsz.y + 3.0f }, SP_TITLE, 2.0f);

        ImGui::SetCursorScreenPos({ nx - 4.0f, orig.y + 8.0f });
        ImGui::InvisibleButton(("sp_nav_" + std::string(navItems[i])).c_str(),
                               { tsz.x + 8.0f, NAV_H - 16.0f });
        if (ImGui::IsItemClicked() && i == 0) goHome = true;
        nx += tsz.x + 42.0f;
    }
    ImGui::PopFont();

    // Profile icon (logged-in users can access schedule)
    constexpr float PROF_R = 18.0f;
    ImVec2 profC = { orig.x + winW - 28.0f - PROF_R, orig.y + NAV_H * 0.5f };
    ImGui::SetCursorScreenPos({ profC.x - PROF_R, profC.y - PROF_R });
    ImGui::InvisibleButton("sp_profile", { PROF_R * 2, PROF_R * 2 });
    SPProfile(dl, profC, PROF_R, IM_COL32(200, 200, 210, 255));

    // City selector
    constexpr float CITY_W = 90.0f, CITY_H = 36.0f;
    float cityRight = profC.x - PROF_R - 16.0f;
    float cityLeft  = cityRight - CITY_W;
    float cityTop   = orig.y + (NAV_H - CITY_H) * 0.5f;
    ImGui::SetCursorScreenPos({ cityLeft, cityTop });
    ImGui::InvisibleButton("sp_city", { CITY_W, CITY_H });
    ImGui::PushFont(F[1]);
    dl->AddText({ cityLeft + 20.0f, cityTop + (CITY_H - 15.0f) * 0.5f },
                ImGui::IsItemHovered() ? SP_WHITE : SP_MUTED, "City");
    ImGui::PopFont();
    SPPin(dl, { cityLeft + 11.0f, cityTop + CITY_H * 0.5f }, 7.0f, SP_ACCENT);

    // Search bar
    constexpr float SB_W = 280.0f, SB_H = 38.0f;
    float sbX = cityLeft - 16.0f - SB_W;
    float sbY = orig.y + (NAV_H - SB_H) * 0.5f;
    dl->AddRectFilled({ sbX, sbY }, { sbX + SB_W, sbY + SB_H },
                      IM_COL32(28,28,28,255), 19.0f);
    dl->AddRect(      { sbX, sbY }, { sbX + SB_W, sbY + SB_H },
                      IM_COL32(55,55,62,255), 19.0f, 0, 1.0f);
    ImGui::PushFont(F[1]);
    dl->AddText({ sbX + 16.0f, sbY + (SB_H - 15.0f) * 0.5f },
                IM_COL32(144,136,144,115), "Search a movie");
    ImGui::PopFont();
    SPSearch(dl, { sbX + SB_W - 14.0f, sbY + SB_H * 0.5f },
             8.0f, IM_COL32(144,136,144,145));

    // ── Date strip ────────────────────────────────────────────────────────────
    constexpr float DS_H = 78.0f;
    float dsY = orig.y + NAV_H;
    dl->AddRectFilled({ orig.x, dsY }, { orig.x + winW, dsY + DS_H }, SP_BG);
    dl->AddLine({ orig.x, dsY + DS_H },
                { orig.x + winW, dsY + DS_H }, SP_BORDER, 1.0f);

    time_t now = time(nullptr); struct tm today; localtime_s(&today, &now);

    // Calendar icon
    float calX = orig.x + 36.0f, calCY = dsY + DS_H * 0.5f;
    dl->AddRectFilled({ calX, calCY - 12.0f },
                      { calX + 28.0f, calCY + 12.0f }, SP_CARD, 5.0f);
    dl->AddRect({ calX, calCY - 12.0f },
                { calX + 28.0f, calCY + 12.0f }, SP_BORDER, 5.0f, 0, 1.0f);
    dl->AddLine({ calX, calCY - 4.0f },
                { calX + 28.0f, calCY - 4.0f }, SP_BORDER, 1.0f);
    dl->AddLine({ calX + 8.0f,  calCY - 16.0f },
                { calX + 8.0f,  calCY - 8.0f  }, SP_ACCENT, 1.5f);
    dl->AddLine({ calX + 20.0f, calCY - 16.0f },
                { calX + 20.0f, calCY - 8.0f  }, SP_ACCENT, 1.5f);

    // Day pills
    constexpr float DAY_W = 104.0f, DAY_H = 54.0f, DAY_GAP = 8.0f;
    float dayStartX = orig.x + 80.0f;
    float dayPillY  = dsY + (DS_H - DAY_H) * 0.5f;

    for (int i = 0; i < 7; i++) {
        time_t t = now + (time_t)(s_spWeekStart + i) * 86400;
        struct tm d; localtime_s(&d, &t);
        float px = dayStartX + i * (DAY_W + DAY_GAP);
        bool  sel = (s_spDayOffset == i);

        ImVec2 ptl = { px, dayPillY }, pbr = { px + DAY_W, dayPillY + DAY_H };
        ImGui::SetCursorScreenPos(ptl);
        char bid[16]; snprintf(bid, sizeof(bid), "sp_day_%d", i);
        ImGui::InvisibleButton(bid, { DAY_W, DAY_H });
        if (ImGui::IsItemClicked()) s_spDayOffset = i;
        bool hov = ImGui::IsItemHovered();

        dl->AddRectFilled(ptl, pbr,
            sel ? SP_CARD : (hov ? IM_COL32(32,32,38,255) : IM_COL32(0,0,0,0)), 10.0f);
        dl->AddRect(ptl, pbr,
            sel ? SP_TITLE : SP_BORDER, 10.0f, 0, sel ? 1.5f : 1.0f);

        ImGui::PushFont(F[1]);
        const char* dn = sp_dayNames[d.tm_wday];
        ImVec2 dnSz = ImGui::CalcTextSize(dn);
        dl->AddText({ px + (DAY_W - dnSz.x) * 0.5f, dayPillY + 8.0f }, SP_MUTED, dn);
        ImGui::PopFont();

        ImGui::PushFont(F[3]);
        char dnum[4]; snprintf(dnum, sizeof(dnum), "%d", d.tm_mday);
        ImVec2 dnumSz = ImGui::CalcTextSize(dnum);
        dl->AddText({ px + (DAY_W - dnumSz.x) * 0.5f, dayPillY + 26.0f },
                    sel ? SP_WHITE : SP_MUTED, dnum);
        ImGui::PopFont();
    }

    // Week nav arrows
    float arrX  = dayStartX + 7 * (DAY_W + DAY_GAP) + 8.0f;
    constexpr float ARR_S = 22.0f;
    float arrCY1 = dsY + DS_H * 0.5f - 14.0f;
    float arrCY2 = dsY + DS_H * 0.5f + 14.0f;

    ImGui::SetCursorScreenPos({ arrX, arrCY1 - ARR_S * 0.5f });
    ImGui::InvisibleButton("sp_wkR", { ARR_S, ARR_S });
    if (ImGui::IsItemClicked()) { s_spWeekStart++; s_spDayOffset = 0; }
    dl->AddRectFilled({ arrX, arrCY1 - ARR_S * 0.5f },
                      { arrX + ARR_S, arrCY1 + ARR_S * 0.5f },
                      ImGui::IsItemHovered() ? SP_TITLE : SP_CARD, 5.0f);
    SPChevR(dl, { arrX + ARR_S * 0.5f, arrCY1 }, 6.0f, SP_WHITE);

    ImGui::SetCursorScreenPos({ arrX, arrCY2 - ARR_S * 0.5f });
    ImGui::InvisibleButton("sp_wkL", { ARR_S, ARR_S });
    if (ImGui::IsItemClicked() && s_spWeekStart > 0) { s_spWeekStart--; s_spDayOffset = 0; }
    dl->AddRectFilled({ arrX, arrCY2 - ARR_S * 0.5f },
                      { arrX + ARR_S, arrCY2 + ARR_S * 0.5f },
                      ImGui::IsItemHovered() ? SP_TITLE : SP_CARD, 5.0f);
    SPChevL(dl, { arrX + ARR_S * 0.5f, arrCY2 }, 6.0f, SP_WHITE);

    // ── Schedule list (scrollable child) ──────────────────────────────────────
    constexpr float SIDE_PAD     = 60.0f;
    constexpr float POST_W       = 155.0f;
    constexpr float POST_H       = 230.0f;
    constexpr float ENTRY_PAD    = 28.0f;
    constexpr float BADGE_H      = 44.0f;
    constexpr float TIME_H_      = 44.0f;
    constexpr float HALL_GAP     = 22.0f;
    constexpr float HALL_SEC_H   = BADGE_H + 12.0f + TIME_H_;
    constexpr float RIGHT_H      = HALL_SEC_H * 2.0f + HALL_GAP;
    constexpr float ENTRY_H      = (POST_H > RIGHT_H ? POST_H : RIGHT_H) + ENTRY_PAD * 2.0f;
    constexpr float TIME_BTN_W   = 128.0f;
    constexpr float TIME_BTN_GAP = 10.0f;
    constexpr float BADGE_W_MAX  = 360.0f;
    constexpr float LIST_RX_OFF  = SIDE_PAD + POST_W + 36.0f;

    float listTop = orig.y + NAV_H + DS_H;
    float listH   = winH - NAV_H - DS_H;

    // Font sizes (LegacySize = size passed to AddFontXXX, renamed in ImGui 1.92)
    float fSz1 = F[1]->LegacySize;
    float fSz3 = F[3]->LegacySize;

    ImGui::SetCursorScreenPos({ orig.x, listTop });
    ImGui::BeginChild("sp_scroll", { winW, listH }, false,
        ImGuiWindowFlags_NoScrollbar);

    ImVec2      cOrig        = ImGui::GetWindowPos();
    ImDrawList* cdl          = ImGui::GetWindowDrawList();
    float       scrollY      = ImGui::GetScrollY();
    ImVec2      mouse        = ImGui::GetIO().MousePos;
    bool        mouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

    // Single Dummy call sets full content height for scroll — no PushFont/InvisibleButton needed
    ImGui::Dummy({ winW, (float)SP_ENTRY_COUNT * ENTRY_H + 24.0f });

    for (int ei = 0; ei < SP_ENTRY_COUNT; ei++) {
        const SPEntry& e = g_schedule[ei];
        float screenEY = cOrig.y + (float)ei * ENTRY_H - scrollY;

        cdl->AddLine({ cOrig.x + SIDE_PAD, screenEY },
                     { cOrig.x + winW - SIDE_PAD, screenEY }, SP_BORDER, 1.0f);

        float screenCY = screenEY + ENTRY_PAD;
        ImVec2 pTL = { cOrig.x + SIDE_PAD, screenCY };
        ImVec2 pBR = { pTL.x + POST_W, screenCY + POST_H };
        if (posters && posters[e.posterIdx])
            cdl->AddImage((ImTextureID)posters[e.posterIdx], pTL, pBR,
                          { 0,0 }, { 1,1 }, SP_WHITE);
        else
            cdl->AddRectFilled(pTL, pBR, IM_COL32(34,34,44,255), 8.0f);
        cdl->AddRect(pTL, pBR, IM_COL32(233,131,160,45), 8.0f, 0, 1.0f);

        float screenRX = cOrig.x + LIST_RX_OFF;
        float screenRY = screenCY + (POST_H - RIGHT_H) * 0.5f;

        for (int hi = 0; hi < 2; hi++) {
            const SPHall& h = e.halls[hi];
            float screenHSecY = screenRY + (float)hi * (HALL_SEC_H + HALL_GAP);

            // Hall badge
            ImVec2 bTL = { screenRX, screenHSecY };
            ImVec2 bBR = { screenRX + BADGE_W_MAX, screenHSecY + BADGE_H };
            cdl->AddRectFilled(bTL, bBR, SP_CARD, 10.0f);
            cdl->AddRect(bTL, bBR, SP_BORDER, 10.0f, 0, 1.0f);

            float tx    = screenRX + 16.0f;
            float midBY = screenHSecY + BADGE_H * 0.5f;

            // "BURGAS city"
            ImVec2 sz = F[3]->CalcTextSizeA(fSz3, FLT_MAX, 0.0f, "BURGAS");
            cdl->AddText(F[3], fSz3, { tx, midBY - fSz3 * 0.54f }, SP_WHITE, "BURGAS");
            tx += sz.x;
            ImVec2 sz2 = F[1]->CalcTextSizeA(fSz1, FLT_MAX, 0.0f, " city");
            cdl->AddText(F[1], fSz1, { tx + 1.0f, midBY - fSz1 * 0.5f + 1.0f }, SP_MUTED, " city");
            tx += sz2.x + 14.0f;

            // "| X hall"
            cdl->AddLine({ tx, screenHSecY + 10.0f }, { tx, screenHSecY + BADGE_H - 10.0f }, SP_BORDER, 1.0f);
            tx += 14.0f;
            ImVec2 sz3 = F[3]->CalcTextSizeA(fSz3, FLT_MAX, 0.0f, h.hall);
            cdl->AddText(F[3], fSz3, { tx, midBY - fSz3 * 0.54f }, SP_WHITE, h.hall);
            tx += sz3.x;
            ImVec2 sz4 = F[1]->CalcTextSizeA(fSz1, FLT_MAX, 0.0f, " hall");
            cdl->AddText(F[1], fSz1, { tx + 1.0f, midBY - fSz1 * 0.5f + 1.0f }, SP_MUTED, " hall");
            tx += sz4.x + 14.0f;

            // "| 3D/2D"
            cdl->AddLine({ tx, screenHSecY + 10.0f }, { tx, screenHSecY + BADGE_H - 10.0f }, SP_BORDER, 1.0f);
            tx += 14.0f;
            cdl->AddText(F[3], fSz3, { tx, midBY - fSz3 * 0.54f }, SP_ACCENT, h.fmt);

            // Time slot buttons + BOOK button — manual hit testing
            float screenTRowY = screenHSecY + BADGE_H + 12.0f;
            bool  rowVis = screenTRowY < listTop + listH && screenTRowY + TIME_H_ > listTop;

            for (int ti = 0; ti < 4; ti++) {
                float  screenTBX = screenRX + (float)ti * (TIME_BTN_W + TIME_BTN_GAP);
                ImVec2 tTL = { screenTBX, screenTRowY };
                ImVec2 tBR = { screenTBX + TIME_BTN_W, screenTRowY + TIME_H_ };

                bool hov = rowVis &&
                    mouse.x >= tTL.x && mouse.x <= tBR.x &&
                    mouse.y >= tTL.y && mouse.y <= tBR.y;
                bool clk = hov && mouseClicked;
                bool sel = (s_selTime[ei][hi] == ti);

                if (clk) s_selTime[ei][hi] = ti;

                // Selected: filled pink; hovered: lighter card; normal: dark card
                ImU32 bgCol = sel  ? IM_COL32(178, 35, 75, 255)  :
                              hov  ? IM_COL32(52,  52, 60, 255)   :
                                     IM_COL32(32,  32, 38, 255);
                ImU32 bdCol = sel  ? SP_TITLE : (hov ? SP_ACCENT : SP_BORDER);
                ImU32 txCol = (sel || hov) ? SP_WHITE : SP_MUTED;

                cdl->AddRectFilled(tTL, tBR, bgCol, 10.0f);
                cdl->AddRect(tTL, tBR, bdCol, 10.0f, 0, sel ? 1.5f : 1.0f);

                ImVec2 tsz = F[1]->CalcTextSizeA(fSz1, FLT_MAX, 0.0f, h.times[ti]);
                cdl->AddText(F[1], fSz1,
                    { screenTBX + (TIME_BTN_W - tsz.x) * 0.5f,
                      screenTRowY + (TIME_H_ - tsz.y) * 0.5f },
                    txCol, h.times[ti]);
            }

            // BOOK button — always visible, navigates to seat-selection detail page
            constexpr float BOOK_W   = 88.0f;
            constexpr float BOOK_GAP = 14.0f;
            float   bookX   = screenRX + 4.0f * (TIME_BTN_W + TIME_BTN_GAP) + BOOK_GAP;
            ImVec2  bookTL  = { bookX, screenTRowY };
            ImVec2  bookBR  = { bookX + BOOK_W, screenTRowY + TIME_H_ };
            bool    bookHov = rowVis &&
                mouse.x >= bookTL.x && mouse.x <= bookBR.x &&
                mouse.y >= bookTL.y && mouse.y <= bookBR.y;
            bool    bookClk = bookHov && mouseClicked;

            cdl->AddRectFilled(bookTL, bookBR,
                bookHov ? IM_COL32(245, 70, 115, 255) : SP_TITLE, 10.0f);
            ImVec2 bkSz = F[1]->CalcTextSizeA(fSz1, FLT_MAX, 0.0f, "BOOK");
            cdl->AddText(F[1], fSz1,
                { bookX + (BOOK_W - bkSz.x) * 0.5f,
                  screenTRowY + (TIME_H_ - bkSz.y) * 0.5f },
                SP_WHITE, "BOOK");

            if (bookClk) selectedMovieIndex = e.posterIdx;
        }
    }

    ImGui::EndChild();

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}
