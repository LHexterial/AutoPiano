#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#include "ui_manager.h"
#include "core/AutoPianoEngine.h" // 🌟 引入核心引擎
#include <windows.h>
#include <iostream>

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

int main() {
    // 系统级初始化
    std::string targetTitle = LoadTargetTitleFromConfig();
    HWND consoleWnd = GetConsoleWindow();
    if (consoleWnd) {
        ShowWindow(consoleWnd, SW_HIDE);
    }
    
    // 初始化 COM 环境 (用于文件选择对话框)
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    // 初始化 GUI 窗口
    if (!GUI::Initialize("AutoPiano v3.0 - MVC架构版", 100, 100)) {
        return 1;
    }

    // ==========================================
    // 🌟 2. 实例化核心引擎 (大脑)
    // ==========================================
    // 这一行代码就替代了原来 main 里所有的原子变量和线程定义
    AutoPianoEngine engine;
    engine.SetTargetWindow(targetTitle); 
    
    bool uiKeepAlive = true;

    // ==========================================
    // 🌟 3. 极致清爽的主循环
    // ==========================================
    while (!GUI::ShouldClose() && uiKeepAlive) {
        GUI::NewFrame();

        // A. 每一帧让大脑处理逻辑
        // 内部包含：F9/F10监听、线程收尸、自动下一首、状态更新
        engine.UpdateLogic();

        // B. 把大脑的引用传给 UI 界面进行绘制
        // UI 会自动通过 engine 的接口读取进度、播放状态，并调用控制函数
        GUI::UpdateUI(&engine, uiKeepAlive);

        GUI::Render();
    }

    // ==========================================
    // 4. 优雅退出
    // ==========================================
    // 当 main 函数结束时，engine 对象会被销毁
    // 它的析构函数会自动执行 Stop() 和 join()，保证程序关闭时不会崩溃
    
    GUI::Shutdown();
    CoUninitialize();
    return 0;
}