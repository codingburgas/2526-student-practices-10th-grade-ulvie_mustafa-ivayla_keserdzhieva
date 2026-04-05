#include <windows.h>
#include <iostream>
#include <string>

#define WINDOW_WIDTH 1440
#define WINDOW_HEIGHT 943
#define BACKGROUND_COLOR RGB(21, 21, 21)  // #151515

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void DrawNavbar(HDC hdc, RECT* clientRect);

bool showGrid = true;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    try {
        std::cout << "Starting Cinema Ticket Booking System - Test Screen" << std::endl;
        std::cout << "Resolution: 1440x943, Background: #151515" << std::endl;

        // Register window class
        const char CLASS_NAME[] = "CinemaTestScreen";

        WNDCLASS wc = {};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;
        wc.hbrBackground = CreateSolidBrush(BACKGROUND_COLOR);
        wc.style = CS_HREDRAW | CS_VREDRAW;

        RegisterClass(&wc);

        // Create window
        HWND hwnd = CreateWindowEx(
            0,
            CLASS_NAME,
            "Test Screen - Cinema Booking System",
            WS_OVERLAPPEDWINDOW,  // Standard window with min/max/close buttons
            CW_USEDEFAULT, CW_USEDEFAULT,
            WINDOW_WIDTH, WINDOW_HEIGHT,
            NULL,
            NULL,
            hInstance,
            NULL
        );

        if (hwnd == NULL) {
            std::cerr << "Failed to create window" << std::endl;
            return 1;
        }

        ShowWindow(hwnd, nCmdShow);

        // Message loop
        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        std::cout << "Application closed successfully" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT clientRect;
            GetClientRect(hwnd, &clientRect);

            // Fill background
            HBRUSH bgBrush = CreateSolidBrush(BACKGROUND_COLOR);
            FillRect(hdc, &clientRect, bgBrush);
            DeleteObject(bgBrush);

            DrawNavbar(hdc, &clientRect);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
            }
            else if (wParam == 'G') {
                showGrid = !showGrid;
                InvalidateRect(hwnd, NULL, TRUE);
            }
            return 0;

        case WM_LBUTTONDOWN: {
            // Check if click is on close button area (simplified)
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);

            int centerX = WINDOW_WIDTH / 2;
            int centerY = WINDOW_HEIGHT / 2;

            // Close button area
            if (pt.x >= centerX + 70 && pt.x <= centerX + 150 &&
                pt.y >= centerY + 60 && pt.y <= centerY + 85) {
                PostQuitMessage(0);
            }
            return 0;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void DrawNavbar(HDC hdc, RECT* clientRect) {
    int startY = 54;

    SetBkMode(hdc, TRANSPARENT);

    // Create fonts
    HFONT navFontSelected = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                               CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                               DEFAULT_PITCH | FF_DONTCARE, "Inter");
    HFONT navFontNormal = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                               CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                               DEFAULT_PITCH | FF_DONTCARE, "Inter");

    // Colors
    COLORREF colorSelected = RGB(181, 181, 181); // #B5B5B5
    COLORREF colorNormal = RGB(109, 109, 109);   // #6D6D6D

    // Pre-calculate text sizes to enable relative dynamic spacing
    SIZE sizeHome, sizeTickets, sizeCity;
    HGDIOBJ oldFont = SelectObject(hdc, navFontSelected);
    GetTextExtentPoint32(hdc, "Home", 4, &sizeHome);
    SelectObject(hdc, navFontNormal);
    GetTextExtentPoint32(hdc, "Your Tickets", 12, &sizeTickets);
    GetTextExtentPoint32(hdc, "City", 4, &sizeCity);

    // Fixed sizing elements
    int logoWidth = 40;
    int searchWidth = 308;
    int cityIconWidth = 25;
    int profileWidth = 36;
    int sidePadding = 50;

    int totalFixedItemsWidth = sidePadding * 2 + logoWidth + sizeHome.cx + sizeTickets.cx + searchWidth + sizeCity.cx + cityIconWidth + profileWidth;

    // Available space to distribute to the gaps (preventing negative values on very small windows)
    int totalOriginalGaps = 42 + 47 + 71 + 497 + 82; // 739
    int availableGapSpace = clientRect->right - totalFixedItemsWidth;
    if (availableGapSpace < totalOriginalGaps) availableGapSpace = totalOriginalGaps;

    // Relative gaps distributing the screen width
    int gap1 = (int)(availableGapSpace * (42.0 / totalOriginalGaps));
    int gap2 = (int)(availableGapSpace * (47.0 / totalOriginalGaps));
    int gap3 = (int)(availableGapSpace * (71.0 / totalOriginalGaps));
    int gap4 = (int)(availableGapSpace * (497.0 / totalOriginalGaps));
    int gap5 = (int)(availableGapSpace * (82.0 / totalOriginalGaps));

    int currentX = sidePadding; // Starting padding from left

    // 1. Logo (Placeholder Box)
    HBRUSH logoBrush = CreateSolidBrush(colorSelected);
    RECT logoRect = { currentX, startY - 5, currentX + logoWidth, startY + 25 };
    FillRect(hdc, &logoRect, logoBrush);
    DeleteObject(logoBrush);
    currentX += logoWidth + gap1;

    // 2. Home (Selected)
    // Draw a subtle highlighted boundary to simulate a soft transparent glow
    HBRUSH glowBrush = CreateSolidBrush(RGB(35, 35, 35)); // Slightly lighter than background #151515
    HPEN glowPen = CreatePen(PS_NULL, 0, RGB(0,0,0));
    HGDIOBJ oldGlowBrush = SelectObject(hdc, glowBrush);
    HGDIOBJ oldGlowPen = SelectObject(hdc, glowPen);

    int paddingX = 16;
    int paddingY = 8;
    RoundRect(hdc, currentX - paddingX, startY - paddingY, currentX + sizeHome.cx + paddingX, startY + sizeHome.cy + paddingY, 20, 20);

    SelectObject(hdc, oldGlowBrush);
    SelectObject(hdc, oldGlowPen);
    DeleteObject(glowBrush);
    DeleteObject(glowPen);

    SelectObject(hdc, navFontSelected);
    SetTextColor(hdc, colorSelected);
    TextOut(hdc, currentX, startY, "Home", 4);
    currentX += sizeHome.cx + gap2;

    // 3. Your Tickets (Unselected)
    SelectObject(hdc, navFontNormal);
    SetTextColor(hdc, colorNormal);
    TextOut(hdc, currentX, startY, "Your Tickets", 12);
    currentX += sizeTickets.cx + gap3;

    // 4. Searchbar
    HBRUSH searchBrush = CreateSolidBrush(RGB(29, 29, 29)); // #1D1D1D
    HPEN nullPen = CreatePen(PS_NULL, 0, RGB(0,0,0));
    HGDIOBJ oldBrush = SelectObject(hdc, searchBrush);
    HGDIOBJ oldPen = SelectObject(hdc, nullPen);

    int searchHeight = 41;
    int searchY = startY - (searchHeight / 2) + 10;
    RoundRect(hdc, currentX, searchY, currentX + searchWidth, searchY + searchHeight, 19, 19);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(searchBrush);
    DeleteObject(nullPen);

    // Search placeholder text
    SetTextColor(hdc, colorNormal);
    TextOut(hdc, currentX + 20, startY - 2, "Search a movie", 14);

    // Fake search icon at end of searchbar
    HPEN iconPen = CreatePen(PS_SOLID, 2, colorNormal);
    oldPen = SelectObject(hdc, iconPen);
    int iconX = currentX + searchWidth - 30;
    int iconY = searchY + searchHeight / 2;
    Ellipse(hdc, iconX - 6, iconY - 6, iconX + 6, iconY + 6);
    MoveToEx(hdc, iconX + 4, iconY + 4, NULL);
    LineTo(hdc, iconX + 10, iconY + 10);
    SelectObject(hdc, oldPen);
    DeleteObject(iconPen);

    currentX += searchWidth + gap4;

    // 5. City
    TextOut(hdc, currentX, startY, "City", 4);

    // Fake location icon
    iconPen = CreatePen(PS_SOLID, 2, colorNormal);
    oldPen = SelectObject(hdc, iconPen);
    int locX = currentX + sizeCity.cx + 15;
    Ellipse(hdc, locX - 4, startY + 4, locX + 4, startY + 12);
    MoveToEx(hdc, locX, startY + 12, NULL);
    LineTo(hdc, locX, startY + 18);
    SelectObject(hdc, oldPen);
    DeleteObject(iconPen);

    currentX += sizeCity.cx + cityIconWidth + gap5;

    // 6. Profile Icon (Circle)
    HBRUSH profileBrush = CreateSolidBrush(colorNormal);
    oldBrush = SelectObject(hdc, profileBrush);
    oldPen = SelectObject(hdc, GetStockObject(NULL_PEN));
    Ellipse(hdc, currentX, startY - 8, currentX + profileWidth, startY + 28);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(profileBrush);

    // Cleanup fonts
    SelectObject(hdc, oldFont);
    DeleteObject(navFontSelected);
    DeleteObject(navFontNormal);
}
