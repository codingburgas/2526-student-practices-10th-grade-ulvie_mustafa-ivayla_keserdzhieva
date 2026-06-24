#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Forward declarations
void RenderMainMenu(
    ID3D11ShaderResourceView** heroBanners, int* heroWidths, int* heroHeights, int& currentHeroIndex,
    ID3D11ShaderResourceView** posters,     int* posterWidths, int* posterHeights, int& selectedMovieIndex,
    ID3D11ShaderResourceView*  playIconTex,     int playIconW,  int playIconH,
    ID3D11ShaderResourceView*  favoriteIconTex, int favIconW,   int favIconH,
    ID3D11ShaderResourceView*  leftArrowTex,    int leftArrW,   int leftArrH,
    ID3D11ShaderResourceView*  rightArrowTex,   int rightArrW,  int rightArrH,
    bool& outShowModal,
    ID3D11ShaderResourceView* blurBgSrv,
    ID3D11ShaderResourceView* googleIconTex,
    ID3D11ShaderResourceView* appleIconTex,
    ID3D11ShaderResourceView* msIconTex);

void RenderMovieDetail(
    int movieIndex, bool& goBack,
    ID3D11ShaderResourceView** posters, int* posterWidths, int* posterHeights);

static ID3D11Device*           g_pd3dDevice           = nullptr;
static ID3D11DeviceContext*    g_pd3dDeviceContext    = nullptr;
static IDXGISwapChain*         g_pSwapChain           = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static ID3D11Texture2D*          g_blurCapTex         = nullptr;
static ID3D11ShaderResourceView* g_blurCapSRV         = nullptr;

bool LoadTextureFromFile(ID3D11Device* dev, const char* path,
                         ID3D11ShaderResourceView** srv, int* w, int* h) {
    if (!dev || !path) return false;
    int iw = 0, ih = 0;
    unsigned char* data = stbi_load(path, &iw, &ih, nullptr, 4);
    if (!data) return false;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width            = iw;
    desc.Height           = ih;
    desc.MipLevels        = 1;
    desc.ArraySize        = 1;
    desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage            = D3D11_USAGE_DEFAULT;
    desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sub = { data, (UINT)(iw * 4), 0 };
    ID3D11Texture2D* tex = nullptr;
    dev->CreateTexture2D(&desc, &sub, &tex);

    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
    sd.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    sd.Texture2D.MipLevels       = 1;
    sd.Texture2D.MostDetailedMip = 0;
    dev->CreateShaderResourceView(tex, &sd, srv);
    tex->Release();

    *w = iw; *h = ih;
    stbi_image_free(data);
    return true;
}

bool LoadTextureFromURL(ID3D11Device* dev, const char* url,
                        ID3D11ShaderResourceView** srv, int* w, int* h) {
    if (!dev || !url) return false;
    const char* tmp = "temp_poster_cache.jpg";
    if (URLDownloadToFileA(nullptr, url, tmp, 0, nullptr) != S_OK) return false;
    return LoadTextureFromFile(dev, tmp, srv, w, h);
}

bool    CreateDeviceD3D(HWND hWnd);
void    CleanupDeviceD3D();
void    CreateRenderTarget();
void    CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main(int, char**) {
    // Enable DPI awareness so text and images render crisp on high-DPI / 4K screens
    SetProcessDPIAware();

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L,
                       GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
                       L"CinemaSystem", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Cinema Ticketing Terminal",
                                WS_OVERLAPPEDWINDOW, 50, 50, 1600, 900,
                                nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // High-quality font config: OversampleH/V=3 eliminates pixelation in text
    ImFontConfig fc;
    fc.OversampleH = 3;
    fc.OversampleV = 3;
    fc.PixelSnapH  = false;

    // Font atlas — indices must stay stable (mainMenu.cpp references by index)
    io.Fonts->AddFontFromFileTTF("assets/fonts/Inter_18pt-Regular.ttf",     18.0f, &fc); // [0]
    io.Fonts->AddFontFromFileTTF("assets/fonts/Inter_18pt-Regular.ttf",     15.0f, &fc); // [1]
    io.Fonts->AddFontFromFileTTF("assets/fonts/Inter_18pt-Regular.ttf",     11.0f, &fc); // [2]
    io.Fonts->AddFontFromFileTTF("assets/fonts/Geologica_Auto-Regular.ttf", 24.0f, &fc); // [3]
    io.Fonts->AddFontFromFileTTF("assets/fonts/Geologica_Auto-Regular.ttf", 20.0f, &fc); // [4]
    io.Fonts->AddFontFromFileTTF("assets/fonts/Geologica_Auto-Regular.ttf", 58.0f, &fc); // [5]

    // Theme
    ImGuiStyle& style = ImGui::GetStyle();
    auto& col = style.Colors;
    col[ImGuiCol_WindowBg]       = { 21/255.f, 21/255.f, 21/255.f, 1.f };
    col[ImGuiCol_ChildBg]        = col[ImGuiCol_WindowBg];
    col[ImGuiCol_PopupBg]        = col[ImGuiCol_WindowBg];
    col[ImGuiCol_Text]           = { 1.f, 1.f, 1.f, 1.f };
    col[ImGuiCol_Border]         = { 0, 0, 0, 0 };
    col[ImGuiCol_BorderShadow]   = { 0, 0, 0, 0 };
    col[ImGuiCol_Button]         = { 0, 0, 0, 0 };
    col[ImGuiCol_ButtonHovered]  = { 0, 0, 0, 0 };
    col[ImGuiCol_ButtonActive]   = { 0, 0, 0, 0 };
    col[ImGuiCol_FrameBg]        = { 0.11f, 0.11f, 0.11f, 1.f };
    col[ImGuiCol_FrameBgHovered] = col[ImGuiCol_FrameBg];
    col[ImGuiCol_FrameBgActive]  = col[ImGuiCol_FrameBg];

    style.WindowPadding    = { 0, 0 };
    style.FramePadding     = { 0, 0 };
    style.ItemSpacing      = { 0, 0 };
    style.ItemInnerSpacing = { 0, 0 };
    style.WindowRounding   = 0.f;
    style.FrameRounding    = 0.f;
    style.WindowBorderSize = 0.f;
    style.FrameBorderSize  = 0.f;

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // ── Hero banners (landscape backdrops from TMDB) ──────────────────────────
    ID3D11ShaderResourceView* heroBanners[3] = {};
    int heroWidths[3] = {}, heroHeights[3] = {};
    LoadTextureFromFile(g_pd3dDevice, "assets/posters/laLaLandPoster.jpg",
        &heroBanners[0], &heroWidths[0], &heroHeights[0]);
    LoadTextureFromFile(g_pd3dDevice, "assets/posters/littleWomenPoster.jpg",
        &heroBanners[1], &heroWidths[1], &heroHeights[1]);
    LoadTextureFromFile(g_pd3dDevice, "assets/posters/theLittlePrincePoster.jpg",
        &heroBanners[2], &heroWidths[2], &heroHeights[2]);

    // ── Posters (local files) ─────────────────────────────────────────────────
    const int totalPosters = 9;
    ID3D11ShaderResourceView* posters[totalPosters] = {};
    int posterWidths[totalPosters] = {}, posterHeights[totalPosters] = {};
    const char* posterFiles[totalPosters] = {
        "assets/posters/10ThingsIHateAboutYouPoster.jpg",
        "assets/posters/deadPoetsSocietyPoster.jpg",
        "assets/posters/laLaLandPoster.jpg",
        "assets/posters/littleWomenPoster.jpg",
        "assets/posters/matildaPoster.jpg",
        "assets/posters/theDramaPoster.jpg",
        "assets/posters/theLittlePrincePoster.jpg",
        "assets/posters/thePrincessDiariesPoster.jpg",
        "assets/posters/theTerminalPoster.jpg"
    };
    for (int i = 0; i < totalPosters; i++)
        LoadTextureFromFile(g_pd3dDevice, posterFiles[i],
                            &posters[i], &posterWidths[i], &posterHeights[i]);

    // ── Icons — tries PNG files; SVGs fall back to programmatic drawing ───────
    // To activate an icon: convert its SVG to PNG with the same base name
    // (e.g. favoriteButton.svg → favoriteButton.png) and drop it in assets/icons/
    ID3D11ShaderResourceView* playIconTex = nullptr; int playIconW = 0, playIconH = 0;
    ID3D11ShaderResourceView* favIconTex  = nullptr; int favIconW  = 0, favIconH  = 0;
    ID3D11ShaderResourceView* leftArrTex  = nullptr; int leftArrW  = 0, leftArrH  = 0;
    ID3D11ShaderResourceView* rightArrTex = nullptr; int rightArrW = 0, rightArrH = 0;
    LoadTextureFromFile(g_pd3dDevice, "assets/icons/playButton.png",       &playIconTex, &playIconW, &playIconH);
    LoadTextureFromFile(g_pd3dDevice, "assets/icons/favoriteButton.png",   &favIconTex,  &favIconW,  &favIconH);
    LoadTextureFromFile(g_pd3dDevice, "assets/icons/leftArrowButton.png",  &leftArrTex,  &leftArrW,  &leftArrH);
    LoadTextureFromFile(g_pd3dDevice, "assets/icons/rightArrowButton.png", &rightArrTex, &rightArrW, &rightArrH);

    ID3D11ShaderResourceView* googleIconTex = nullptr; int googleIconW = 0, googleIconH = 0;
    ID3D11ShaderResourceView* appleIconTex  = nullptr; int appleIconW  = 0, appleIconH  = 0;
    ID3D11ShaderResourceView* msIconTex     = nullptr; int msIconW     = 0, msIconH     = 0;
    LoadTextureFromFile(g_pd3dDevice, "assets/icons/googleIcon.png",    &googleIconTex, &googleIconW, &googleIconH);
    LoadTextureFromFile(g_pd3dDevice, "assets/icons/appleIcon.png",     &appleIconTex,  &appleIconW,  &appleIconH);
    LoadTextureFromFile(g_pd3dDevice, "assets/icons/microsoftIcon.png", &msIconTex,     &msIconW,     &msIconH);

    int selectedMovieIndex = -1;
    int currentHeroIndex   = 0;
    bool showModal         = false;

    bool running = true;
    while (running) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) running = false;
        }
        if (!running) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (selectedMovieIndex >= 0) {
            bool goBack = false;
            RenderMovieDetail(selectedMovieIndex, goBack,
                posters, posterWidths, posterHeights);
            if (goBack) selectedMovieIndex = -1;
        } else {
            RenderMainMenu(heroBanners, heroWidths, heroHeights, currentHeroIndex,
                posters, posterWidths, posterHeights, selectedMovieIndex,
                playIconTex, playIconW, playIconH,
                favIconTex,  favIconW,  favIconH,
                leftArrTex,  leftArrW,  leftArrH,
                rightArrTex, rightArrW, rightArrH,
                showModal, g_blurCapSRV,
                googleIconTex, appleIconTex, msIconTex);
        }

        ImGui::Render();
        const float cc[4] = { 21/255.f, 21/255.f, 21/255.f, 1.f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, cc);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Capture backbuffer into blur texture while the modal is not open
        if (!showModal && g_blurCapTex) {
            ID3D11Texture2D* backBuf = nullptr;
            if (SUCCEEDED(g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuf)))) {
                g_pd3dDeviceContext->CopyResource(g_blurCapTex, backBuf);
                backBuf->Release();
            }
        }

        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount                        = 2;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = hWnd;
    sd.SampleDesc.Count                   = 1;
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL fl;
    const D3D_FEATURE_LEVEL fla[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        fla, 2, D3D11_SDK_VERSION, &sd,
        &g_pSwapChain, &g_pd3dDevice, &fl, &g_pd3dDeviceContext);
    if (hr != S_OK) return false;
    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain)        { g_pSwapChain->Release();        g_pSwapChain        = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice)        { g_pd3dDevice->Release();        g_pd3dDevice        = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);

    // Create matching texture for blur background capture
    D3D11_TEXTURE2D_DESC bd = {};
    pBackBuffer->GetDesc(&bd);
    bd.BindFlags      = D3D11_BIND_SHADER_RESOURCE;
    bd.Usage          = D3D11_USAGE_DEFAULT;
    bd.CPUAccessFlags = 0;
    if (SUCCEEDED(g_pd3dDevice->CreateTexture2D(&bd, nullptr, &g_blurCapTex))) {
        D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
        sd.Format              = bd.Format;
        sd.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
        sd.Texture2D.MipLevels = 1;
        g_pd3dDevice->CreateShaderResourceView(g_blurCapTex, &sd, &g_blurCapSRV);
    }
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
    if (g_blurCapSRV)           { g_blurCapSRV->Release();          g_blurCapSRV           = nullptr; }
    if (g_blurCapTex)           { g_blurCapTex->Release();          g_blurCapTex           = nullptr; }
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
