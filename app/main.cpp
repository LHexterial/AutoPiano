#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#include "ui_manager.h"
#include "core/player.h"
#include "core/midi_parser.h"
#include <thread>
#include <atomic>
#include <windows.h>
#include <iostream>

// 全局原子变量
static std::atomic<bool> isPlaying{ false };
static std::atomic<float> currentProgress{ 0.0f };
static std::thread playThread;

// 你的 RAII 守卫：放在这里或头文件里
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
void PlayThreadWrapper(std::string path, float speed, float prep) {
    try {
        TimePeriodGuard timerGuard(1); // 自动开启/关闭高精度模式
        
        // 1. 转换路径并解析
        // 注意：这里建议保留 core/midi_parser.cpp 里的逻辑
        auto actions = decodeMidi(UTF8ToANSI(path), 0.0);
        
        if (actions.empty()) {
            throw std::runtime_error("无法解析 MIDI 文件或文件为空。");
        }

        // 2. 调用 player.cpp 里的 play 函数
        // 你的 player.cpp 里的 play 应该已经被改造成接受 std::atomic 参数的版本了
        play(actions, speed, prep, isPlaying, currentProgress);

    } catch (const std::exception& e) {
        MessageBoxA(nullptr, e.what(), "AutoPiano 错误", MB_OK | MB_ICONERROR);
    }
    
    isPlaying = false;
    currentProgress = 0.0f;
}

int main() {
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
    while (!GUI::ShouldClose() && uiKeepAlive) {
        GUI::NewFrame();

        bool uiPlaying = isPlaying.load();
        float uiProgress = currentProgress.load();

        GUI::UpdateUI(speed, prep, midiPath, uiPlaying, uiProgress, uiKeepAlive);

        // 状态机切换逻辑
        if (uiPlaying && !isPlaying.load()) {
            // 开始播放
            isPlaying = true;
            if (playThread.joinable()) playThread.join(); // 清理旧线程
            playThread = std::thread(PlayThreadWrapper, midiPath, speed, prep);
        } 
        else if (!uiPlaying && isPlaying.load()) {
            // 请求停止
            isPlaying = false;
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