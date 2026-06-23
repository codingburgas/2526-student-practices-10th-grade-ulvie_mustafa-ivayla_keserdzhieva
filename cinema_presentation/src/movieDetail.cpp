#include "imgui.h"
#include <d3d11.h>
#include <string>
#include <cstdio>
#include <algorithm>

static constexpr ImU32 MD_BG     = IM_COL32( 21,  21,  21, 255);
static constexpr ImU32 MD_ACCENT = IM_COL32(233, 131, 160, 255);
static constexpr ImU32 MD_MUTED  = IM_COL32(144, 136, 144, 255);
static constexpr ImU32 MD_WHITE  = IM_COL32(255, 255, 255, 255);
static constexpr ImU32 MD_LOGO   = IM_COL32(163,  49,  82, 255);

static void MDChevronLeft(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    float s = r * 0.42f;
    dl->AddLine({ c.x + s * 0.35f, c.y - s }, { c.x - s * 0.35f, c.y }, col, 2.5f);
    dl->AddLine({ c.x - s * 0.35f, c.y },     { c.x + s * 0.35f, c.y + s }, col, 2.5f);
}

struct MovieData {
    const char* title;
    int         year;
    const char* genre;
    const char* duration;
    float       rating;
    const char* synopsis;
};

static const MovieData g_movieData[9] = {
    { "10 Things I Hate About You", 1999, "Romance · Comedy",   "1h 37m", 7.3f,
      "A popular girl can only date if her sharp-tongued older sister does first. "
      "A charming schemer is paid to woo the difficult sister, but their unlikely "
      "romance turns out to be the most genuine thing either has ever known." },
    { "Dead Poets Society",         1989, "Drama",              "2h 8m",  8.1f,
      "An unorthodox English teacher at an elite prep school inspires his students "
      "to seize the day, question authority, and discover the transformative "
      "power of poetry and free thought. Carpe diem." },
    { "La La Land",                 2016, "Romance · Musical",  "2h 8m",  8.0f,
      "A jazz pianist and an aspiring actress fall in love while chasing "
      "their dreams across Los Angeles. A visually dazzling ode to ambition, "
      "artistry, and the bittersweet cost of following your heart." },
    { "Little Women",               2019, "Drama",              "2h 15m", 7.8f,
      "The four March sisters navigate love, loss, and independence in "
      "post-Civil War New England. A timeless story of family, creativity, "
      "and the quiet courage it takes to live life entirely on your own terms." },
    { "Matilda",                    1996, "Comedy · Family",    "1h 38m", 6.9f,
      "A brilliant girl with extraordinary telekinetic powers escapes her "
      "neglectful parents and a terrifying headmistress, finding solace "
      "in books, kindness, and the magic she has always carried within herself." },
    { "The Drama",                  2023, "Drama",              "2h 0m",  7.0f,
      "An emotional journey through loss, identity, and the unexpected "
      "connections that quietly bind us together. A slow-burn, powerful "
      "film about the small moments that end up changing everything." },
    { "The Little Prince",          2015, "Animation · Family", "1h 48m", 7.7f,
      "A little girl in a rigidly scheduled world befriends an eccentric aviator "
      "who tells her the extraordinary story of the Little Prince — a boy "
      "who traveled from his tiny planet to learn what truly matters in life." },
    { "The Princess Diaries",       2001, "Comedy · Romance",   "1h 51m", 6.3f,
      "An awkward San Francisco teenager discovers she is the heir to the "
      "throne of a small European kingdom. A charming coming-of-age story "
      "about identity, grace under pressure, and daring to believe in yourself." },
    { "The Terminal",               2004, "Comedy · Drama",     "2h 8m",  7.4f,
      "A man from Eastern Europe arrives at JFK airport only to find his "
      "country no longer exists, leaving him stranded and unable to enter the US. "
      "He turns the terminal into an unlikely home — proving human warmth "
      "can thrive absolutely anywhere." }
};

void RenderMovieDetail(
    int movieIndex, bool& goBack,
    ID3D11ShaderResourceView** posters, int* /*posterWidths*/, int* /*posterHeights*/)
{
    ImGuiIO&    io   = ImGui::GetIO();
    ImFont**    F    = io.Fonts->Fonts.Data;
    float       winW = io.DisplaySize.x;

    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(21/255.f, 21/255.f, 21/255.f, 1.f));
    ImGui::Begin("MovieDetail", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize   |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove     |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings);

    ImVec2      orig = ImGui::GetWindowPos();
    ImDrawList* dl   = ImGui::GetWindowDrawList();

    int idx = (movieIndex >= 0 && movieIndex < 9) ? movieIndex : 0;
    const MovieData& mv = g_movieData[idx];

    // ── Navbar ────────────────────────────────────────────────────────────────
    constexpr float NAV_H = 64.0f;
    dl->AddRectFilled(orig, { orig.x + winW, orig.y + NAV_H }, MD_BG);
    dl->AddLine({ orig.x, orig.y + NAV_H }, { orig.x + winW, orig.y + NAV_H },
                IM_COL32(52, 52, 58, 255), 1.0f);

    ImGui::PushFont(F[3]); // Geologica 24px
    dl->AddText({ orig.x + 36.0f, orig.y + (NAV_H - 24.0f) * 0.5f }, MD_LOGO, "CINEMA");
    ImGui::PopFont();

    // ── Back button ───────────────────────────────────────────────────────────
    constexpr float SIDE_PAD = 60.0f;
    float backY = orig.y + NAV_H + 24.0f;
    constexpr float BACK_W = 88.0f, BACK_H = 36.0f;

    ImGui::SetCursorScreenPos({ orig.x + SIDE_PAD, backY });
    ImGui::InvisibleButton("backBtn", { BACK_W, BACK_H });
    bool backHov = ImGui::IsItemHovered();
    if (ImGui::IsItemClicked()) goBack = true;

    ImU32 backCol = backHov ? MD_WHITE : MD_MUTED;
    MDChevronLeft(dl, { orig.x + SIDE_PAD + 10.0f, backY + BACK_H * 0.5f }, 12.0f, backCol);
    ImGui::PushFont(F[0]); // Inter 18px
    dl->AddText({ orig.x + SIDE_PAD + 26.0f, backY + (BACK_H - 18.0f) * 0.5f }, backCol, "Back");
    ImGui::PopFont();

    // ── Content ───────────────────────────────────────────────────────────────
    float contentY = backY + BACK_H + 28.0f;

    // Poster
    constexpr float POSTER_W = 320.0f, POSTER_H = 472.0f;
    float posterX = orig.x + SIDE_PAD;
    float posterY = contentY;

    if (posters && posters[idx])
        dl->AddImageRounded((ImTextureID)posters[idx],
            { posterX, posterY }, { posterX + POSTER_W, posterY + POSTER_H },
            { 0,0 }, { 1,1 }, MD_WHITE, 12.0f);
    else
        dl->AddRectFilled({ posterX, posterY },
            { posterX + POSTER_W, posterY + POSTER_H }, IM_COL32(34,34,44,255), 12.0f);

    dl->AddRect({ posterX, posterY }, { posterX + POSTER_W, posterY + POSTER_H },
                IM_COL32(233,131,160,55), 12.0f, 0, 1.5f);

    // Info
    float infoX = posterX + POSTER_W + 56.0f;
    float infoW = winW - infoX - SIDE_PAD;
    float curY  = contentY;

    // Title
    ImGui::PushFont(F[5]); // Geologica 58px
    dl->AddText(ImGui::GetFont(), 58.0f, { infoX, curY },
                MD_WHITE, mv.title, nullptr, infoW);
    ImVec2 titleSz = ImGui::GetFont()->CalcTextSizeA(58.0f, infoW, infoW, mv.title);
    curY += titleSz.y + 14.0f;
    ImGui::PopFont();

    // Year · Genre · Duration
    ImGui::PushFont(F[0]); // Inter 18px
    char meta[160];
    snprintf(meta, sizeof(meta), "%d  ·  %s  ·  %s", mv.year, mv.genre, mv.duration);
    dl->AddText({ infoX, curY }, MD_MUTED, meta);
    curY += 18.0f + 16.0f;
    ImGui::PopFont();

    // Rating score + dot track
    {
        char ratingStr[16];
        snprintf(ratingStr, sizeof(ratingStr), "%.1f / 10", mv.rating);
        ImGui::PushFont(F[1]); // Inter 15px
        dl->AddText({ infoX, curY + 3.0f }, MD_ACCENT, ratingStr);
        ImGui::PopFont();

        float dotX = infoX + 72.0f;
        float dotY = curY + 10.0f;
        int filled = (int)(mv.rating + 0.5f);
        for (int i = 0; i < 10; i++) {
            ImU32 dc = (i < filled) ? IM_COL32(233,131,160,255) : IM_COL32(55,55,62,255);
            dl->AddCircleFilled({ dotX + i * 17.0f, dotY }, 5.0f, dc, 16);
        }
        curY += 28.0f + 18.0f;
    }

    // Divider
    dl->AddLine({ infoX, curY }, { infoX + infoW * 0.55f, curY },
                IM_COL32(52,52,58,255), 1.0f);
    curY += 20.0f;

    // Synopsis heading
    ImGui::PushFont(F[4]); // Geologica 20px
    dl->AddText({ infoX, curY }, MD_ACCENT, "Synopsis");
    curY += 20.0f + 10.0f;
    ImGui::PopFont();

    // Synopsis body
    ImGui::PushFont(F[0]); // Inter 18px
    dl->AddText(ImGui::GetFont(), 18.0f, { infoX, curY },
                IM_COL32(210, 205, 210, 255), mv.synopsis, nullptr, infoW * 0.85f);
    ImVec2 synSz = ImGui::GetFont()->CalcTextSizeA(18.0f, infoW * 0.85f, infoW * 0.85f, mv.synopsis);
    curY += synSz.y + 36.0f;
    ImGui::PopFont();

    // Book Ticket button
    constexpr float BTN_W = 200.0f, BTN_H = 52.0f;
    ImGui::SetCursorScreenPos({ infoX, curY });
    ImGui::InvisibleButton("bookBtn", { BTN_W, BTN_H });
    bool bookHov = ImGui::IsItemHovered();

    ImU32 btnFill = bookHov ? IM_COL32(236,64,113,255) : IM_COL32(163,49,82,255);
    dl->AddRectFilled({ infoX, curY }, { infoX + BTN_W, curY + BTN_H }, btnFill, 26.0f);
    ImGui::PushFont(F[3]); // Geologica 24px
    const char* btnLabel = "Book Ticket";
    ImVec2 bSz = ImGui::CalcTextSize(btnLabel);
    dl->AddText({ infoX + (BTN_W - bSz.x) * 0.5f, curY + (BTN_H - bSz.y) * 0.5f },
                MD_WHITE, btnLabel);
    ImGui::PopFont();

    float bottom = (std::max)(posterY + POSTER_H, curY + BTN_H) + 80.0f;
    ImGui::SetCursorScreenPos({ 0.0f, bottom });
    ImGui::Dummy({ 1.0f, 1.0f });

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}
