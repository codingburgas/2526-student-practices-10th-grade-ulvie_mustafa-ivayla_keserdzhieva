#include "imgui.h"
#include "ticketStore.h"
#include <d3d11.h>
#include <string>
#include <cstdio>
#include <cmath>

// ── Palette ───────────────────────────────────────────────────────────────────
static constexpr ImU32 TK_BG     = IM_COL32( 21,  21,  21, 255);
static constexpr ImU32 TK_CARD   = IM_COL32( 40,  18,  28, 255);
static constexpr ImU32 TK_CBDR   = IM_COL32(200,  65,  95, 170);
static constexpr ImU32 TK_TITLE  = IM_COL32(236,  64, 113, 255);
static constexpr ImU32 TK_ACCENT = IM_COL32(233, 131, 160, 255);
static constexpr ImU32 TK_MUTED  = IM_COL32(144, 136, 144, 255);
static constexpr ImU32 TK_WHITE  = IM_COL32(255, 255, 255, 255);
static constexpr ImU32 TK_LOGO   = IM_COL32(163,  49,  82, 255);
static constexpr ImU32 TK_NBDR   = IM_COL32( 52,  52,  58, 255);
static constexpr ImU32 TK_DARK   = IM_COL32( 32,  32,  38, 255);

// ── Icon helpers ──────────────────────────────────────────────────────────────
static void TKProfile(ImDrawList* dl, ImVec2 c, float r) {
    ImU32 col = IM_COL32(200, 200, 210, 255);
    dl->AddCircleFilled(c, r, IM_COL32(48,48,56,255), 32);
    dl->AddCircle(c, r, IM_COL32(72,72,82,255), 32, 1.0f);
    dl->AddCircleFilled({ c.x, c.y - r * 0.24f }, r * 0.34f, col, 20);
    dl->PushClipRect({ c.x - r, c.y - r }, { c.x + r, c.y + r }, true);
    dl->AddCircleFilled({ c.x, c.y + r * 0.62f }, r * 0.52f, col, 24);
    dl->PopClipRect();
}
static void TKSearch(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    ImVec2 oc = { c.x - r * 0.08f, c.y - r * 0.08f };
    dl->AddCircle(oc, r * 0.58f, col, 24, 1.8f);
    float a = 0.785f;
    ImVec2 ls = { oc.x + r * 0.58f * cosf(a), oc.y + r * 0.58f * sinf(a) };
    dl->AddLine(ls, { ls.x + r * 0.42f * cosf(a), ls.y + r * 0.42f * sinf(a) }, col, 1.8f);
}
static void TKPin(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    dl->AddCircleFilled({ c.x, c.y - r * 0.18f }, r * 0.42f, col, 20);
    dl->AddTriangleFilled(
        { c.x - r * 0.35f, c.y - r * 0.05f },
        { c.x + r * 0.35f, c.y - r * 0.05f },
        { c.x, c.y + r * 0.92f }, col);
    dl->AddCircleFilled({ c.x, c.y - r * 0.18f }, r * 0.18f, TK_BG, 16);
}


void RenderMyTickets(
    bool& goHome, bool& goSchedule,
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
    ImGui::Begin("MyTicketsPage", nullptr,
        ImGuiWindowFlags_NoTitleBar   | ImGuiWindowFlags_NoResize   |
        ImGuiWindowFlags_NoCollapse   | ImGuiWindowFlags_NoMove     |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar  | ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2      orig = ImGui::GetWindowPos();
    ImDrawList* dl   = ImGui::GetWindowDrawList();

    // ── Navbar ────────────────────────────────────────────────────────────────
    constexpr float NAV_H = 64.0f;
    dl->AddRectFilled(orig, { orig.x + winW, orig.y + NAV_H }, TK_BG);
    dl->AddLine({ orig.x, orig.y + NAV_H },
                { orig.x + winW, orig.y + NAV_H }, TK_NBDR, 1.0f);

    // Logo
    ImGui::PushFont(F[3]);
    float logoY = orig.y + (NAV_H - F[3]->LegacySize) * 0.5f;
    dl->AddText({ orig.x + 36.0f, logoY }, TK_LOGO, "CINEMA");
    float logoW = ImGui::CalcTextSize("CINEMA").x;
    ImGui::PopFont();

    // Nav items — "Your Tickets" is active
    ImGui::PushFont(F[0]);
    const char* navItems[]  = { "Home", "Your Tickets", "Schedule" };
    bool        navActive[] = { false,  true,           false      };
    float nx = orig.x + 36.0f + logoW + 32.0f;
    for (int i = 0; i < 3; i++) {
        ImVec2 tsz = ImGui::CalcTextSize(navItems[i]);
        float  ty  = orig.y + (NAV_H - tsz.y) * 0.5f;
        dl->AddText({ nx, ty }, navActive[i] ? TK_WHITE : TK_MUTED, navItems[i]);
        if (navActive[i])
            dl->AddLine({ nx, ty + tsz.y + 3.0f },
                        { nx + tsz.x, ty + tsz.y + 3.0f }, TK_TITLE, 2.0f);

        ImGui::SetCursorScreenPos({ nx - 4.0f, orig.y + 8.0f });
        ImGui::InvisibleButton(("tk_nav_" + std::string(navItems[i])).c_str(),
                               { tsz.x + 8.0f, NAV_H - 16.0f });
        if (ImGui::IsItemClicked()) {
            if (i == 0) goHome     = true;
            if (i == 2) goSchedule = true;
        }
        nx += tsz.x + 42.0f;
    }
    ImGui::PopFont();

    // Profile icon
    constexpr float PROF_R = 18.0f;
    ImVec2 profC = { orig.x + winW - 28.0f - PROF_R, orig.y + NAV_H * 0.5f };
    ImGui::SetCursorScreenPos({ profC.x - PROF_R, profC.y - PROF_R });
    ImGui::InvisibleButton("tk_profile", { PROF_R * 2, PROF_R * 2 });
    TKProfile(dl, profC, PROF_R);

    // City
    constexpr float CITY_W = 90.0f, CITY_H = 36.0f;
    float cityLeft = profC.x - PROF_R - 16.0f - CITY_W;
    float cityTop  = orig.y + (NAV_H - CITY_H) * 0.5f;
    ImGui::SetCursorScreenPos({ cityLeft, cityTop });
    ImGui::InvisibleButton("tk_city", { CITY_W, CITY_H });
    ImGui::PushFont(F[1]);
    dl->AddText({ cityLeft + 20.0f, cityTop + (CITY_H - F[1]->LegacySize) * 0.5f },
                ImGui::IsItemHovered() ? TK_WHITE : TK_MUTED, "City");
    ImGui::PopFont();
    TKPin(dl, { cityLeft + 11.0f, cityTop + CITY_H * 0.5f }, 7.0f, TK_ACCENT);

    // Search bar
    constexpr float SB_W = 280.0f, SB_H = 38.0f;
    float sbX = cityLeft - 16.0f - SB_W;
    float sbY = orig.y + (NAV_H - SB_H) * 0.5f;
    dl->AddRectFilled({ sbX, sbY }, { sbX + SB_W, sbY + SB_H },
                      IM_COL32(28,28,28,255), 19.0f);
    dl->AddRect(      { sbX, sbY }, { sbX + SB_W, sbY + SB_H },
                      IM_COL32(55,55,62,255), 19.0f, 0, 1.0f);
    ImGui::PushFont(F[1]);
    dl->AddText({ sbX + 16.0f, sbY + (SB_H - F[1]->LegacySize) * 0.5f },
                IM_COL32(144,136,144,115), "Search a movie");
    ImGui::PopFont();
    TKSearch(dl, { sbX + SB_W - 14.0f, sbY + SB_H * 0.5f },
             8.0f, IM_COL32(144,136,144,145));

    // ── Ticket cards ──────────────────────────────────────────────────────────
    constexpr float SIDE_PAD    = 36.0f;
    constexpr float CARD_H      = 172.0f;
    constexpr float CARD_GAP    = 28.0f;
    constexpr float TOP_MARGIN  = 72.0f;
    constexpr float POST_W      = 118.0f;
    constexpr float POST_H      = 152.0f;
    constexpr float CARD_PAD    = 18.0f;
    constexpr float BADGE_H     = 44.0f;
    constexpr float PILL_H      = 52.0f;
    constexpr float TIME_PILL_W = 130.0f;
    constexpr float DATE_PILL_W = 100.0f;

    float cardW = winW - 2.0f * SIDE_PAD;
    float cardY = orig.y + NAV_H + TOP_MARGIN;

    if (g_purchasedTickets.empty()) {
        ImGui::PushFont(F[4]);
        const char* emptyMsg = "No tickets yet — buy one from the schedule!";
        ImVec2 emSz = ImGui::CalcTextSize(emptyMsg);
        dl->AddText({ orig.x + (winW - emSz.x) * 0.5f, cardY + 40.0f }, TK_MUTED, emptyMsg);
        ImGui::PopFont();
    }

    for (int ti = 0; ti < (int)g_purchasedTickets.size(); ti++) {
        const PurchasedTicket& t = g_purchasedTickets[ti];
        ImVec2 cTL = { orig.x + SIDE_PAD, cardY };
        ImVec2 cBR = { orig.x + SIDE_PAD + cardW, cardY + CARD_H };

        dl->AddRectFilled(cTL, cBR, TK_CARD, 14.0f);
        dl->AddRect(cTL, cBR, TK_CBDR, 14.0f, 0, 1.5f);

        // Poster
        float pOffY = (CARD_H - POST_H) * 0.5f;
        ImVec2 pTL  = { cTL.x + CARD_PAD, cardY + pOffY };
        ImVec2 pBR  = { pTL.x + POST_W, pTL.y + POST_H };
        if (posters && posters[t.posterIdx])
            dl->AddImage((ImTextureID)posters[t.posterIdx], pTL, pBR,
                         { 0,0 }, { 1,1 }, TK_WHITE);
        else
            dl->AddRectFilled(pTL, pBR, IM_COL32(34,34,44,255), 8.0f);
        dl->AddRect(pTL, pBR, IM_COL32(200,80,110,50), 8.0f, 0, 1.0f);

        // Content column
        float cx = pTL.x + POST_W + 22.0f;
        float cy = cardY + CARD_PAD + 2.0f;

        // "Place:" label
        ImGui::PushFont(F[1]);
        dl->AddText({ cx, cy }, TK_MUTED, "Place:");
        cy += F[1]->LegacySize + 6.0f;
        ImGui::PopFont();

        // Place badge: BURGAS city | A hall | B row | 08 seat | 3D
        constexpr float BADGE_W = 440.0f;
        ImVec2 bTL = { cx, cy }, bBR = { cx + BADGE_W, cy + BADGE_H };
        dl->AddRectFilled(bTL, bBR, TK_DARK, 10.0f);
        dl->AddRect(bTL, bBR, TK_NBDR, 10.0f, 0, 1.0f);

        struct BSec { const char* big; const char* sub; ImU32 bigCol; };
        BSec secs[5] = {
            { "BURGAS",          " city",  TK_WHITE  },
            { t.hall.c_str(),    " hall",  TK_WHITE  },
            { t.row.c_str(),     " row",   TK_WHITE  },
            { t.seat.c_str(),    " seat",  TK_WHITE  },
            { t.fmt.c_str(),     nullptr,  TK_ACCENT },
        };
        float bx    = cx + 14.0f;
        float midBY = cy + BADGE_H * 0.5f;
        for (int si = 0; si < 5; si++) {
            if (si > 0) {
                dl->AddLine({ bx, cy + 10.0f }, { bx, cy + BADGE_H - 10.0f }, TK_NBDR, 1.0f);
                bx += 12.0f;
            }
            ImGui::PushFont(F[4]);
            ImVec2 bigSz = ImGui::CalcTextSize(secs[si].big);
            dl->AddText({ bx, midBY - bigSz.y * 0.54f }, secs[si].bigCol, secs[si].big);
            bx += bigSz.x;
            ImGui::PopFont();
            if (secs[si].sub) {
                ImGui::PushFont(F[2]);
                ImVec2 subSz = ImGui::CalcTextSize(secs[si].sub);
                dl->AddText({ bx + 1.0f, midBY - subSz.y * 0.5f + 2.0f },
                            TK_MUTED, secs[si].sub);
                bx += subSz.x + 12.0f;
                ImGui::PopFont();
            } else {
                bx += 12.0f;
            }
        }
        cy += BADGE_H + 12.0f;

        // "Time:" and "Date:" labels
        float timeX = cx;
        float dateX = cx + TIME_PILL_W + 24.0f;
        ImGui::PushFont(F[1]);
        dl->AddText({ timeX, cy }, TK_MUTED, "Time:");
        dl->AddText({ dateX, cy }, TK_MUTED, "Date:");
        cy += F[1]->LegacySize + 6.0f;
        ImGui::PopFont();

        // Time pill
        ImVec2 tpTL = { timeX, cy }, tpBR = { timeX + TIME_PILL_W, cy + PILL_H };
        dl->AddRectFilled(tpTL, tpBR, TK_DARK, 10.0f);
        dl->AddRect(tpTL, tpBR, TK_NBDR, 10.0f, 0, 1.0f);
        ImGui::PushFont(F[1]);
        ImVec2 timeSz = ImGui::CalcTextSize(t.time.c_str());
        dl->AddText({ timeX + (TIME_PILL_W - timeSz.x) * 0.5f,
                      cy    + (PILL_H       - timeSz.y) * 0.5f },
                    TK_WHITE, t.time.c_str());
        ImGui::PopFont();

        // Date pill
        ImVec2 dpTL = { dateX, cy }, dpBR = { dateX + DATE_PILL_W, cy + PILL_H };
        dl->AddRectFilled(dpTL, dpBR, TK_DARK, 10.0f);
        dl->AddRect(dpTL, dpBR, TK_NBDR, 10.0f, 0, 1.0f);
        // day name (small, muted, top-center)
        ImGui::PushFont(F[2]);
        ImVec2 daySz = ImGui::CalcTextSize(t.dayName.c_str());
        dl->AddText({ dateX + (DATE_PILL_W - daySz.x) * 0.5f, cy + 7.0f },
                    TK_MUTED, t.dayName.c_str());
        ImGui::PopFont();
        // day number (large, white, below)
        ImGui::PushFont(F[3]);
        char numBuf[4]; snprintf(numBuf, sizeof(numBuf), "%d", t.dayNum);
        ImVec2 numSz = ImGui::CalcTextSize(numBuf);
        dl->AddText({ dateX + (DATE_PILL_W - numSz.x) * 0.5f,
                      cy    + 7.0f + F[2]->LegacySize + 2.0f },
                    TK_WHITE, numBuf);
        ImGui::PopFont();

        cardY += CARD_H + CARD_GAP;
    }

    (void)winH;
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}
