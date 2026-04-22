#include "ui_manager.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <windows.h>
#include <commdlg.h> // 用于文件选择弹窗

// imgui官方示例文件内的辅助函数声明
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 全局变量的声明，来自官方的示例文件
static HWND                     g_hwnd = nullptr;
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static bool                     g_WindowShouldClose = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static bool                     g_done = false;

// 声明 ImGui 的 Windows 消息处理函数
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Helper functions 以下是来自官方示例文件的辅助函数
bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    // This is a basic setup. Optimally could use e.g. DXGI_SWAP_EFFECT_FLIP_DISCARD and handle fullscreen mode differently. See #8979 for suggestions.
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
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    // Disable DXGI's default Alt+Enter fullscreen behavior.
    // - You are free to leave this enabled, but it will not work properly with multiple viewports.
    // - This must be done for all windows associated to the device. Our DX11 backend does this automatically for secondary viewports that it creates.
    IDXGIFactory* pSwapChainFactory;
    if (SUCCEEDED(g_pSwapChain->GetParent(IID_PPV_ARGS(&pSwapChainFactory))))
    {
        pSwapChainFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
        pSwapChainFactory->Release();
    }

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

namespace GUI {
    // 初始化窗口、DirectX 11 和 ImGui (已包含 DPI 缩放处理)
    #pragma region Initialize
    bool Initialize(const char* title, int width, int height)
    {
        // 开启 DPI 缩放感知 (官方标准)
        ImGui_ImplWin32_EnableDpiAwareness();
        float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

        // 创建窗口 (使用宽字符处理标题)
        WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"AutoPianoWindowClass", nullptr };
        ::RegisterClassExW(&wc);

        int len = MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
        wchar_t* wTitle = new wchar_t[len];
        MultiByteToWideChar(CP_UTF8, 0, title, -1, wTitle, len);
        g_hwnd = ::CreateWindowW(wc.lpszClassName, wTitle, WS_OVERLAPPEDWINDOW, 100, 100, (int)(width * main_scale), (int)(height * main_scale), nullptr, nullptr, wc.hInstance, nullptr);
        delete[] wTitle;

        // 调用你复制的官方辅助函数
        if (!CreateDeviceD3D(g_hwnd)) {
            CleanupDeviceD3D();
            ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
            return false;
        }

        ::ShowWindow(g_hwnd, SW_HIDE);
        ::UpdateWindow(g_hwnd);

        // 设置 ImGui 环境
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // 开启停靠
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // 开启多视口

        ImGui::StyleColorsDark();

        // 适配 DPI 缩放样式
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(main_scale);
        style.FontScaleDpi = main_scale;
        io.ConfigDpiScaleFonts = true;
        io.ConfigDpiScaleViewports = true;

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // 初始化后端
        ImGui_ImplWin32_Init(g_hwnd);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

        // 加载微软雅黑字体，解决中文乱码
        if (!io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\msyh.ttc", 17.0f * main_scale, NULL, io.Fonts->GetGlyphRangesChineseFull())) {
            io.Fonts->AddFontDefault();
        }
        g_done = false;
        return true;
    }
    #pragma endregion
    // 处理 Windows 消息循环，检查是否退出
    bool ShouldClose()
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                g_done = true;
        }
        return g_done;
    }

    // 准备新的一帧 (包含官方示例中的 Resize 延迟处理和后台休眠优化)
    void NewFrame()
    {
//        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
//            ::Sleep(10);
//        }
        g_SwapChainOccluded = false;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }
    
    // 绘制 AutoPiano 专属控制台
    void UpdateUI(float &speed, float &prep, std::string &midiPath, bool &isPlaying, bool& isPaused, float progress, bool &keepAlive, std::string& targetTitle)
    {
 //       ImGui::DockSpaceOverViewport();
        ImGui::SetNextWindowSize(ImVec2(550, 450), ImGuiCond_FirstUseEver);

        if (!ImGui::Begin("AutoPiano v2.0 by 别净吃饭啊", &keepAlive, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }

        if (!targetTitle.empty()) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[焦点锁定] 已绑定目标: %s", targetTitle.c_str());
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "[全局模式] 未绑定窗口，全局发送按键");
        }
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("状态: %s", isPlaying ? "正在演奏中..." : "等待指令");
        ImGui::Separator();

        ImGui::BeginDisabled(isPlaying);
        ImGui::Text("曲谱路径: %s", midiPath.c_str());
        if (ImGui::Button("选择曲谱", ImVec2(0, 0))) {
            std::string selected = OpenFileDialog();
            if (!selected.empty()) midiPath = selected;
        }
        ImGui::SliderFloat("演奏速度", &speed, 0.1f, 3.0f, "%.2f x");
        ImGui::SliderFloat("准备时间", &prep, 0.0f, 10.0f, "%.1f 秒");
        ImGui::EndDisabled();

        ImGui::Spacing();
        if (isPlaying) {
            if (isPaused)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f)); // 绿色
                if (ImGui::Button("继续演奏(F9)",ImVec2(ImGui::GetContentRegionAvail().x / 2 - 5, 50)))
                {
                    isPaused = false;
                }
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.5f, 0.1f, 1.0f)); // 橙色
                if (ImGui::Button("暂停演奏 (F9)", ImVec2(ImGui::GetContentRegionAvail().x / 2 - 5, 50)))
                {
                    isPaused = true;
                }
            }
            ImGui::PopStyleColor();
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("停止播放 (F10)", ImVec2(-1, 50)))
            {
                isPlaying = false;
                isPaused = false;
            }
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
            if (ImGui::Button("开始演奏(start)", ImVec2(-1, 50))) {
                if (!midiPath.empty() && midiPath != "未加载曲谱")
                {
                    isPlaying = true;
                    isPaused = false;
                }
            }
            ImGui::PopStyleColor();
        }

        ImGui::ProgressBar(progress, ImVec2(-1, 0));
        ImGui::End();
    }
    
    std::string OpenFileDialog() 
    {
    OPENFILENAMEW ofn = {};
    wchar_t szFile[260] = { 0 };                     
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = 260;
    ofn.lpstrFilter = L"MIDI Files\0*.mid;*.midi\0All Files\0*.*\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameW(&ofn)) {
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, szFile, -1, NULL, 0, NULL, NULL);
        std::string utf8Path(size_needed, '\0');
        WideCharToMultiByte(CP_UTF8, 0, szFile, -1, &utf8Path[0], size_needed, NULL, NULL);
        if (!utf8Path.empty() && utf8Path.back() == '\0')
            utf8Path.pop_back();
        return utf8Path;
    }
    return "";
    }
    // 渲染输出到屏幕
    void Render()
    {
        ImGui::Render();
        float clear_color_with_alpha[4] = { 0.1f, 0.1f, 0.1f, 1.0f };   // 修正：必须为数组
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }
    
    // 释放所有资源
    void Shutdown()
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        CleanupDeviceD3D();
        ::DestroyWindow(g_hwnd);
        ::UnregisterClassW(L"AutoPianoWindowClass", GetModuleHandle(nullptr));
    }
}