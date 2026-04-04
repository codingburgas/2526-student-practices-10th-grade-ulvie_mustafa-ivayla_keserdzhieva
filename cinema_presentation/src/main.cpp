#include <windows.h>
#include <iostream>
#include <string>

#define WINDOW_WIDTH 1440
#define WINDOW_HEIGHT 943
#define BACKGROUND_COLOR RGB(21, 21, 21)  // #151515

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void DrawGrid(HDC hdc, RECT* clientRect);
void DrawTextLabels(HDC hdc, RECT* clientRect);

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

void DrawGrid(HDC hdc, RECT* clientRect) {
    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
    SelectObject(hdc, gridPen);

    // Draw vertical lines
    for (int x = 0; x < clientRect->right; x += 100) {
        MoveToEx(hdc, x, 0, NULL);
        LineTo(hdc, x, clientRect->bottom);
    }

    // Draw horizontal lines
    for (int y = 0; y < clientRect->bottom; y += 100) {
        MoveToEx(hdc, 0, y, NULL);
        LineTo(hdc, clientRect->right, y);
    }

    DeleteObject(gridPen);
}

void DrawTextLabels(HDC hdc, RECT* clientRect) {
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(85, 85, 85));

    HFONT labelFont = CreateFont(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                               CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                               DEFAULT_PITCH | FF_DONTCARE, "Courier New");
    SelectObject(hdc, labelFont);

    // Corner labels
    TextOut(hdc, 20, 20, "1440×943", 8);
    TextOut(hdc, WINDOW_WIDTH - 60, 20, "W:1440", 6);
    TextOut(hdc, 20, WINDOW_HEIGHT - 30, "H:943", 5);
    TextOut(hdc, WINDOW_WIDTH - 50, WINDOW_HEIGHT - 30, "943px", 5);

    DeleteObject(labelFont);
}
