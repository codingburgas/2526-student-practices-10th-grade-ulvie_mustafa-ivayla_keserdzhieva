#include "imgui.h"
#include "ticketStore.h"
#include "BookingService.h"
#include "citySelector.h"
#include <d3d11.h>
#include <string>
#include <cstdio>
#include <cmath>
#include <ctime>

// ── Palette (matches app) ─────────────────────────────────────────────────────
static constexpr ImU32 PU_BG     = IM_COL32( 21,  21,  21, 255);
static constexpr ImU32 PU_CARD   = IM_COL32( 32,  32,  38, 255);
static constexpr ImU32 PU_CARD2  = IM_COL32( 28,  28,  34, 255);
static constexpr ImU32 PU_TITLE  = IM_COL32(236,  64, 113, 255);
static constexpr ImU32 PU_ACCENT = IM_COL32(233, 131, 160, 255);
static constexpr ImU32 PU_MUTED  = IM_COL32(144, 136, 144, 255);
static constexpr ImU32 PU_WHITE  = IM_COL32(255, 255, 255, 255);
static constexpr ImU32 PU_LOGO   = IM_COL32(163,  49,  82, 255);
static constexpr ImU32 PU_BORDER = IM_COL32( 52,  52,  58, 255);
static constexpr ImU32 PU_GREEN  = IM_COL32( 72, 199, 142, 255);

// ── Movie data (mirror of movieDetail) ───────────────────────────────────────
struct PurchaseMovie {
    const char* title;
    const char* times[4];
};
static const PurchaseMovie g_pmov[9] = {
    { "10 Things I Hate About You", { "10:00 AM","1:15 PM","4:30 PM","8:00 PM"  } },
    { "Dead Poets Society",         { "11:00 AM","2:00 PM","5:15 PM","9:00 PM"  } },
    { "La La Land",                 { "10:30 AM","1:45 PM","5:00 PM","8:30 PM"  } },
    { "Little Women",               { "11:15 AM","2:30 PM","5:45 PM","9:15 PM"  } },
    { "Matilda",                    { "10:00 AM","12:30 PM","3:00 PM","6:00 PM" } },
    { "The Drama",                  { "11:45 AM","1:20 PM","4:45 PM","10:20 PM" } },
    { "The Little Prince",          { "10:15 AM","1:00 PM","4:15 PM","7:30 PM"  } },
    { "The Princess Diaries",       { "10:30 AM","1:30 PM","4:00 PM","7:00 PM"  } },
    { "The Terminal",               { "11:00 AM","2:15 PM","5:30 PM","9:00 PM"  } },
};
static const char* s_dayNames3[] = { "Sun","Mon","Tue","Wed","Thu","Fri","Sat" };
static const char* s_monNames[]  = { "Jan","Feb","Mar","Apr","May","Jun",
                                     "Jul","Aug","Sep","Oct","Nov","Dec" };

// ── Per-session state ─────────────────────────────────────────────────────────
static int  s_payMethod  = 0;       // 0=card  1=gpay  2=apay  3=cash
static char s_cardNum[20]  = "";
static char s_cardName[64] = "";
static char s_cardExp[6]   = "";
static char s_cardCvv[5]   = "";
static bool s_confirmed    = false;

// ── Icon helpers ──────────────────────────────────────────────────────────────
static void PUProfile(ImDrawList* dl, ImVec2 c, float r) {
    ImU32 col = IM_COL32(200,200,210,255);
    dl->AddCircleFilled(c, r, IM_COL32(48,48,56,255), 32);
    dl->AddCircle(c, r, IM_COL32(72,72,82,255), 32, 1.0f);
    dl->AddCircleFilled({ c.x, c.y-r*0.24f }, r*0.34f, col, 20);
    dl->PushClipRect({ c.x-r, c.y-r }, { c.x+r, c.y+r }, true);
    dl->AddCircleFilled({ c.x, c.y+r*0.62f }, r*0.52f, col, 24);
    dl->PopClipRect();
}
static void PUCheck(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    dl->AddCircleFilled(c, r, IM_COL32(30,50,40,255), 40);
    dl->AddCircle(c, r, col, 40, 2.0f);
    float s = r * 0.45f;
    dl->AddLine({ c.x - s*0.9f, c.y }, { c.x - s*0.15f, c.y + s*0.85f }, col, 3.0f);
    dl->AddLine({ c.x - s*0.15f, c.y + s*0.85f }, { c.x + s, c.y - s*0.7f }, col, 3.0f);
}
static void PUSearch(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    ImVec2 oc = { c.x - r*0.08f, c.y - r*0.08f };
    dl->AddCircle(oc, r*0.58f, col, 24, 1.8f);
    float a = 0.785f;
    ImVec2 ls = { oc.x + r*0.58f*cosf(a), oc.y + r*0.58f*sinf(a) };
    dl->AddLine(ls, { ls.x + r*0.42f*cosf(a), ls.y + r*0.42f*sinf(a) }, col, 1.8f);
}
static void PUPin(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    dl->AddCircleFilled({ c.x, c.y-r*0.18f }, r*0.42f, col, 20);
    dl->AddTriangleFilled(
        { c.x-r*0.35f, c.y-r*0.05f },
        { c.x+r*0.35f, c.y-r*0.05f },
        { c.x, c.y+r*0.92f }, col);
    dl->AddCircleFilled({ c.x, c.y-r*0.18f }, r*0.18f, PU_BG, 16);
}

// ── Styled InputText helper ───────────────────────────────────────────────────
static void PUInput(ImFont** F, const char* label, const char* id,
                    char* buf, int bufsz, float x, float y, float w,
                    bool password = false)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    constexpr float FH = 44.0f;

    // Label
    ImGui::PushFont(F[1]);
    float labelH = F[1]->LegacySize;
    dl->AddText({ x, y }, PU_MUTED, label);
    ImGui::PopFont();
    float fy = y + labelH + 6.0f;

    // Frame background
    dl->AddRectFilled({ x, fy }, { x+w, fy+FH }, PU_CARD, 10.0f);
    dl->AddRect({ x, fy }, { x+w, fy+FH }, PU_BORDER, 10.0f, 0, 1.0f);

    // InputText
    ImGui::SetCursorScreenPos({ x, fy });
    ImGui::PushStyleColor(ImGuiCol_FrameBg,        ImVec4(32/255.f,32/255.f,38/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(38/255.f,38/255.f,46/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  ImVec4(44/255.f,44/255.f,54/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_Border,         ImVec4(52/255.f,52/255.f,58/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_Text,           ImVec4(1,1,1,1));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14.0f, (FH - F[1]->LegacySize) * 0.5f));
    ImGui::PushFont(F[1]);
    ImGui::SetNextItemWidth(w);
    ImGuiInputTextFlags flags = password ? ImGuiInputTextFlags_Password : 0;
    ImGui::InputText(id, buf, bufsz, flags);
    ImGui::PopFont();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(5);
}

// ─────────────────────────────────────────────────────────────────────────────
void RenderPurchase(
    bool& goBack, bool& goHome, bool& goTickets, bool& goSchedule,
    int movieIndex, int seatCount, int timeIdx,
    int dayOffset, int weekStart,
    ID3D11ShaderResourceView** posters,
    int* /*posterWidths*/, int* /*posterHeights*/,
    BookingService* bookingSvc)
{
    // Reset confirmed flag when entering a new purchase session
    static int s_lastMovie = -9999;
    if (movieIndex != s_lastMovie) {
        s_confirmed = false;
        s_payMethod = 0;
        s_cardNum[0] = s_cardName[0] = s_cardExp[0] = s_cardCvv[0] = '\0';
        s_lastMovie = movieIndex;
    }

    ImGuiIO&    io   = ImGui::GetIO();
    ImFont**    F    = io.Fonts->Fonts.Data;
    float       winW = io.DisplaySize.x;
    float       winH = io.DisplaySize.y;

    int  idx   = (movieIndex >= 0 && movieIndex < 9) ? movieIndex : 0;
    int  nTix  = seatCount > 0 ? seatCount : 1;
    int  price = nTix * 15;
    int  tax   = (int)(price * 0.1f + 0.5f);
    int  total = price + tax;

    const PurchaseMovie& pm = g_pmov[idx];
    const char* timeStr = pm.times[(timeIdx >= 0 && timeIdx < 4) ? timeIdx : 0];

    // Compute selected date
    time_t now = time(nullptr);
    time_t sel = now + (time_t)(weekStart + dayOffset) * 86400;
    struct tm sd; localtime_s(&sd, &sel);
    char dateStr[32];
    snprintf(dateStr, sizeof(dateStr), "%s, %d %s",
             s_dayNames3[sd.tm_wday], sd.tm_mday, s_monNames[sd.tm_mon]);

    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::PushStyleColor(ImGuiCol_WindowBg,
        ImVec4(21/255.f, 21/255.f, 21/255.f, 1.f));
    ImGui::Begin("PurchasePage", nullptr,
        ImGuiWindowFlags_NoTitleBar   | ImGuiWindowFlags_NoResize   |
        ImGuiWindowFlags_NoCollapse   | ImGuiWindowFlags_NoMove     |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar  | ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2      orig = ImGui::GetWindowPos();
    ImDrawList* dl   = ImGui::GetWindowDrawList();

    // ── Navbar ────────────────────────────────────────────────────────────────
    constexpr float NAV_H = 64.0f;
    dl->AddRectFilled(orig, { orig.x+winW, orig.y+NAV_H }, PU_BG);
    dl->AddLine({ orig.x, orig.y+NAV_H }, { orig.x+winW, orig.y+NAV_H }, PU_BORDER, 1.0f);

    ImGui::PushFont(F[3]);
    float logoY = orig.y + (NAV_H - F[3]->LegacySize) * 0.5f;
    dl->AddText({ orig.x+36.0f, logoY }, PU_LOGO, "CINEMA");
    float logoW = ImGui::CalcTextSize("CINEMA").x;
    ImGui::PopFont();

    ImGui::PushFont(F[0]);
    const char* navItems[] = { "Home", "Your Tickets", "Schedule" };
    float nx = orig.x + 36.0f + logoW + 32.0f;
    for (int i = 0; i < 3; i++) {
        ImVec2 tsz = ImGui::CalcTextSize(navItems[i]);
        float  ty  = orig.y + (NAV_H - tsz.y) * 0.5f;
        dl->AddText({ nx, ty }, PU_MUTED, navItems[i]);
        ImGui::SetCursorScreenPos({ nx-4.0f, orig.y+8.0f });
        ImGui::InvisibleButton(("pu_nav_"+std::string(navItems[i])).c_str(),
                               { tsz.x+8.0f, NAV_H-16.0f });
        if (ImGui::IsItemClicked()) {
            if (i == 0) goHome     = true;
            if (i == 1) goTickets  = true;
            if (i == 2) goSchedule = true;
        }
        nx += tsz.x + 42.0f;
    }
    ImGui::PopFont();

    // Profile
    constexpr float PROF_R = 18.0f;
    ImVec2 profC = { orig.x+winW-28.0f-PROF_R, orig.y+NAV_H*0.5f };
    ImGui::SetCursorScreenPos({ profC.x-PROF_R, profC.y-PROF_R });
    ImGui::InvisibleButton("pu_prof", { PROF_R*2, PROF_R*2 });
    PUProfile(dl, profC, PROF_R);

    // City
    constexpr float CITY_W = 90.0f, CITY_H = 36.0f;
    float cityLeft = profC.x - PROF_R - 16.0f - CITY_W;
    float cityTop  = orig.y + (NAV_H - CITY_H) * 0.5f;
    ImGui::SetCursorScreenPos({ cityLeft, cityTop });
    ImGui::InvisibleButton("pu_city", { CITY_W, CITY_H });
    bool puCityHov = ImGui::IsItemHovered();
    RenderCitySelector(F, { cityLeft, cityTop }, CITY_W, CITY_H, ImGui::IsItemClicked());
    ImGui::PushFont(F[1]);
    dl->AddText({ cityLeft+20.0f, cityTop+(CITY_H-F[1]->LegacySize)*0.5f },
                puCityHov ? PU_WHITE : PU_MUTED, g_selectedCity.c_str());
    ImGui::PopFont();
    PUPin(dl, { cityLeft+11.0f, cityTop+CITY_H*0.5f }, 7.0f, PU_ACCENT);

    // Search bar
    constexpr float SB_W = 280.0f, SB_H = 38.0f;
    float sbX = cityLeft - 16.0f - SB_W;
    float sbY = orig.y + (NAV_H - SB_H) * 0.5f;
    dl->AddRectFilled({ sbX,sbY }, { sbX+SB_W,sbY+SB_H }, IM_COL32(28,28,28,255), 19.0f);
    dl->AddRect(      { sbX,sbY }, { sbX+SB_W,sbY+SB_H }, IM_COL32(55,55,62,255), 19.0f, 0, 1.0f);
    ImGui::PushFont(F[1]);
    dl->AddText({ sbX+16.0f, sbY+(SB_H-F[1]->LegacySize)*0.5f },
                IM_COL32(144,136,144,115), "Search a movie");
    ImGui::PopFont();
    PUSearch(dl, { sbX+SB_W-14.0f, sbY+SB_H*0.5f }, 8.0f, IM_COL32(144,136,144,145));

    // ── Page title ────────────────────────────────────────────────────────────
    constexpr float SIDE_PAD = 36.0f;
    float titleY = orig.y + NAV_H + 28.0f;
    ImGui::PushFont(F[3]);
    dl->AddText({ orig.x+SIDE_PAD, titleY }, PU_WHITE, "Complete Your Purchase");
    ImGui::PopFont();

    // ── Two-column content ────────────────────────────────────────────────────
    constexpr float SPLIT    = 0.52f;
    float contentTop = titleY + F[3]->LegacySize + 22.0f;
    float splitX     = winW * SPLIT;

    // Vertical divider
    float divBot = orig.y + winH - 70.0f;
    dl->AddLine({ orig.x+splitX, contentTop }, { orig.x+splitX, divBot }, PU_BORDER, 1.0f);

    // ── LEFT: Order Summary ───────────────────────────────────────────────────
    float lx = orig.x + SIDE_PAD;
    float ly = contentTop;
    float lw = splitX - SIDE_PAD * 2.0f;

    // Section header
    ImGui::PushFont(F[4]);
    dl->AddText({ lx, ly }, PU_ACCENT, "Order Summary");
    ly += F[4]->LegacySize + 16.0f;
    ImGui::PopFont();

    // Summary card
    constexpr float SUMCARD_H = 180.0f;
    ImVec2 scTL = { lx, ly }, scBR = { lx+lw, ly+SUMCARD_H };
    dl->AddRectFilled(scTL, scBR, PU_CARD, 14.0f);
    dl->AddRect(scTL, scBR, IM_COL32(200,65,95,140), 14.0f, 0, 1.5f);

    // Poster inside card
    constexpr float P_W = 88.0f, P_H = 124.0f;
    float pPad = 18.0f;
    float pOffY = (SUMCARD_H - P_H) * 0.5f;
    ImVec2 pTL = { lx+pPad, ly+pOffY }, pBR = { lx+pPad+P_W, ly+pOffY+P_H };
    if (posters && posters[idx])
        dl->AddImage((ImTextureID)posters[idx], pTL, pBR, {0,0},{1,1}, PU_WHITE);
    else
        dl->AddRectFilled(pTL, pBR, IM_COL32(34,34,44,255), 6.0f);
    dl->AddRect(pTL, pBR, IM_COL32(200,80,110,50), 6.0f, 0, 1.0f);

    // Info beside poster
    float ix = pBR.x + 18.0f;
    float iy = ly + 20.0f;
    float iw = lw - (P_W + pPad + 18.0f) - 16.0f;

    ImGui::PushFont(F[4]);
    dl->AddText(ImGui::GetFont(), F[4]->LegacySize,
                { ix, iy }, PU_TITLE, pm.title, nullptr, iw);
    float ttH = ImGui::GetFont()->CalcTextSizeA(F[4]->LegacySize, iw, iw, pm.title).y;
    iy += ttH + 8.0f;
    ImGui::PopFont();

    ImGui::PushFont(F[1]);
    // Date row
    char dateBuf[48]; snprintf(dateBuf, sizeof(dateBuf), "Date:  %s", dateStr);
    dl->AddText({ ix, iy }, PU_MUTED, dateBuf);
    iy += F[1]->LegacySize + 5.0f;
    // Time row
    char timeBuf[32]; snprintf(timeBuf, sizeof(timeBuf), "Time:  %s", timeStr);
    dl->AddText({ ix, iy }, PU_MUTED, timeBuf);
    iy += F[1]->LegacySize + 5.0f;
    // Hall row
    dl->AddText({ ix, iy }, PU_MUTED, "Hall:  A");
    iy += F[1]->LegacySize + 10.0f;
    // Seats + price
    char seatBuf[48]; snprintf(seatBuf, sizeof(seatBuf), "%d ticket%s  @  $15 each",
                               nTix, nTix == 1 ? "" : "s");
    dl->AddText({ ix, iy }, PU_WHITE, seatBuf);
    ImGui::PopFont();

    ly += SUMCARD_H + 16.0f;

    // Totals card
    constexpr float TOT_H = 106.0f;
    ImVec2 totTL = { lx, ly }, totBR = { lx+lw, ly+TOT_H };
    dl->AddRectFilled(totTL, totBR, PU_CARD, 14.0f);
    dl->AddRect(totTL, totBR, PU_BORDER, 14.0f, 0, 1.0f);

    ImGui::PushFont(F[1]);
    float ty = ly + 18.0f;
    float rightX = lx + lw - 18.0f;

    auto DrawRow = [&](const char* lbl, const char* val, ImU32 lblCol, ImU32 valCol) {
        dl->AddText({ lx+18.0f, ty }, lblCol, lbl);
        ImVec2 vSz = ImGui::CalcTextSize(val);
        dl->AddText({ rightX - vSz.x, ty }, valCol, val);
        ty += F[1]->LegacySize + 8.0f;
    };
    char subBuf[16], taxBuf[16], totBuf[16];
    snprintf(subBuf, 16, "$%d.00", price);
    snprintf(taxBuf, 16, "$%d.00", tax);
    snprintf(totBuf, 16, "$%d.00", total);

    DrawRow("Subtotal",  subBuf, PU_MUTED, PU_WHITE);

    // Divider between subtotal+tax and total
    ty += 2.0f;
    dl->AddLine({ lx+14.0f, ty }, { lx+lw-14.0f, ty }, PU_BORDER, 1.0f);
    ty += 8.0f;

    // Total (bold via larger font)
    ImGui::PopFont();
    ImGui::PushFont(F[3]);
    dl->AddText({ lx+18.0f, ty }, PU_WHITE, "Total");
    ImVec2 totSz = ImGui::CalcTextSize(totBuf);
    dl->AddText({ rightX - totSz.x, ty }, PU_TITLE, totBuf);
    ImGui::PopFont();

    // ── RIGHT: Payment ────────────────────────────────────────────────────────
    float rx = orig.x + splitX + SIDE_PAD;
    float ry = contentTop;
    float rw = winW - splitX - SIDE_PAD * 2.0f;

    // Section header
    ImGui::PushFont(F[4]);
    dl->AddText({ rx, ry }, PU_ACCENT, "Payment");
    ry += F[4]->LegacySize + 16.0f;
    ImGui::PopFont();

    // Payment method selector
    ImGui::PushFont(F[1]);
    dl->AddText({ rx, ry }, PU_MUTED, "Payment Method");
    ry += F[1]->LegacySize + 10.0f;
    ImGui::PopFont();

    const char* methods[] = { "Credit Card", "Google Pay", "Apple Pay", "Cash on Place" };
    constexpr float METHOD_H = 46.0f;
    float mw = (rw - 24.0f) / 4.0f;
    for (int i = 0; i < 4; i++) {
        float mx = rx + i * (mw + 8.0f);
        bool sel = (s_payMethod == i);
        ImVec2 mTL = { mx, ry }, mBR = { mx+mw, ry+METHOD_H };
        ImGui::SetCursorScreenPos(mTL);
        char mid[16]; snprintf(mid, 16, "pm_%d", i);
        ImGui::InvisibleButton(mid, { mw, METHOD_H });
        if (ImGui::IsItemClicked()) s_payMethod = i;
        bool mHov = ImGui::IsItemHovered();
        dl->AddRectFilled(mTL, mBR,
            sel  ? IM_COL32(60,20,36,255) :
            mHov ? IM_COL32(38,38,46,255) : PU_CARD, 10.0f);
        dl->AddRect(mTL, mBR,
            sel  ? PU_TITLE :
            mHov ? PU_ACCENT : PU_BORDER, 10.0f, 0, sel ? 2.0f : 1.0f);
        ImGui::PushFont(F[1]);
        ImVec2 msz = ImGui::CalcTextSize(methods[i]);
        dl->AddText({ mx+(mw-msz.x)*0.5f, ry+(METHOD_H-msz.y)*0.5f },
                    sel ? PU_WHITE : PU_MUTED, methods[i]);
        ImGui::PopFont();
    }
    ry += METHOD_H + 22.0f;

    // ── Payment form ──────────────────────────────────────────────────────────
    constexpr float FIELD_H_TOTAL = 44.0f + 18.0f + 8.0f; // field+label+gap

    if (s_payMethod == 0) {
        // Credit card form
        PUInput(F, "Card Number", "##cnum",  s_cardNum,  sizeof(s_cardNum),
                rx, ry, rw);
        ry += FIELD_H_TOTAL + 6.0f;

        PUInput(F, "Cardholder Name", "##cname", s_cardName, sizeof(s_cardName),
                rx, ry, rw);
        ry += FIELD_H_TOTAL + 6.0f;

        float halfW = (rw - 14.0f) * 0.5f;
        PUInput(F, "Expiry (MM/YY)", "##cexp", s_cardExp, sizeof(s_cardExp),
                rx, ry, halfW);
        PUInput(F, "CVV", "##ccvv", s_cardCvv, sizeof(s_cardCvv),
                rx + halfW + 14.0f, ry, halfW, true);
        ry += FIELD_H_TOTAL + 10.0f;
    } else {
        // Google/Apple Pay — no form, just a note
        ImVec2 noteTL = { rx, ry }, noteBR = { rx+rw, ry+64.0f };
        dl->AddRectFilled(noteTL, noteBR, PU_CARD, 12.0f);
        dl->AddRect(noteTL, noteBR, PU_BORDER, 12.0f, 0, 1.0f);
        ImGui::PushFont(F[1]);
        const char* note = (s_payMethod == 1)
            ? "You will be redirected to Google Pay\nto complete your payment securely."
            : (s_payMethod == 2)
            ? "You will be redirected to Apple Pay\nto complete your payment securely."
            : "Pay with cash at the cinema entrance.\nPlease arrive 10 minutes early.";
        ImVec2 nSz = ImGui::CalcTextSize(note, nullptr, false, rw - 28.0f);
        dl->AddText(ImGui::GetFont(), F[1]->LegacySize,
                    { rx + 14.0f, ry + (64.0f - nSz.y) * 0.5f },
                    PU_MUTED, note, nullptr, rw - 28.0f);
        ImGui::PopFont();
        ry += 64.0f + 10.0f;
    }

    // ── Bottom bar ────────────────────────────────────────────────────────────
    constexpr float BAR_H  = 70.0f;
    constexpr float PAY_H  = 48.0f;
    constexpr float BACK_W = 110.0f;
    float barY = orig.y + winH - BAR_H;
    dl->AddRectFilled({ orig.x, barY }, { orig.x+winW, orig.y+winH },
                      IM_COL32(24,24,28,255));
    dl->AddLine({ orig.x, barY }, { orig.x+winW, barY }, PU_BORDER, 1.0f);

    // Back button
    float backX = orig.x + SIDE_PAD;
    float backY = barY + (BAR_H - PAY_H) * 0.5f;
    ImGui::SetCursorScreenPos({ backX, backY });
    ImGui::InvisibleButton("pu_back", { BACK_W, PAY_H });
    bool backHov = ImGui::IsItemHovered();
    if (ImGui::IsItemClicked()) goBack = true;
    dl->AddRectFilled({ backX, backY }, { backX+BACK_W, backY+PAY_H },
        backHov ? IM_COL32(50,50,60,255) : PU_CARD, PAY_H*0.5f);
    dl->AddRect({ backX, backY }, { backX+BACK_W, backY+PAY_H },
        PU_BORDER, PAY_H*0.5f, 0, 1.0f);
    ImGui::PushFont(F[1]);
    ImVec2 bkSz = ImGui::CalcTextSize("< Back");
    dl->AddText({ backX+(BACK_W-bkSz.x)*0.5f, backY+(PAY_H-bkSz.y)*0.5f },
                PU_MUTED, "< Back");
    ImGui::PopFont();

    // Total display
    ImGui::PushFont(F[3]);
    char barTotal[24]; snprintf(barTotal, sizeof(barTotal), "Total: $%d", total);
    dl->AddText({ backX + BACK_W + 24.0f, barY + (BAR_H - F[3]->LegacySize) * 0.5f },
                PU_WHITE, barTotal);
    ImGui::PopFont();

    // Pay Now button
    float payW = splitX - SIDE_PAD - 40.0f;
    float payX = orig.x + winW - SIDE_PAD - payW;
    float payY = barY + (BAR_H - PAY_H) * 0.5f;
    ImGui::SetCursorScreenPos({ payX, payY });
    ImGui::InvisibleButton("pu_pay", { payW, PAY_H });
    bool payHov  = ImGui::IsItemHovered();
    bool payClk  = ImGui::IsItemClicked();
    bool canPay  = s_payMethod != 0 || (s_cardNum[0] && s_cardName[0] && s_cardExp[0] && s_cardCvv[0]);
    dl->AddRectFilled({ payX, payY }, { payX+payW, payY+PAY_H },
        !canPay ? IM_COL32(80,30,50,255) :
        payHov  ? IM_COL32(245,75,125,255) : PU_TITLE,
        PAY_H * 0.5f);
    if (payClk && canPay) {
        s_confirmed = true;
        for (int s = 0; s < nTix; s++) {
            PurchasedTicket tk;
            tk.posterIdx  = idx;
            tk.title      = pm.title;
            tk.hall       = "A";
            tk.row        = "B";
            char seatBuf[4];
            snprintf(seatBuf, sizeof(seatBuf), "%02d", s + 1);
            tk.seat       = seatBuf;
            tk.fmt        = "2 D";
            tk.time       = timeStr;
            tk.dayName    = s_dayNames3[sd.tm_wday];
            tk.dayNum     = sd.tm_mday;
            tk.month      = sd.tm_mon + 1;
            tk.year       = sd.tm_year + 1900;
            tk.ticketType = "Adult";
            g_purchasedTickets.push_back(tk);

            if (bookingSvc) {
                TicketRecord rec;
                rec.title      = tk.title;
                rec.hall       = tk.hall;
                rec.seat       = tk.seat;
                rec.fmt        = tk.fmt;
                rec.showTime   = tk.time;
                rec.day        = tk.dayNum;
                rec.month      = tk.month;
                rec.year       = tk.year;
                rec.ticketType = tk.ticketType;
                rec.city       = g_selectedCity;
                rec.location   = "Galleria Burgas";
                bookingSvc->BookTicket(rec);
            }
        }
    }
    ImGui::PushFont(F[0]);
    char payLabel[24]; snprintf(payLabel, sizeof(payLabel), "Pay $%d", total);
    ImVec2 plSz = ImGui::CalcTextSize(payLabel);
    dl->AddText({ payX+(payW-plSz.x)*0.5f, payY+(PAY_H-plSz.y)*0.5f },
                canPay ? PU_WHITE : IM_COL32(144,80,100,255), payLabel);
    ImGui::PopFont();

    // ── Confirmation overlay ──────────────────────────────────────────────────
    if (s_confirmed) {
        // Darken background
        dl->AddRectFilled(orig, { orig.x+winW, orig.y+winH },
                          IM_COL32(0,0,0,200));

        // Centered modal card
        constexpr float MODAL_W = 420.0f, MODAL_H = 320.0f;
        float mx = orig.x + (winW - MODAL_W) * 0.5f;
        float my = orig.y + (winH - MODAL_H) * 0.5f;
        dl->AddRectFilled({ mx, my }, { mx+MODAL_W, my+MODAL_H },
                          IM_COL32(32,22,30,255), 20.0f);
        dl->AddRect({ mx, my }, { mx+MODAL_W, my+MODAL_H },
                    PU_TITLE, 20.0f, 0, 2.0f);

        // Checkmark
        constexpr float CHK_R = 34.0f;
        float cky = my + 52.0f;
        PUCheck(dl, { mx+MODAL_W*0.5f, cky }, CHK_R, PU_GREEN);

        // "Booking Confirmed!" title
        float textY = cky + CHK_R + 18.0f;
        ImGui::PushFont(F[3]);
        const char* conf = "Booking Confirmed!";
        ImVec2 cSz = ImGui::CalcTextSize(conf);
        dl->AddText({ mx+(MODAL_W-cSz.x)*0.5f, textY }, PU_GREEN, conf);
        textY += cSz.y + 10.0f;
        ImGui::PopFont();

        // Movie title
        ImGui::PushFont(F[4]);
        ImVec2 mttSz = ImGui::GetFont()->CalcTextSizeA(
            F[4]->LegacySize, MODAL_W-48.0f, MODAL_W-48.0f, pm.title);
        dl->AddText(ImGui::GetFont(), F[4]->LegacySize,
                    { mx+(MODAL_W-mttSz.x)*0.5f, textY }, PU_WHITE, pm.title);
        textY += mttSz.y + 8.0f;
        ImGui::PopFont();

        // Date + time line
        ImGui::PushFont(F[1]);
        char summaryLine[64];
        snprintf(summaryLine, sizeof(summaryLine), "%s  •  %s  •  Hall A", dateStr, timeStr);
        ImVec2 slSz = ImGui::CalcTextSize(summaryLine);
        dl->AddText({ mx+(MODAL_W-slSz.x)*0.5f, textY }, PU_MUTED, summaryLine);
        textY += slSz.y + 24.0f;
        ImGui::PopFont();

        // "View My Tickets" button
        constexpr float BTN_W = 220.0f, BTN_H = 44.0f;
        float btnX = mx + (MODAL_W - BTN_W) * 0.5f;
        ImGui::SetCursorScreenPos({ btnX, textY });
        ImGui::InvisibleButton("pu_view_tix", { BTN_W, BTN_H });
        bool vtHov = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked()) goTickets = true;
        dl->AddRectFilled({ btnX, textY }, { btnX+BTN_W, textY+BTN_H },
            vtHov ? IM_COL32(245,75,125,255) : PU_TITLE, BTN_H*0.5f);
        ImGui::PushFont(F[1]);
        ImVec2 vSz = ImGui::CalcTextSize("View My Tickets");
        dl->AddText({ btnX+(BTN_W-vSz.x)*0.5f, textY+(BTN_H-vSz.y)*0.5f },
                    PU_WHITE, "View My Tickets");
        ImGui::PopFont();

        // Close / Back to Home
        textY += BTN_H + 10.0f;
        ImGui::PushFont(F[1]);
        const char* bkLbl = "Back to Home";
        ImVec2 bkSz2 = ImGui::CalcTextSize(bkLbl);
        float bkX2 = mx + (MODAL_W - bkSz2.x) * 0.5f;
        ImGui::SetCursorScreenPos({ bkX2 - 4.0f, textY - 4.0f });
        ImGui::InvisibleButton("pu_home2", { bkSz2.x + 8.0f, bkSz2.y + 8.0f });
        bool bkHov2 = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked()) goHome = true;
        dl->AddText({ bkX2, textY },
                    bkHov2 ? PU_WHITE : PU_MUTED, bkLbl);
        ImGui::PopFont();
    }

    (void)winH;
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}
