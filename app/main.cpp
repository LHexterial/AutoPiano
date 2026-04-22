#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#include "ui_manager.h"
#include "core/player.h"
#include "core/midi_parser.h"
#include <thread>
#include <atomic>
#include <windows.h>
#include <iostream>

// 全局原子变量
static std::atomic<bool> isPaused{false};
static std::atomic<bool> isPlaying{ false };
static std::atomic<float> currentProgress{ 0.0f };
static std::thread playThread;

std::string LoadTargetTitleFromConfig() {
    char result[] = {0};
    
    // 获取 config.ini 的绝对路径，确保双击运行也能找到
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string iniPath = std::string(exePath).substr(0, std::string(exePath).find_last_of("\\/")) + "\\config.ini";

    // 使用 Windows 原生 API 读取
    GetPrivateProfileStringA("Settings", "TargetWindow", "第五人格", result, 256, iniPath.c_str());
    return std::string(result);
}

struct TimePeriodGuard {
    explicit TimePeriodGuard(UINT period) { timeBeginPeriod(period); }
    ~TimePeriodGuard() { timeEndPeriod(1); }
};
// ==================== UTF-8 转 ANSI（用于旧式文件 API） ====================
// 解决 ImGui 的 UTF-8 编码与 Windows 底层 C++ 读取中文路径冲突的问题
std::string UTF8ToANSI(const std::string& utf8Str) {
    if (utf8Str.empty()) return "";
    
    // 1. UTF-8 转宽字符 (UTF-16)
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
    if (wideSize <= 0) return "";
    std::wstring wideStr(wideSize, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, wideStr.data(), wideSize);
    
    // 2. 宽字符转 ANSI (GBK 等本地编码)
    int ansiSize = WideCharToMultiByte(CP_ACP, 0, wideStr.c_str(), -1, NULL, 0, NULL, NULL);
    if (ansiSize <= 0) return "";
    std::string ansiStr(ansiSize, 0);
    WideCharToMultiByte(CP_ACP, 0, wideStr.c_str(), -1, ansiStr.data(), ansiSize, NULL, NULL);
    
    // 移除末尾多余的 '\0' 终止符
    ansiStr.resize(ansiSize - 1);
    return ansiStr;
}

// 子线程包装函数
void PlayThreadWrapper(std::string path, float speed, float prep, std::string targetTitle) {
    try {
        TimePeriodGuard timerGuard(1);
        auto actions = decodeMidi(UTF8ToANSI(path), 0.0);
        if (actions.empty()) throw std::runtime_error("解析失败");

        play(actions, speed, prep, isPlaying, isPaused,currentProgress, targetTitle);

    } catch (const std::exception& e) {
        MessageBoxA(nullptr, e.what(), "错误", MB_OK | MB_ICONERROR);
    }
    isPlaying = false;
    currentProgress = 0.0f;
}
int main() {
    std::string targetTitle = LoadTargetTitleFromConfig();
    HWND consoleWnd = GetConsoleWindow();
    if (consoleWnd) {
        ShowWindow(consoleWnd, SW_HIDE);
    }
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    if (!GUI::Initialize("AutoPiano v2.0", 100, 100)) {
        return 1;
    }

    float speed = 1.0f;
    float prep = 3.0f;
    std::string midiPath = "未加载曲谱";
    bool uiKeepAlive = true;
    bool wasF9Pressed = false;
    while (!GUI::ShouldClose() && uiKeepAlive) {
        GUI::NewFrame();
        bool isF9Pressed = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
        if (isPlaying.load() && isF9Pressed && !wasF9Pressed)
        {
            isPaused = !isPaused;
        }
        wasF9Pressed = isF9Pressed;

        bool uiPlaying = isPlaying.load();
        bool uiPaused = isPaused.load();
        float uiProgress = currentProgress.load();

        GUI::UpdateUI(speed, prep, midiPath, uiPlaying, uiPaused,uiProgress, uiKeepAlive, targetTitle);

        if (uiPaused != isPaused.load())
        {
            isPaused.store(uiPaused);
        }
        // 状态机切换逻辑
        
        if (uiPlaying != isPlaying.load())
        {
            isPlaying.store(uiPlaying);// 及时更新状态
            if (uiPlaying)
            {
                isPaused.store(false);
                if (playThread.joinable())
                    playThread.join();
                playThread = std::thread(PlayThreadWrapper, midiPath, speed, prep, targetTitle);
            }
            else
            {
                if (playThread.joinable())
                {
                    playThread.join();
                }
            }
        }


        GUI::Render();
    }

    // 优雅退出
    isPlaying = false;
    if (playThread.joinable()) playThread.join();

    GUI::Shutdown();
    CoUninitialize();
    return 0;
}