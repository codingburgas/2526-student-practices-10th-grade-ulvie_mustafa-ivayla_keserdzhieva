#pragma once
#include "imgui.h"
#include <string>

inline std::string g_selectedCity     = "Burgas";
inline bool        g_cityDropdownOpen = false;

static const char* const k_Cities[] = { "Burgas", "Sofia", "Plovdiv", "Varna", "Ruse" };
static constexpr int     k_CityCount = 5;

// Call immediately after the city InvisibleButton.
// wasClicked = ImGui::IsItemClicked() captured right after that button.
inline void RenderCitySelector(ImFont** F, ImVec2 btnTL, float btnW, float btnH, bool wasClicked)
{
    if (wasClicked)
        g_cityDropdownOpen = !g_cityDropdownOpen;

    if (!g_cityDropdownOpen) return;

    ImDrawList* fdl  = ImGui::GetForegroundDrawList();
    ImGuiIO&    io   = ImGui::GetIO();

    constexpr float DROP_W = 170.0f;
    constexpr float ROW_H  = 40.0f;
    constexpr float PAD    = 4.0f;
    float dropH = k_CityCount * ROW_H + PAD * 2.0f;
    float dropX = btnTL.x;
    float dropY = btnTL.y + btnH + 6.0f;

    // Keep inside the right edge of the screen
    if (dropX + DROP_W > io.DisplaySize.x - 4.0f)
        dropX = io.DisplaySize.x - DROP_W - 4.0f;

    ImVec2 dTL = { dropX, dropY }, dBR = { dropX + DROP_W, dropY + dropH };

    // Drop shadow
    fdl->AddRectFilled({ dTL.x + 4, dTL.y + 4 }, { dBR.x + 4, dBR.y + 4 },
                       IM_COL32(0, 0, 0, 100), 10.0f);
    // Panel background & border
    fdl->AddRectFilled(dTL, dBR, IM_COL32(28, 28, 34, 255), 10.0f);
    fdl->AddRect      (dTL, dBR, IM_COL32(52, 52, 58, 255), 10.0f, 0, 1.5f);

    ImVec2 mouse  = io.MousePos;
    bool   lclick = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

    // Close when clicking outside (but not on the toggle button itself — wasClicked handles that)
    bool inDrop = mouse.x >= dTL.x && mouse.x <= dBR.x &&
                  mouse.y >= dTL.y && mouse.y <= dBR.y;
    if (lclick && !inDrop && !wasClicked) {
        g_cityDropdownOpen = false;
        return;
    }

    float ry = dropY + PAD;
    for (int i = 0; i < k_CityCount; i++) {
        bool sel = (g_selectedCity == k_Cities[i]);
        bool hov = mouse.x >= dropX + PAD && mouse.x <= dropX + DROP_W - PAD &&
                   mouse.y >= ry           && mouse.y <= ry + ROW_H;

        ImVec2 rTL = { dropX + PAD, ry }, rBR = { dropX + DROP_W - PAD, ry + ROW_H };
        if (sel)      fdl->AddRectFilled(rTL, rBR, IM_COL32( 60, 20, 36, 255), 8.0f);
        else if (hov) fdl->AddRectFilled(rTL, rBR, IM_COL32( 40, 40, 50, 255), 8.0f);

        if (lclick && hov) {
            g_selectedCity     = k_Cities[i];
            g_cityDropdownOpen = false;
        }

        fdl->AddText(F[1], F[1]->LegacySize,
                     { dropX + 16.0f, ry + (ROW_H - F[1]->LegacySize) * 0.5f },
                     sel ? IM_COL32(236,  64, 113, 255) :
                     hov ? IM_COL32(255, 255, 255, 255) :
                           IM_COL32(144, 136, 144, 255),
                     k_Cities[i]);

        // Pink dot on the right for the selected row
        if (sel)
            fdl->AddCircleFilled({ dropX + DROP_W - 16.0f, ry + ROW_H * 0.5f },
                                 4.0f, IM_COL32(236, 64, 113, 255), 12);

        ry += ROW_H;
    }
}
