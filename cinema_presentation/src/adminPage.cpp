#include "imgui.h"
#include "citySelector.h"
#include "MovieService.h"
#include <d3d11.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cmath>

// ── Palette ────────────────────────────────────────────────────────────────────
static constexpr ImU32 AD_BG     = IM_COL32( 21,  21,  21, 255);
static constexpr ImU32 AD_CARD   = IM_COL32( 32,  32,  38, 255);
static constexpr ImU32 AD_CARD2  = IM_COL32( 28,  28,  34, 255);
static constexpr ImU32 AD_TITLE  = IM_COL32(236,  64, 113, 255);
static constexpr ImU32 AD_ACCENT = IM_COL32(233, 131, 160, 255);
static constexpr ImU32 AD_MUTED  = IM_COL32(144, 136, 144, 255);
static constexpr ImU32 AD_WHITE  = IM_COL32(255, 255, 255, 255);
static constexpr ImU32 AD_LOGO   = IM_COL32(163,  49,  82, 255);
static constexpr ImU32 AD_BORDER = IM_COL32( 52,  52,  58, 255);
static constexpr ImU32 AD_GREEN  = IM_COL32( 72, 199, 142, 255);
static constexpr ImU32 AD_RED    = IM_COL32(220,  60,  80, 255);

// ── Page-level state ──────────────────────────────────────────────────────────
static int  s_adTab          = 0;
static bool s_showMovieForm  = false;
static bool s_showShowForm   = false;
static bool s_editShowMode   = false;
static bool s_needRefresh    = true;
static bool s_deleteConfirmM = false;  // confirm delete movie
static bool s_deleteConfirmS = false;  // confirm delete show
static int  s_deleteTargetId = -1;
static std::string s_adError;
static std::string s_adSuccess;

// Form buffers — Add/Edit Movie
static char s_mvTitle[256] = "";
static char s_mvGenre[100] = "";
static int  s_mvYear       = 2024;

// Form buffers — Add/Edit Show
static int  s_shMovieSel    = 0;
static char s_shHall[10]    = "";
static int  s_shFormatSel   = 0;           // 0=2D 1=3D
static char s_shTimes[4][20] = {};
static int  s_editShowId    = -1;

// Loaded data
static std::vector<MovieRecord> s_movies;
static std::vector<ShowRecord>  s_shows;

// ── Icon helpers ───────────────────────────────────────────────────────────────
static void ADProfile(ImDrawList* dl, ImVec2 c, float r) {
    ImU32 col = IM_COL32(200,200,210,255);
    dl->AddCircleFilled(c, r, IM_COL32(48,48,56,255), 32);
    dl->AddCircle      (c, r, IM_COL32(72,72,82,255), 32, 1.0f);
    dl->AddCircleFilled({ c.x, c.y-r*0.24f }, r*0.34f, col, 20);
    dl->PushClipRect({ c.x-r, c.y-r }, { c.x+r, c.y+r }, true);
    dl->AddCircleFilled({ c.x, c.y+r*0.62f }, r*0.52f, col, 24);
    dl->PopClipRect();
}
static void ADPin(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    dl->AddCircleFilled({ c.x, c.y-r*0.18f }, r*0.42f, col, 20);
    dl->AddTriangleFilled(
        { c.x-r*0.35f, c.y-r*0.05f },
        { c.x+r*0.35f, c.y-r*0.05f },
        { c.x, c.y+r*0.92f }, col);
    dl->AddCircleFilled({ c.x, c.y-r*0.18f }, r*0.18f, AD_BG, 16);
}

// ── Styled button helper ───────────────────────────────────────────────────────
static bool ADButton(ImDrawList* dl, ImFont** F, const char* label, const char* id,
                     float x, float y, float w, float h,
                     ImU32 colNorm, ImU32 colHov, ImU32 textCol = AD_WHITE, float rounding = 8.0f)
{
    ImGui::SetCursorScreenPos({ x, y });
    ImGui::InvisibleButton(id, { w, h });
    bool hov = ImGui::IsItemHovered();
    bool clk = ImGui::IsItemClicked();
    dl->AddRectFilled({ x,y }, { x+w,y+h }, hov ? colHov : colNorm, rounding);
    ImGui::PushFont(F[1]);
    ImVec2 tsz = ImGui::CalcTextSize(label);
    dl->AddText({ x+(w-tsz.x)*0.5f, y+(h-tsz.y)*0.5f }, textCol, label);
    ImGui::PopFont();
    return clk;
}

// ── Styled InputText ───────────────────────────────────────────────────────────
static void ADInputText(ImFont** F, const char* label, const char* id,
                        char* buf, int bufsz, float w)
{
    ImGui::PushFont(F[1]);
    ImGui::TextUnformatted(label);
    ImGui::PopFont();
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_FrameBg,        ImVec4(32/255.f,32/255.f,40/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(40/255.f,40/255.f,50/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  ImVec4(48/255.f,48/255.f,60/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_Border,         ImVec4(72/255.f,72/255.f,82/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_Text,           ImVec4(1,1,1,1));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f,9.0f));
    ImGui::PushFont(F[1]);
    ImGui::SetNextItemWidth(w);
    ImGui::InputText(id, buf, bufsz);
    ImGui::PopFont();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(5);
}

static void ADInputInt(ImFont** F, const char* label, const char* id, int* v, float w) {
    ImGui::PushFont(F[1]);
    ImGui::TextUnformatted(label);
    ImGui::PopFont();
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_FrameBg,        ImVec4(32/255.f,32/255.f,40/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(40/255.f,40/255.f,50/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  ImVec4(48/255.f,48/255.f,60/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_Text,           ImVec4(1,1,1,1));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f,9.0f));
    ImGui::PushFont(F[1]);
    ImGui::SetNextItemWidth(w);
    ImGui::InputInt(id, v, 0, 0);
    ImGui::PopFont();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);
}

static void ADCombo(ImFont** F, const char* label, const char* id,
                    int* selected, const char* const* items, int count, float w)
{
    ImGui::PushFont(F[1]);
    ImGui::TextUnformatted(label);
    ImGui::PopFont();
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_FrameBg,        ImVec4(32/255.f,32/255.f,40/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(40/255.f,40/255.f,50/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  ImVec4(48/255.f,48/255.f,60/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg,        ImVec4(24/255.f,24/255.f,30/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_Text,           ImVec4(1,1,1,1));
    ImGui::PushStyleColor(ImGuiCol_Header,         ImVec4(60/255.f,20/255.f,36/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered,  ImVec4(80/255.f,30/255.f,50/255.f,1.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f,9.0f));
    ImGui::PushFont(F[1]);
    ImGui::SetNextItemWidth(w);
    if (*selected < 0 || *selected >= count) *selected = 0;
    ImGui::Combo(id, selected, items, count);
    ImGui::PopFont();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(7);
}

// ── Open-popup triggers (set before Begin / after Begin is ambiguous) ──────────
static bool s_openMoviePopup = false;
static bool s_openShowPopup  = false;
static bool s_openDelMPopup  = false;
static bool s_openDelSPopup  = false;

// ── Popup modals ──────────────────────────────────────────────────────────────
static void ShowMovieFormPopup(ImFont** F, MovieService* svc) {
    ImGui::SetNextWindowSize({ 460.0f, 0.0f });
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, { 0.5f,0.5f });
    ImGui::PushStyleColor(ImGuiCol_PopupBg,      ImVec4(24/255.f,18/255.f,22/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0,0,0,0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  ImVec2(28.0f,28.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,    ImVec2(0.0f,14.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.0f);

    if (ImGui::BeginPopupModal("Add Movie##pop", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {

        ImDrawList* pdl = ImGui::GetWindowDrawList();
        ImVec2 wpos = ImGui::GetWindowPos();
        ImVec2 wsz  = ImGui::GetWindowSize();
        pdl->AddRect(wpos, { wpos.x+wsz.x, wpos.y+wsz.y },
                     IM_COL32(60,20,36,255), 16.0f, 0, 1.5f);

        ImGui::PushFont(F[3]);
        ImGui::TextColored({236/255.f,64/255.f,113/255.f,1.f}, "Add Movie");
        ImGui::PopFont();
        ImGui::Dummy({0,4});

        static std::string formErr;
        ADInputText(F, "Title *", "##mp_title", s_mvTitle, sizeof(s_mvTitle), 400.0f);
        ADInputText(F, "Genre",   "##mp_genre", s_mvGenre, sizeof(s_mvGenre), 400.0f);
        ADInputInt (F, "Year",    "##mp_year",  &s_mvYear, 120.0f);

        if (!formErr.empty()) {
            ImGui::PushFont(F[1]);
            ImGui::TextColored({220/255.f,60/255.f,80/255.f,1.f}, "%s", formErr.c_str());
            ImGui::PopFont();
        }

        ImGui::Dummy({0,8});
        float bw = 170.0f;
        ImGui::SetCursorPosX((400.0f - bw*2.0f - 12.0f) * 0.5f + 28.0f);

        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(44/255.f,44/255.f,54/255.f,1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(60/255.f,60/255.f,72/255.f,1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(50/255.f,50/255.f,62/255.f,1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f,10.0f));
        ImGui::PushFont(F[1]);
        ImGui::SetNextItemWidth(bw);
        if (ImGui::Button("Cancel##mp", { bw, 40.0f })) {
            formErr.clear(); ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine(0, 12.0f);
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(236/255.f,64/255.f,113/255.f,1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(245/255.f,85/255.f,130/255.f,1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(220/255.f,50/255.f,100/255.f,1.f));
        if (ImGui::Button("Add Movie##mp_ok", { bw, 40.0f })) {
            formErr.clear();
            if (s_mvTitle[0] == '\0') {
                formErr = "Title is required.";
            } else if (svc && svc->AddMovie(s_mvTitle, s_mvGenre, s_mvYear)) {
                s_adSuccess  = std::string("Movie '") + s_mvTitle + "' added.";
                s_adError.clear();
                s_needRefresh = true;
                formErr.clear();
                ImGui::CloseCurrentPopup();
            } else {
                formErr = svc ? svc->GetLastError() : "Service unavailable.";
            }
        }
        ImGui::PopStyleColor(3);
        ImGui::PopFont();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

static void ShowShowFormPopup(ImFont** F, MovieService* svc) {
    const char* title = s_editShowMode ? "Edit Show##pop" : "Add Show##pop";
    ImGui::SetNextWindowSize({ 480.0f, 0.0f });
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, { 0.5f,0.5f });
    ImGui::PushStyleColor(ImGuiCol_PopupBg,      ImVec4(24/255.f,18/255.f,22/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0,0,0,0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  ImVec2(28.0f,28.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,    ImVec2(0.0f,12.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.0f);

    if (ImGui::BeginPopupModal(title, nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {

        ImDrawList* pdl = ImGui::GetWindowDrawList();
        ImVec2 wpos = ImGui::GetWindowPos();
        ImVec2 wsz  = ImGui::GetWindowSize();
        pdl->AddRect(wpos, { wpos.x+wsz.x, wpos.y+wsz.y },
                     IM_COL32(60,20,36,255), 16.0f, 0, 1.5f);

        ImGui::PushFont(F[3]);
        ImGui::TextColored({236/255.f,64/255.f,113/255.f,1.f},
                           s_editShowMode ? "Edit Show" : "Add Show");
        ImGui::PopFont();
        ImGui::Dummy({0,4});

        static std::string formErr;

        // Movie combo
        {
            std::vector<const char*> movieNames;
            for (auto& m : s_movies) movieNames.push_back(m.title.c_str());
            if (movieNames.empty()) movieNames.push_back("(no movies)");
            ADCombo(F, "Movie *", "##sh_movie", &s_shMovieSel,
                    movieNames.data(), (int)movieNames.size(), 420.0f);
        }

        ADInputText(F, "Hall * (e.g. A)", "##sh_hall", s_shHall, sizeof(s_shHall), 120.0f);

        {
            const char* fmts[] = { "2D", "3D" };
            ADCombo(F, "Format", "##sh_fmt", &s_shFormatSel, fmts, 2, 120.0f);
        }

        // Times
        ADInputText(F, "Time 1 (e.g. 10:00 AM)", "##sh_t1", s_shTimes[0], 20, 200.0f);
        ADInputText(F, "Time 2",                  "##sh_t2", s_shTimes[1], 20, 200.0f);
        ADInputText(F, "Time 3",                  "##sh_t3", s_shTimes[2], 20, 200.0f);
        ADInputText(F, "Time 4",                  "##sh_t4", s_shTimes[3], 20, 200.0f);

        if (!formErr.empty()) {
            ImGui::PushFont(F[1]);
            ImGui::TextColored({220/255.f,60/255.f,80/255.f,1.f}, "%s", formErr.c_str());
            ImGui::PopFont();
        }

        ImGui::Dummy({0,8});
        float bw = 180.0f;

        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(44/255.f,44/255.f,54/255.f,1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(60/255.f,60/255.f,72/255.f,1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(50/255.f,50/255.f,62/255.f,1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f,10.0f));
        ImGui::PushFont(F[1]);
        if (ImGui::Button("Cancel##sh", { bw, 40.0f })) {
            formErr.clear(); ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine(0, 12.0f);
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(236/255.f,64/255.f,113/255.f,1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(245/255.f,85/255.f,130/255.f,1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(220/255.f,50/255.f,100/255.f,1.f));
        const char* okLabel = s_editShowMode ? "Save Changes##sh_ok" : "Add Show##sh_ok";
        if (ImGui::Button(okLabel, { bw, 40.0f })) {
            formErr.clear();
            if (s_shHall[0] == '\0') {
                formErr = "Hall is required.";
            } else if (s_movies.empty()) {
                formErr = "No movies available. Add a movie first.";
            } else {
                ShowRecord rec;
                rec.movieId    = s_movies[s_shMovieSel].id;
                rec.hall       = s_shHall;
                rec.format     = (s_shFormatSel == 0) ? "2D" : "3D";
                rec.times[0]   = s_shTimes[0];
                rec.times[1]   = s_shTimes[1];
                rec.times[2]   = s_shTimes[2];
                rec.times[3]   = s_shTimes[3];

                bool ok = false;
                if (s_editShowMode) {
                    rec.id = s_editShowId;
                    ok = svc && svc->UpdateShow(rec);
                } else {
                    ok = svc && svc->AddShow(rec);
                }

                if (ok) {
                    s_adSuccess   = s_editShowMode ? "Show updated." : "Show added.";
                    s_adError.clear();
                    s_needRefresh = true;
                    formErr.clear();
                    ImGui::CloseCurrentPopup();
                } else {
                    formErr = svc ? svc->GetLastError() : "Service unavailable.";
                }
            }
        }
        ImGui::PopStyleColor(3);
        ImGui::PopFont();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

static void ShowDeleteConfirmPopup(ImFont** F, MovieService* svc, bool isMovie) {
    const char* popId = isMovie ? "Delete Movie?##pop" : "Delete Show?##pop";
    ImGui::SetNextWindowSize({ 380.0f, 0.0f });
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, { 0.5f,0.5f });
    ImGui::PushStyleColor(ImGuiCol_PopupBg,          ImVec4(24/255.f,18/255.f,22/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0,0,0,0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  ImVec2(28.0f,28.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,    ImVec2(0.0f,14.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.0f);

    if (ImGui::BeginPopupModal(popId, nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {

        ImDrawList* pdl = ImGui::GetWindowDrawList();
        ImVec2 wpos = ImGui::GetWindowPos();
        ImVec2 wsz  = ImGui::GetWindowSize();
        pdl->AddRect(wpos, { wpos.x+wsz.x, wpos.y+wsz.y },
                     IM_COL32(180,40,60,255), 16.0f, 0, 1.5f);

        ImGui::PushFont(F[3]);
        ImGui::TextColored({220/255.f,60/255.f,80/255.f,1.f},
                           isMovie ? "Delete Movie?" : "Delete Show?");
        ImGui::PopFont();
        ImGui::Dummy({0,4});
        ImGui::PushFont(F[1]);
        ImGui::TextColored({144/255.f,136/255.f,144/255.f,1.f},
            isMovie
            ? "This will also remove all shows for this movie."
            : "This show will be permanently removed.");
        ImGui::PopFont();
        ImGui::Dummy({0,8});

        float bw = 150.0f;
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(44/255.f,44/255.f,54/255.f,1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(60/255.f,60/255.f,72/255.f,1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(50/255.f,50/255.f,62/255.f,1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f,10.0f));
        ImGui::PushFont(F[1]);
        if (ImGui::Button("Cancel##dc", { bw, 40.0f })) {
            s_deleteTargetId = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine(0, 12.0f);
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(180/255.f,40/255.f,60/255.f,1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(210/255.f,55/255.f,75/255.f,1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(160/255.f,30/255.f,50/255.f,1.f));
        if (ImGui::Button("Delete##dc_ok", { bw, 40.0f }) && svc) {
            bool ok = isMovie ? svc->DeleteMovie(s_deleteTargetId)
                              : svc->DeleteShow (s_deleteTargetId);
            if (ok) {
                s_adSuccess   = isMovie ? "Movie deleted." : "Show deleted.";
                s_adError.clear();
                s_needRefresh = true;
            } else {
                s_adError   = svc->GetLastError();
                s_adSuccess.clear();
            }
            s_deleteTargetId = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(3);
        ImGui::PopFont();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

// ── Main render ───────────────────────────────────────────────────────────────
void RenderAdmin(bool& goHome, bool& goTickets, bool& goSchedule,
                 MovieService* movieSvc)
{
    ImGuiIO&    io   = ImGui::GetIO();
    ImFont**    F    = io.Fonts->Fonts.Data;
    float       winW = io.DisplaySize.x;
    float       winH = io.DisplaySize.y;

    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(21/255.f,21/255.f,21/255.f,1.f));
    ImGui::Begin("AdminPage", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove   |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2      orig = ImGui::GetWindowPos();
    ImDrawList* dl   = ImGui::GetWindowDrawList();

    // ── Navbar ────────────────────────────────────────────────────────────────
    constexpr float NAV_H = 64.0f;
    dl->AddRectFilled(orig, { orig.x+winW, orig.y+NAV_H }, AD_BG);
    dl->AddLine({ orig.x, orig.y+NAV_H }, { orig.x+winW, orig.y+NAV_H }, AD_BORDER, 1.0f);

    ImGui::PushFont(F[3]);
    float logoY = orig.y + (NAV_H - F[3]->LegacySize) * 0.5f;
    dl->AddText({ orig.x+36.0f, logoY }, AD_LOGO, "CINEMA");
    float logoW = ImGui::CalcTextSize("CINEMA").x;
    ImGui::PopFont();

    ImGui::PushFont(F[0]);
    const char* navItems[]  = { "Home", "Your Tickets", "Schedule", "Admin Panel" };
    bool        navActive[] = { false,  false,           false,      true          };
    float nx = orig.x + 36.0f + logoW + 32.0f;
    for (int i = 0; i < 4; i++) {
        ImVec2 tsz = ImGui::CalcTextSize(navItems[i]);
        float  ty  = orig.y + (NAV_H - tsz.y) * 0.5f;
        dl->AddText({ nx, ty }, navActive[i] ? AD_WHITE : AD_MUTED, navItems[i]);
        if (navActive[i])
            dl->AddLine({ nx, ty+tsz.y+3.0f }, { nx+tsz.x, ty+tsz.y+3.0f }, AD_TITLE, 2.0f);
        ImGui::SetCursorScreenPos({ nx-4.0f, orig.y+8.0f });
        ImGui::InvisibleButton(("ad_nav_"+std::string(navItems[i])).c_str(),
                               { tsz.x+8.0f, NAV_H-16.0f });
        if (ImGui::IsItemClicked()) {
            if (i == 0) goHome     = true;
            if (i == 1) goTickets  = true;
            if (i == 2) goSchedule = true;
        }
        nx += tsz.x + 42.0f;
    }
    ImGui::PopFont();

    constexpr float PROF_R = 18.0f;
    ImVec2 profC = { orig.x+winW-28.0f-PROF_R, orig.y+NAV_H*0.5f };
    ImGui::SetCursorScreenPos({ profC.x-PROF_R, profC.y-PROF_R });
    ImGui::InvisibleButton("ad_profile", { PROF_R*2, PROF_R*2 });
    ADProfile(dl, profC, PROF_R);

    constexpr float CITY_W = 90.0f, CITY_H = 36.0f;
    float cityLeft = profC.x - PROF_R - 16.0f - CITY_W;
    float cityTop  = orig.y + (NAV_H - CITY_H) * 0.5f;
    ImGui::SetCursorScreenPos({ cityLeft, cityTop });
    ImGui::InvisibleButton("ad_city", { CITY_W, CITY_H });
    bool adCityHov = ImGui::IsItemHovered();
    RenderCitySelector(F, { cityLeft, cityTop }, CITY_W, CITY_H, ImGui::IsItemClicked());
    ImGui::PushFont(F[1]);
    dl->AddText({ cityLeft+20.0f, cityTop+(CITY_H-F[1]->LegacySize)*0.5f },
                adCityHov ? AD_WHITE : AD_MUTED, g_selectedCity.c_str());
    ImGui::PopFont();
    ADPin(dl, { cityLeft+11.0f, cityTop+CITY_H*0.5f }, 7.0f, AD_ACCENT);

    // ── Content ───────────────────────────────────────────────────────────────
    constexpr float SIDE_PAD = 40.0f;
    float cx = orig.x + SIDE_PAD;
    float cw = winW - SIDE_PAD * 2.0f;
    float cy = orig.y + NAV_H + 30.0f;

    ImGui::PushFont(F[3]);
    dl->AddText({ cx, cy }, AD_WHITE, "Admin Panel");
    cy += F[3]->LegacySize + 24.0f;
    ImGui::PopFont();

    // ── Tabs ─────────────────────────────────────────────────────────────────
    const char* tabs[] = { "Movies", "Shows" };
    constexpr float TAB_W = 120.0f, TAB_H = 40.0f;
    for (int i = 0; i < 2; i++) {
        float tx = cx + i * (TAB_W + 8.0f);
        bool sel = (s_adTab == i);
        ImVec2 tTL = { tx, cy }, tBR = { tx+TAB_W, cy+TAB_H };
        ImGui::SetCursorScreenPos(tTL);
        char tid[16]; snprintf(tid, sizeof(tid), "ad_tab_%d", i);
        ImGui::InvisibleButton(tid, { TAB_W, TAB_H });
        if (ImGui::IsItemClicked()) {
            s_adTab = i;
            s_adError.clear(); s_adSuccess.clear();
        }
        bool thov = ImGui::IsItemHovered();
        dl->AddRectFilled(tTL, tBR,
            sel  ? IM_COL32(60,20,36,255) :
            thov ? IM_COL32(35,35,44,255) : AD_CARD, 8.0f);
        dl->AddRect(tTL, tBR, sel ? AD_TITLE : AD_BORDER, 8.0f, 0, sel ? 2.0f : 1.0f);
        ImGui::PushFont(F[1]);
        ImVec2 tsz = ImGui::CalcTextSize(tabs[i]);
        dl->AddText({ tx+(TAB_W-tsz.x)*0.5f, cy+(TAB_H-tsz.y)*0.5f },
                    sel ? AD_WHITE : AD_MUTED, tabs[i]);
        ImGui::PopFont();
    }

    // Add button aligned right
    constexpr float ADD_W = 140.0f;
    float addX = cx + cw - ADD_W;
    const char* addLabel = (s_adTab == 0) ? "+ Add Movie" : "+ Add Show";
    if (ADButton(dl, F, addLabel, "ad_add", addX, cy, ADD_W, TAB_H,
                 AD_TITLE, IM_COL32(245,85,130,255))) {
        if (s_adTab == 0) {
            s_mvTitle[0] = '\0'; s_mvGenre[0] = '\0'; s_mvYear = 2024;
            s_openMoviePopup = true;
        } else {
            s_shHall[0] = '\0'; s_shFormatSel = 0; s_shMovieSel = 0; s_editShowMode = false;
            for (int k = 0; k < 4; k++) s_shTimes[k][0] = '\0';
            s_openShowPopup = true;
        }
        s_adError.clear(); s_adSuccess.clear();
    }
    cy += TAB_H + 12.0f;

    // Status messages
    if (!s_adError.empty()) {
        ImGui::PushFont(F[1]);
        dl->AddText({ cx, cy }, AD_RED, s_adError.c_str());
        ImGui::PopFont();
        cy += 22.0f + 6.0f;
    } else if (!s_adSuccess.empty()) {
        ImGui::PushFont(F[1]);
        dl->AddText({ cx, cy }, AD_GREEN, s_adSuccess.c_str());
        ImGui::PopFont();
        cy += 22.0f + 6.0f;
    }

    // Refresh data if needed
    if (s_needRefresh && movieSvc) {
        movieSvc->GetAllMovies(s_movies);
        movieSvc->GetAllShows(s_shows);
        s_needRefresh = false;
    }

    // ── Table ─────────────────────────────────────────────────────────────────
    constexpr float ROW_H   = 46.0f;
    constexpr float HEAD_H  = 38.0f;
    float tableTop = cy;
    float tableH   = orig.y + winH - tableTop - 16.0f;

    // Draw table header
    dl->AddRectFilled({ cx, tableTop }, { cx+cw, tableTop+HEAD_H },
                      IM_COL32(32,22,30,255));
    dl->AddLine({ cx, tableTop+HEAD_H }, { cx+cw, tableTop+HEAD_H }, AD_BORDER, 1.0f);

    ImGui::PushFont(F[1]);
    if (s_adTab == 0) {
        // Movies header
        float colX[] = { cx+10.0f, cx+320.0f, cx+510.0f, cx+620.0f };
        const char* cols[] = { "Title", "Genre", "Year", "Actions" };
        for (int c = 0; c < 4; c++)
            dl->AddText({ colX[c], tableTop+(HEAD_H-F[1]->LegacySize)*0.5f }, AD_ACCENT, cols[c]);
    } else {
        // Shows header
        float colX[] = { cx+10.0f, cx+250.0f, cx+330.0f, cx+420.0f, cx+790.0f };
        const char* cols[] = { "Movie", "Hall", "Format", "Times (up to 4)", "Actions" };
        for (int c = 0; c < 5; c++)
            dl->AddText({ colX[c], tableTop+(HEAD_H-F[1]->LegacySize)*0.5f }, AD_ACCENT, cols[c]);
    }
    ImGui::PopFont();

    // Scrollable rows via child window
    float rowsTop = tableTop + HEAD_H;
    ImGui::SetNextWindowPos({ cx, rowsTop });
    ImGui::SetNextWindowSize({ cw, tableH - HEAD_H });
    ImGui::PushStyleColor(ImGuiCol_ChildBg,    ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0,0,0,0));
    ImGui::BeginChild("ad_rows", { cw, tableH - HEAD_H }, false,
                      ImGuiWindowFlags_NoScrollbar);

    ImDrawList* rdl  = ImGui::GetWindowDrawList();
    ImVec2      rpos = ImGui::GetWindowPos();

    if (s_adTab == 0) {
        // ── Movies rows ───────────────────────────────────────────────────────
        for (int row = 0; row < (int)s_movies.size(); row++) {
            const auto& m  = s_movies[row];
            float ry = rpos.y + row * ROW_H;
            ImU32 bg = (row & 1) ? AD_CARD2 : IM_COL32(26,26,32,255);
            rdl->AddRectFilled({ rpos.x, ry }, { rpos.x+cw, ry+ROW_H }, bg);
            rdl->AddLine({ rpos.x, ry+ROW_H }, { rpos.x+cw, ry+ROW_H },
                         IM_COL32(40,40,48,255), 1.0f);

            float textY = ry + (ROW_H - F[1]->LegacySize) * 0.5f;
            ImGui::PushFont(F[1]);
            // Title (clipped)
            rdl->PushClipRect({ rpos.x+10, ry }, { rpos.x+305.0f, ry+ROW_H }, true);
            rdl->AddText({ rpos.x+10.0f, textY }, AD_WHITE, m.title.c_str());
            rdl->PopClipRect();
            // Genre
            rdl->PushClipRect({ rpos.x+320, ry }, { rpos.x+500.0f, ry+ROW_H }, true);
            rdl->AddText({ rpos.x+320.0f, textY }, AD_MUTED, m.genre.c_str());
            rdl->PopClipRect();
            // Year
            char yrBuf[8]; snprintf(yrBuf, sizeof(yrBuf), "%d", m.year);
            rdl->AddText({ rpos.x+510.0f, textY }, AD_MUTED, yrBuf);
            ImGui::PopFont();

            // Delete button
            float btnX = rpos.x + 620.0f;
            float btnY = ry + (ROW_H - 28.0f) * 0.5f;
            ImGui::SetCursorScreenPos({ btnX, btnY });
            char did[32]; snprintf(did, sizeof(did), "ad_del_m%d", row);
            ImGui::InvisibleButton(did, { 80.0f, 28.0f });
            bool dhov = ImGui::IsItemHovered();
            if (ImGui::IsItemClicked()) {
                s_deleteTargetId = m.id;
                s_openDelMPopup  = true;
                s_adError.clear(); s_adSuccess.clear();
            }
            rdl->AddRectFilled({ btnX, btnY }, { btnX+80.0f, btnY+28.0f },
                dhov ? IM_COL32(200,40,60,255) : IM_COL32(150,30,45,255), 6.0f);
            ImGui::PushFont(F[1]);
            ImVec2 dtSz = ImGui::CalcTextSize("Delete");
            rdl->AddText({ btnX+(80.0f-dtSz.x)*0.5f, btnY+(28.0f-dtSz.y)*0.5f },
                         AD_WHITE, "Delete");
            ImGui::PopFont();
        }

        if (s_movies.empty()) {
            ImGui::PushFont(F[1]);
            rdl->AddText({ rpos.x + cw*0.5f - 80.0f, rpos.y + 24.0f },
                         AD_MUTED, "No movies yet. Click '+ Add Movie' to add one.");
            ImGui::PopFont();
        }

    } else {
        // ── Shows rows ────────────────────────────────────────────────────────
        for (int row = 0; row < (int)s_shows.size(); row++) {
            const auto& s  = s_shows[row];
            float ry = rpos.y + row * ROW_H;
            ImU32 bg = (row & 1) ? AD_CARD2 : IM_COL32(26,26,32,255);
            rdl->AddRectFilled({ rpos.x, ry }, { rpos.x+cw, ry+ROW_H }, bg);
            rdl->AddLine({ rpos.x, ry+ROW_H }, { rpos.x+cw, ry+ROW_H },
                         IM_COL32(40,40,48,255), 1.0f);

            float textY = ry + (ROW_H - F[1]->LegacySize) * 0.5f;
            ImGui::PushFont(F[1]);
            // Movie title
            rdl->PushClipRect({ rpos.x+10, ry }, { rpos.x+240.0f, ry+ROW_H }, true);
            rdl->AddText({ rpos.x+10.0f, textY }, AD_WHITE, s.movieTitle.c_str());
            rdl->PopClipRect();
            // Hall
            rdl->AddText({ rpos.x+250.0f, textY }, AD_MUTED, s.hall.c_str());
            // Format
            rdl->AddText({ rpos.x+330.0f, textY }, AD_MUTED, s.format.c_str());
            // Times
            char timeBuf[80] = "";
            for (int t = 0; t < 4; t++) {
                if (!s.times[t].empty()) {
                    if (timeBuf[0]) strncat_s(timeBuf, sizeof(timeBuf), "  ", _TRUNCATE);
                    strncat_s(timeBuf, sizeof(timeBuf), s.times[t].c_str(), _TRUNCATE);
                }
            }
            rdl->PushClipRect({ rpos.x+420, ry }, { rpos.x+780.0f, ry+ROW_H }, true);
            rdl->AddText({ rpos.x+420.0f, textY }, AD_MUTED, timeBuf);
            rdl->PopClipRect();
            ImGui::PopFont();

            // Edit button
            float editX = rpos.x + 790.0f;
            float btnY  = ry + (ROW_H - 28.0f) * 0.5f;
            ImGui::SetCursorScreenPos({ editX, btnY });
            char eid[32]; snprintf(eid, sizeof(eid), "ad_edit_s%d", row);
            ImGui::InvisibleButton(eid, { 68.0f, 28.0f });
            bool ehov = ImGui::IsItemHovered();
            if (ImGui::IsItemClicked()) {
                s_editShowMode = true;
                s_editShowId   = s.id;
                // Pre-fill form
                s_shMovieSel = 0;
                for (int mi = 0; mi < (int)s_movies.size(); mi++)
                    if (s_movies[mi].id == s.movieId) { s_shMovieSel = mi; break; }
                strncpy_s(s_shHall, sizeof(s_shHall), s.hall.c_str(), _TRUNCATE);
                s_shFormatSel = (s.format == "3D") ? 1 : 0;
                for (int t = 0; t < 4; t++)
                    strncpy_s(s_shTimes[t], sizeof(s_shTimes[t]), s.times[t].c_str(), _TRUNCATE);
                s_openShowPopup = true;
                s_adError.clear(); s_adSuccess.clear();
            }
            rdl->AddRectFilled({ editX, btnY }, { editX+68.0f, btnY+28.0f },
                ehov ? IM_COL32(60,80,180,255) : IM_COL32(45,60,140,255), 6.0f);
            ImGui::PushFont(F[1]);
            ImVec2 etSz = ImGui::CalcTextSize("Edit");
            rdl->AddText({ editX+(68.0f-etSz.x)*0.5f, btnY+(28.0f-etSz.y)*0.5f },
                         AD_WHITE, "Edit");
            ImGui::PopFont();

            // Delete button
            float delX = editX + 76.0f;
            ImGui::SetCursorScreenPos({ delX, btnY });
            char did[32]; snprintf(did, sizeof(did), "ad_del_s%d", row);
            ImGui::InvisibleButton(did, { 80.0f, 28.0f });
            bool dhov = ImGui::IsItemHovered();
            if (ImGui::IsItemClicked()) {
                s_deleteTargetId = s.id;
                s_openDelSPopup  = true;
                s_adError.clear(); s_adSuccess.clear();
            }
            rdl->AddRectFilled({ delX, btnY }, { delX+80.0f, btnY+28.0f },
                dhov ? IM_COL32(200,40,60,255) : IM_COL32(150,30,45,255), 6.0f);
            ImGui::PushFont(F[1]);
            ImVec2 dtSz = ImGui::CalcTextSize("Delete");
            rdl->AddText({ delX+(80.0f-dtSz.x)*0.5f, btnY+(28.0f-dtSz.y)*0.5f },
                         AD_WHITE, "Delete");
            ImGui::PopFont();
        }

        if (s_shows.empty()) {
            ImGui::PushFont(F[1]);
            rdl->AddText({ rpos.x + cw*0.5f - 80.0f, rpos.y + 24.0f },
                         AD_MUTED, "No shows yet. Add movies first, then add shows.");
            ImGui::PopFont();
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor(2);

    // ── Trigger popups (must be called outside BeginChild) ────────────────────
    if (s_openMoviePopup) { ImGui::OpenPopup("Add Movie##pop"); s_openMoviePopup = false; }
    if (s_openShowPopup)  {
        ImGui::OpenPopup(s_editShowMode ? "Edit Show##pop" : "Add Show##pop");
        s_openShowPopup = false;
    }
    if (s_openDelMPopup)  { ImGui::OpenPopup("Delete Movie?##pop"); s_openDelMPopup = false; }
    if (s_openDelSPopup)  { ImGui::OpenPopup("Delete Show?##pop");  s_openDelSPopup = false; }

    ShowMovieFormPopup  (F, movieSvc);
    ShowShowFormPopup   (F, movieSvc);
    ShowDeleteConfirmPopup(F, movieSvc, true);
    ShowDeleteConfirmPopup(F, movieSvc, false);

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::End();
}

// Called from main.cpp when entering the admin panel to force a data refresh
void ResetAdminState() {
    s_needRefresh = true;
    s_adError.clear();
    s_adSuccess.clear();
}
