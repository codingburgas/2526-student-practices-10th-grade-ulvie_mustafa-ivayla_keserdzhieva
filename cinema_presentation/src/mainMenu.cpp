#include "imgui.h"
#include <d3d11.h>

void RenderMainMenu(ID3D11ShaderResourceView** heroBanners, int* heroWidths, int* heroHeights, int& currentHeroIndex, ID3D11ShaderResourceView** posters, int* posterWidths, int* posterHeights, int& selectedMovieIndex) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, 1.0f)); // Dark background

    ImGui::Begin("Da512 Museum", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings);

    ImVec2 winPos = ImGui::GetWindowPos();
    float scrollY = ImGui::GetScrollY();
    ImVec2 layoutStartPos = ImVec2(winPos.x, winPos.y - scrollY);
    float winWidth = ImGui::GetWindowWidth();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // ============= TOP NAVIGATION BAR =============
    float navHeight = 60.0f;
    ImVec2 navBarPos = layoutStartPos;
    drawList->AddRectFilled(navBarPos, ImVec2(navBarPos.x + winWidth, navBarPos.y + navHeight), IM_COL32(13, 13, 13, 255));

    // Logo "Da512"
    ImGui::SetCursorScreenPos(ImVec2(navBarPos.x + 20, navBarPos.y + 15));
    ImGui::SetWindowFontScale(1.4f);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Da512");
    ImGui::SetWindowFontScale(1.0f);

    // Navigation Menu Items
    float menuX = navBarPos.x + 150;
    const char* menuItems[] = { "Explore", "Top Picks", "Giveaways", "Just Announced", "All Offers" };
    for (int i = 0; i < 5; i++) {
        ImGui::SetCursorScreenPos(ImVec2(menuX, navBarPos.y + 20));
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", menuItems[i]);
        menuX += 130;
    }

    // Search and Profile icons on the right
    ImGui::SetCursorScreenPos(ImVec2(navBarPos.x + winWidth - 150, navBarPos.y + 20));
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Search");
    ImGui::SameLine(navBarPos.x + winWidth - 80);
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Profile");

    // ============= HERO BANNER SECTION =============
    float heroStartY = navBarPos.y + navHeight;
    float heroH = 350.0f;
    ImVec2 heroBannerPos = ImVec2(navBarPos.x, heroStartY);

    // Hero image or background
    if (heroBanners && heroBanners[currentHeroIndex]) {
        drawList->AddImage((ImTextureID)heroBanners[currentHeroIndex], heroBannerPos, ImVec2(heroBannerPos.x + winWidth, heroBannerPos.y + heroH), ImVec2(0, 0), ImVec2(1, 1));
    } else {
        drawList->AddRectFilled(heroBannerPos, ImVec2(heroBannerPos.x + winWidth, heroBannerPos.y + heroH), IM_COL32(30, 30, 40, 255));
    }

    // Dark gradient overlay on hero image
    drawList->AddRectFilledMultiColor(heroBannerPos, ImVec2(heroBannerPos.x + winWidth, heroBannerPos.y + heroH), 
        IM_COL32(0, 0, 0, 80), IM_COL32(0, 0, 0, 80), IM_COL32(0, 0, 0, 150), IM_COL32(0, 0, 0, 150));

    // Hero text overlay
    ImVec2 heroTextPos = ImVec2(heroBannerPos.x + 40, heroBannerPos.y + 50);
    ImGui::SetCursorScreenPos(heroTextPos);
    ImGui::SetWindowFontScale(2.0f);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Treasured ten: selections from");
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "the costume collection");
    ImGui::SetWindowFontScale(1.0f);

    // Hero subtitle
    ImGui::SetCursorScreenPos(ImVec2(heroBannerPos.x + 40, heroBannerPos.y + 140));
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "TODAY | 9:30 AM");
    ImGui::SetCursorScreenPos(ImVec2(heroBannerPos.x + 40, heroBannerPos.y + 160));
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Chicago History Museum");

    // Play and Heart buttons
    ImVec2 playBtnPos = ImVec2(heroBannerPos.x + 40, heroBannerPos.y + 200);
    drawList->AddCircleFilled(playBtnPos, 25.0f, IM_COL32(255, 255, 255, 200));
    // Play triangle
    drawList->AddTriangleFilled(ImVec2(playBtnPos.x - 8, playBtnPos.y - 10), ImVec2(playBtnPos.x - 8, playBtnPos.y + 10), ImVec2(playBtnPos.x + 10, playBtnPos.y), IM_COL32(0, 0, 0, 255));

    ImVec2 heartBtnPos = ImVec2(heroBannerPos.x + 100, heroBannerPos.y + 200);
    drawList->AddCircle(heartBtnPos, 25.0f, IM_COL32(255, 255, 255, 200), 0, 2.0f);
    // Simple heart shape (approximation)
    drawList->AddText(ImGui::GetFont(), 30.0f, ImVec2(heartBtnPos.x - 7, heartBtnPos.y - 10), IM_COL32(255, 255, 255, 255), "\xE2\x9D\xA4"); // Heart symbol

    // ============= GIVEAWAYS SECTION =============
    ImGui::SetCursorScreenPos(ImVec2(heroBannerPos.x + 40, heroBannerPos.y + heroH + 40));
    ImGui::SetWindowFontScale(1.6f);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Giveaways");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SetCursorScreenPos(ImVec2(heroBannerPos.x + 40, heroBannerPos.y + heroH + 75));

    // Event cards data
    const char* eventTitles[] = { "Riot Fest 2022", "SIX", "THE KILLERS", "Moulin Roueel The musical", "North coast music festival", "Sacred rose" };
    const char* eventLabels[] = { "NEW", "NEW", "NEW", "NEW", "NEW", "NEW" };
    
    float cardWidth = 200.0f;
    float cardHeight = 280.0f;
    float cardSpacing = 20.0f;
    float startX = heroBannerPos.x + 40;
    float startY = heroBannerPos.y + heroH + 75;

    for (int i = 0; i < 6; i++) {
        float cardX = startX + (i * (cardWidth + cardSpacing));
        ImVec2 cardPos = ImVec2(cardX, startY);

        // Card background (image or placeholder)
        if (posters && posters[i]) {
            drawList->AddImage((ImTextureID)posters[i], cardPos, ImVec2(cardPos.x + cardWidth, cardPos.y + cardHeight), ImVec2(0, 0), ImVec2(1, 1));
        } else {
            // Placeholder with gradient
            drawList->AddRectFilled(cardPos, ImVec2(cardPos.x + cardWidth, cardPos.y + cardHeight), IM_COL32(40, 40, 50, 255), 8.0f);
        }

        // Dark overlay on card
        drawList->AddRectFilled(cardPos, ImVec2(cardPos.x + cardWidth, cardPos.y + cardHeight), IM_COL32(0, 0, 0, 100), 8.0f);

        // Event label badge (NEW/WIN)
        drawList->AddRectFilled(ImVec2(cardPos.x + 10, cardPos.y + 10), ImVec2(cardPos.x + 50, cardPos.y + 28), IM_COL32(204, 255, 0, 255), 3.0f);
        drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.8f, ImVec2(cardPos.x + 15, cardPos.y + 13), IM_COL32(0, 0, 0, 255), eventLabels[i]);

        // Event title at bottom of card
        ImVec2 titlePos = ImVec2(cardPos.x + 10, cardPos.y + cardHeight - 50);
        drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 1.1f, titlePos, IM_COL32(255, 255, 255, 255), eventTitles[i], nullptr, cardWidth - 20);

        // Invisible button for interaction
        ImGui::SetCursorScreenPos(cardPos);
        ImGui::InvisibleButton(eventTitles[i], ImVec2(cardWidth, cardHeight));
        if (ImGui::IsItemHovered()) {
            drawList->AddRect(cardPos, ImVec2(cardPos.x + cardWidth, cardPos.y + cardHeight), IM_COL32(204, 255, 0, 255), 8.0f, 0, 2.0f);
        }
    }

    // Add spacing at the bottom for scrolling
    ImGui::SetCursorScreenPos(ImVec2(heroBannerPos.x, startY + cardHeight + 100));
    ImGui::Dummy(ImVec2(100, 100));

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}









