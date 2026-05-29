#include "imgui.h"
#include <d3d11.h>

void RenderMainMenu(ID3D11ShaderResourceView** heroBanners, int* heroWidths, int* heroHeights, int& currentHeroIndex, ID3D11ShaderResourceView** posters, int* posterWidths, int* posterHeights, int& selectedMovieIndex) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(40, 40));

    // Remove NoDecoration so scrollbar is allowed! Include NoTitleBar, NoResize, NoCollapse Instead.
    ImGui::Begin("Modern Streaming Layout", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings);

    // Coordinate space tracking 
    ImVec2 winPos = ImGui::GetWindowPos();
    float scrollY = ImGui::GetScrollY();
    ImVec2 layoutStartPos = ImVec2(winPos.x, winPos.y - scrollY);
    
    // Shift the poster image significantly higher by subtracting from the top offset Y starting point
    float heroOffsetY = -300.0f; // Shifted even higher!
    float paddingTop = 100.0f; // Padding from top of screen!
    ImVec2 heroPos = ImVec2(layoutStartPos.x, layoutStartPos.y + paddingTop + heroOffsetY);
    
    float winWidth = ImGui::GetWindowWidth();
    float heroH = 750.0f; // Made the hero banner height dramatically higher!
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    // Select current banner
    ID3D11ShaderResourceView* heroBannerSRV = heroBanners ? heroBanners[currentHeroIndex] : nullptr;
    int heroWidth = heroWidths ? heroWidths[currentHeroIndex] : 1920;
    int heroHeight = heroHeights ? heroHeights[currentHeroIndex] : 1080;

    // Draw only within the strictly visible container!
    float visibleH = (heroPos.y + heroH) - (layoutStartPos.y + paddingTop); 
    ImVec2 visiblePos = ImVec2(layoutStartPos.x, layoutStartPos.y + paddingTop);
    
    if (heroBannerSRV) {
        // UV cropping to simulate object-fit: cover (fills totally, NO black spots!)
        float imgAspect = (float)heroWidth / (float)heroHeight;
        float rectAspect = winWidth / visibleH;
        ImVec2 uv0 = ImVec2(0, 0);
        ImVec2 uv1 = ImVec2(1, 1);
        
        if (rectAspect > imgAspect) {
            // Window is wider than image proportion: crop top/bottom
            float cropAmount = (1.0f - (imgAspect / rectAspect)) * 0.5f;
            uv0.y = cropAmount;
            uv1.y = 1.0f - cropAmount;
        } else {
            // Window is taller than image proportion: crop left/right
            float cropAmount = (1.0f - (rectAspect / imgAspect)) * 0.5f;
            uv0.x = cropAmount;
            uv1.x = 1.0f - cropAmount;
        }
        
        drawList->AddImage((ImTextureID)heroBannerSRV, visiblePos, ImVec2(visiblePos.x + winWidth, visiblePos.y + visibleH), uv0, uv1);
    } else {
        drawList->AddRectFilled(visiblePos, ImVec2(visiblePos.x + winWidth, visiblePos.y + visibleH), IM_COL32(0, 0, 0, 255));
    }
    
    // Gradient overlay (Black to Dark background color #0D0D0D to blend)
    drawList->AddRectFilledMultiColor(visiblePos, ImVec2(visiblePos.x + winWidth, visiblePos.y + visibleH), 
        IM_COL32(0,0,0,0), IM_COL32(0,0,0,0), IM_COL32(13,13,13,255), IM_COL32(13,13,13,255)); 

    // Mask the top to create a DOWNWARD smile curve!
    float curveDepth = 100.0f; 
    // Start mask in the top corners (no initial depth drop)
    drawList->PathLineTo(ImVec2(heroPos.x, heroPos.y)); 
    // Control point is pushed downward in the center to create a U shape
    drawList->PathBezierQuadraticCurveTo(ImVec2(heroPos.x + winWidth / 2.0f, heroPos.y + curveDepth * 2.0f), ImVec2(heroPos.x + winWidth, heroPos.y), 50);
    // Complete the masking block above the curve
    drawList->PathLineTo(ImVec2(heroPos.x + winWidth, layoutStartPos.y - 10.0f));
    drawList->PathLineTo(ImVec2(heroPos.x, layoutStartPos.y - 10.0f));
    drawList->PathFillConvex(IM_COL32(13, 13, 13, 255));

    // Mask the bottom to create a U-shaped smile (curving downwards)
    // We start masking from the bottom corners
    drawList->PathLineTo(ImVec2(heroPos.x, heroPos.y + heroH - curveDepth));
    // Control point is pushed downward in the center to create a U shape
    drawList->PathBezierQuadraticCurveTo(ImVec2(heroPos.x + winWidth / 2.0f, heroPos.y + heroH + curveDepth), ImVec2(heroPos.x + winWidth, heroPos.y + heroH - curveDepth), 50);
    // Complete the masking block underneath the curve
    drawList->PathLineTo(ImVec2(heroPos.x + winWidth, heroPos.y + heroH + curveDepth + 10.0f));
    drawList->PathLineTo(ImVec2(heroPos.x, heroPos.y + heroH + curveDepth + 10.0f));
    drawList->PathFillConvex(IM_COL32(13, 13, 13, 255));

    // Header Text over the Banner
    ImGui::SetCursorScreenPos(ImVec2(layoutStartPos.x + 40, layoutStartPos.y + paddingTop + 20));
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("CINEMA");
    ImGui::SetWindowFontScale(1.0f);
    
    ImGui::SameLine(ImGui::GetWindowWidth() - 200);
    ImGui::SetCursorPosY(layoutStartPos.y + paddingTop + 25);
    ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.58f, 1.0f), "Search");
    ImGui::SameLine();
    ImGui::Text("Profile");
    
    // Hero Text strings dynamically matched to banner index
    const char* heroTitles[3] = { "DUNE: PART TWO", "INTERSTELLAR", "BLADE RUNNER 2049" };
    const char* heroMetas[3] = { "2024 \x95 Sci-Fi \x95 2h 46m", "2014 \x95 Sci-Fi \x95 2h 49m", "2017 \x95 Sci-Fi \x95 2h 44m" };

    // Add Hero Text (Centered vertically within the remaining visible lens shape)
    ImVec2 heroTextPos = ImVec2(heroPos.x + 40, heroPos.y + heroH / 2.0f + 60.0f);
    ImGui::SetWindowFontScale(2.5f);
    drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), heroTextPos, IM_COL32(255, 255, 255, 255), heroTitles[currentHeroIndex % 3], nullptr, 0.0f);
    ImGui::SetWindowFontScale(1.0f);
    drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 1.2f, ImVec2(heroTextPos.x, heroTextPos.y + 45), IM_COL32(142, 142, 147, 255), heroMetas[currentHeroIndex % 3], nullptr, 0.0f);
    
    // Left Arrow
    ImVec2 leftArrowCenter = ImVec2(visiblePos.x + 35.0f + 25.0f, visiblePos.y + visibleH / 2.0f); // 35px padding + radius
    drawList->AddCircleFilled(leftArrowCenter, 25.0f, IM_COL32(204, 255, 0, 255)); // Green circle
    // Arrow lines
    drawList->AddLine(ImVec2(leftArrowCenter.x + 5, leftArrowCenter.y - 10), ImVec2(leftArrowCenter.x - 5, leftArrowCenter.y), IM_COL32(0, 0, 0, 255), 3.0f);
    drawList->AddLine(ImVec2(leftArrowCenter.x - 5, leftArrowCenter.y), ImVec2(leftArrowCenter.x + 5, leftArrowCenter.y + 10), IM_COL32(0, 0, 0, 255), 3.0f);
    
    ImGui::SetCursorScreenPos(ImVec2(leftArrowCenter.x - 25.0f, leftArrowCenter.y - 25.0f));
    if (ImGui::InvisibleButton("left_banner", ImVec2(50, 50))) {
        currentHeroIndex = (currentHeroIndex - 1 + 3) % 3;
    }

    // Right Arrow
    ImVec2 rightArrowCenter = ImVec2(visiblePos.x + winWidth - 35.0f - 25.0f, visiblePos.y + visibleH / 2.0f); // 35px padding + radius
    drawList->AddCircleFilled(rightArrowCenter, 25.0f, IM_COL32(204, 255, 0, 255)); // Green circle
    // Arrow lines
    drawList->AddLine(ImVec2(rightArrowCenter.x - 5, rightArrowCenter.y - 10), ImVec2(rightArrowCenter.x + 5, rightArrowCenter.y), IM_COL32(0, 0, 0, 255), 3.0f);
    drawList->AddLine(ImVec2(rightArrowCenter.x + 5, rightArrowCenter.y), ImVec2(rightArrowCenter.x - 5, rightArrowCenter.y + 10), IM_COL32(0, 0, 0, 255), 3.0f);

    ImGui::SetCursorScreenPos(ImVec2(rightArrowCenter.x - 25.0f, rightArrowCenter.y - 25.0f));
    if (ImGui::InvisibleButton("right_banner", ImVec2(50, 50))) {
        currentHeroIndex = (currentHeroIndex + 1) % 3;
    }

    // Hero button
    ImGui::SetCursorScreenPos(ImVec2(heroTextPos.x, heroTextPos.y + 80));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.80f, 1.00f, 0.00f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f)); 
    if (ImGui::Button("Book Ticket", ImVec2(150, 40))) {
        selectedMovieIndex = 0;
        // m_bookingService->BookMovie(...)
    }
    ImGui::PopStyleColor(2);

    // Push cursor below the entire drawing, using SetCursorScreenPos
    // We add Dummy element so ImGui registers the huge height and allows scrolling!
    ImGui::SetCursorScreenPos(ImVec2(layoutStartPos.x + 40, heroPos.y + heroH + 20));

    // Movies Grid
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("Trending Now");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Spacing();
    
    const int COLUMNS = 6;
    if (ImGui::BeginTable("movies_grid", COLUMNS, ImGuiTableFlags_SizingStretchSame)) {
        for (int i = 0; i < 6; i++) {
            ImGui::TableNextColumn();
            ImVec2 p = ImGui::GetCursorScreenPos();
            float w = ImGui::GetContentRegionAvail().x;
            float h = w * 1.5f; // Poster aspect ratio
            
            // Poster picture or background
            if (posters && posters[i]) {
                drawList->AddImage((ImTextureID)posters[i], p, ImVec2(p.x + w, p.y + h), ImVec2(0,0), ImVec2(1,1));
            } else {
                drawList->AddRectFilled(p, ImVec2(p.x + w, p.y + h), IM_COL32(0, 0, 0, 255), 8.0f);
            }
            
            if (i == 0) { // "Win" promo tag on first one
                drawList->AddRectFilled(ImVec2(p.x + 10, p.y + 10), ImVec2(p.x + 50, p.y + 35), IM_COL32(204, 255, 0, 255), 4.0f); // #CCFF00
                drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(p.x + 18, p.y + 16), IM_COL32(10, 10, 10, 255), "WIN");
            }
            
            // Add fake poster text centered if no poster
            const char* titles[6] = { "Dune: Part Two", "Interstellar", "Blade Runner 2049", "The Dark Knight", "Inception", "The Matrix" };
            const char* meta[6] = { "2024 \x95 Sci-Fi", "2014 \x95 Sci-Fi", "2017 \x95 Sci-Fi", "2008 \x95 Action", "2010 \x95 Sci-Fi", "1999 \x95 Sci-Fi" };
            
            if (!posters || !posters[i]) {
                ImVec2 textSize = ImGui::CalcTextSize(titles[i]);
                drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(p.x + w/2 - textSize.x/2, p.y + h/2 - textSize.y/2), IM_COL32(255, 255, 255, 200), titles[i]);
            }
            
            // Invisible button to make the area clickable, this also registers the height of the item in the layout for scrolling!
            ImGui::InvisibleButton(titles[i], ImVec2(w, h));
            if (ImGui::IsItemHovered()) {
                drawList->AddRect(p, ImVec2(p.x + w, p.y + h), IM_COL32(204, 255, 0, 255), 8.0f, 0, 2.0f); // Accent hover border
            }
            if (ImGui::IsItemClicked()) {
                selectedMovieIndex = i;
            }
            
            ImGui::Spacing();
            
            ImGui::SetWindowFontScale(1.2f);
            ImGui::Text("%s", titles[i]);
            ImGui::SetWindowFontScale(1.0f);
            ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.58f, 1.0f), "%s", meta[i]);
        }
        ImGui::EndTable();
    }
    
    // Add extra bottom spacing so we can scroll past the posters easily
    ImGui::Dummy(ImVec2(100, 100));

    ImGui::End();
    ImGui::PopStyleVar();
}









