#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Forward declare native message handler from ImGui
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Forward declare Main Menu rendering function
void RenderMainMenu(ID3D11ShaderResourceView* heroBannerSRV, int heroWidth, int heroHeight, ID3D11ShaderResourceView** posters, int* posterWidths, int* posterHeights, int& selectedMovieIndex);

// Global Direct3D Device Pointers
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Texture loading utility using Windows URLMon & stb_image
bool LoadTextureFromURL(ID3D11Device* pd3dDevice, const char* url, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height) {
    if (!pd3dDevice || !url) return false;
    
    // Download image from API
    const char* temp_file = "temp_poster_cache.jpg";
    if (URLDownloadToFileA(nullptr, url, temp_file, 0, nullptr) != S_OK) return false;

    // Load with stb_image
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(temp_file, &image_width, &image_height, NULL, 4);
    if (image_data == NULL) return false;

    // Create DirectX 11 Texture
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D* pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

    // Create Texture View
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
    pTexture->Release();

    *out_width = image_width;
    *out_height = image_height;
    stbi_image_free(image_data);
    return true;
}

// System Setup Declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main(int, char**) {
    // 1. Register Win32 Window Class
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"CinemaSystem", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Cinema Ticketing Terminal", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    // 2. Initialize Direct3D Backend
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // 3. Initialize Dear ImGui Context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Apply Modern Dark Theme
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    ImVec4 bg = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);        // #0D0D0D
    ImVec4 surface = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);   // #1E1E1E
    ImVec4 interact = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    ImVec4 accent = ImVec4(0.80f, 1.00f, 0.00f, 1.00f);    // #CCFF00
    ImVec4 accent_hover = ImVec4(0.85f, 1.00f, 0.20f, 1.00f);
    ImVec4 accent_active = ImVec4(0.70f, 0.90f, 0.00f, 1.00f);
    ImVec4 text_primary = ImVec4(1.00f, 1.00f, 1.00f, 1.00f); // #FFFFFF
    ImVec4 text_secondary = ImVec4(0.55f, 0.55f, 0.58f, 1.00f); // #8E8E93

    colors[ImGuiCol_WindowBg] = bg;
    colors[ImGuiCol_ChildBg] = bg;
    colors[ImGuiCol_PopupBg] = surface;
    colors[ImGuiCol_Text] = text_primary;
    colors[ImGuiCol_TextDisabled] = text_secondary;
    colors[ImGuiCol_FrameBg] = surface;
    colors[ImGuiCol_FrameBgHovered] = interact;
    colors[ImGuiCol_FrameBgActive] = interact;
    colors[ImGuiCol_Button] = surface;
    colors[ImGuiCol_ButtonHovered] = interact;
    colors[ImGuiCol_ButtonActive] = interact;
    colors[ImGuiCol_Header] = surface;
    colors[ImGuiCol_HeaderHovered] = interact;
    colors[ImGuiCol_HeaderActive] = interact;
    colors[ImGuiCol_Border] = bg;
    colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_CheckMark] = accent;
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = accent_active;

    style.WindowPadding = ImVec2(32, 32);
    style.FramePadding = ImVec2(16, 10);
    style.ItemSpacing = ImVec2(24, 24);
    style.ItemInnerSpacing = ImVec2(12, 12);
    style.WindowRounding = 12.0f;
    style.ChildRounding = 10.0f;
    style.FrameRounding = 8.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;

    // Bindings Setup
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Main App Simulation Data
    bool showTicketingWindow = true;
    int selectedMovieIndex = -1;
    const char* movies[] = { "Dune: Part Two", "Interstellar", "Blade Runner 2049" };

    // Fetch the hero banner directly from The Movie Database (TMDB) image service API
    ID3D11ShaderResourceView* heroBannerSRV = nullptr;
    int heroWidth = 0, heroHeight = 0;
    // Dune: Part Two backdrop API Image
    LoadTextureFromURL(g_pd3dDevice, "https://image.tmdb.org/t/p/original/xOMo8BRK7PfcJv9JCnx7s5hj0PX.jpg", &heroBannerSRV, &heroWidth, &heroHeight);

    // Load poster images
    ID3D11ShaderResourceView* posters[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
    int posterWidths[6] = { 0, 0, 0, 0, 0, 0 };
    int posterHeights[6] = { 0, 0, 0, 0, 0, 0 };
    
    // Dune Part Two Poster
    LoadTextureFromURL(g_pd3dDevice, "https://image.tmdb.org/t/p/w500/1pdfLvkbY9ohJlCjQH2JGqqUT1O.jpg", &posters[0], &posterWidths[0], &posterHeights[0]);
    // Interstellar Poster
    LoadTextureFromURL(g_pd3dDevice, "https://image.tmdb.org/t/p/w500/gEU2QlsUUQYKnUKtiI00Sn8vRXz.jpg", &posters[1], &posterWidths[1], &posterHeights[1]);
    // Blade Runner 2049 Poster
    LoadTextureFromURL(g_pd3dDevice, "https://image.tmdb.org/t/p/w500/gajva2L0rPYkEWjzgFlBXCAVBE5.jpg", &posters[2], &posterWidths[2], &posterHeights[2]);
    // The Dark Knight
    LoadTextureFromURL(g_pd3dDevice, "https://image.tmdb.org/t/p/w500/qJ2tW6WMUDux911r6m7haRef0WH.jpg", &posters[3], &posterWidths[3], &posterHeights[3]);
    // Inception
    LoadTextureFromURL(g_pd3dDevice, "https://image.tmdb.org/t/p/w500/9gk7adHYeDvHkCSEqAvQNLV5Uge.jpg", &posters[4], &posterWidths[4], &posterHeights[4]);
    // The Matrix
    LoadTextureFromURL(g_pd3dDevice, "https://image.tmdb.org/t/p/w500/f89U3ADr1oiB1s9GkdPOEpXUk5H.jpg", &posters[5], &posterWidths[5], &posterHeights[5]);


    // 4. Windows Event Loop
    bool running = true;
    while (running) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                running = false;
        }
        if (!running) break;

        // 5. Start ImGui Frame Layout
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // --- RENDER CINEMA APP LAYOUT ---
        RenderMainMenu(heroBannerSRV, heroWidth, heroHeight, posters, posterWidths, posterHeights, selectedMovieIndex);

        // 6. Finalization & Frame Presentation
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.05f, 0.05f, 0.05f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // VSync enabled
    }

    // 7. Core Shutdown Procedures
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper engine primitives to construct the DX11 context cleanly
bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK) return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Low level intercept windows processing events to handle mouse hooks/inputs
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_KEYMENU) return 0; // Disable ALT application menu focus
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}



