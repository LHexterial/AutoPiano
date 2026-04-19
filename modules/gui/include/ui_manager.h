#pragma once
#include <string>

namespace GUI {
    // 初始化窗口、DirectX 11 和 ImGui (已包含 DPI 缩放处理)
    bool Initialize(const char* title, int width, int height);
    
    // 处理 Windows 消息循环，检查是否退出
    bool ShouldClose();
    
    // 准备新的一帧 (包含官方示例中的 Resize 延迟处理和后台休眠优化)
    void NewFrame();
    
    // 绘制 AutoPiano 专属控制台
    void UpdateUI(float& speed, float& prep, std::string& midiPath, bool& isPlaying, float progress, bool& keepAlive);
    
    // 渲染输出到屏幕
    void Render();
    
    //打开文件夹
    std::string OpenFileDialog();
    // 释放所有资源
    void Shutdown();
}