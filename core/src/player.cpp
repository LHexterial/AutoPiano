#include "core/player.h"
#include "core/types.h"
#include <windows.h>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <atomic>

struct TimePeriodGuard {
    TimePeriodGuard(UINT period) { timeBeginPeriod(period); }
    ~TimePeriodGuard() { timeEndPeriod(1); }
};

void precise_sleep(double seconds) 
{
    if (seconds <= 0.0) return;
    LARGE_INTEGER freq, start, current;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    
    double targetTicks = seconds * freq.QuadPart;
    
    while (true) {
        QueryPerformanceCounter(&current);
        double elapsedTicks = static_cast<double>(current.QuadPart - start.QuadPart);
        if (elapsedTicks >= targetTicks) break;
        
        double remainingSeconds = (targetTicks - elapsedTicks) / freq.QuadPart;
        if (remainingSeconds > 0.002) {
            Sleep(1); 
        }
    }
}

// 硬件扫描码
std::unordered_map<char, WORD> SCAN_CODES = {
    {',', 0x33}, {'.', 0x34}, {'/', 0x35}, {'i', 0x17}, {'o', 0x18}, {'p', 0x19}, {'[', 0x1a},
    {'l', 0x26}, {';', 0x27}, {'9', 0x0a}, {'0', 0x0b}, {'-', 0x0c},
    {'z', 0x2c}, {'x', 0x2d}, {'c', 0x2e}, {'v', 0x2f}, {'b', 0x30}, {'n', 0x31}, {'m', 0x32},
    {'s', 0x1f}, {'d', 0x20}, {'g', 0x22}, {'h', 0x23}, {'j', 0x24},
    {'q', 0x10}, {'w', 0x11}, {'e', 0x12}, {'r', 0x13}, {'t', 0x14}, {'y', 0x15}, {'u', 0x16},
    {'2', 0x03}, {'3', 0x04}, {'5', 0x06}, {'6', 0x07}, {'7', 0x08}
};

void sendKey(WORD scanCode, bool isDown)
{
    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = scanCode;
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    if (!isDown)
        input.ki.dwFlags |= KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

void play(const std::vector<Action>& actionQueue, 
          double speedMultiplier, 
          double prepareTime, 
          std::atomic<bool>& isPlaying, 
          std::atomic<float>& currentProgress)
{
    if (actionQueue.empty())
        return;
    Sleep(static_cast<DWORD>(prepareTime * 1000));
    timeBeginPeriod(1);
    if (speedMultiplier <= 0.0) speedMultiplier = 1.0;
    TimePeriodGuard guard(1);
    LARGE_INTEGER frequency, startTime, currentTime;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&startTime);
    std::unordered_set<WORD> activeKeys;
    
    double totalDuration = actionQueue.back().time;
    // 🌟 改造 3：保命代码，防止除以 0
    if (totalDuration <= 0.0) totalDuration = 1.0; 

    for (size_t i = 0; i < actionQueue.size(); ++i)
    {
        // 🌟 核心：监听主界面发来的“停止”信号，以及 F10 物理急停
        if (!isPlaying || (GetAsyncKeyState(VK_F10) & 0x8000))
        {
            isPlaying = false; 
            break; 
        }

        const auto& action = actionQueue[i];

        // 🌟 核心：将进度实时汇报给 UI 界面
        currentProgress = static_cast<float>(action.time / totalDuration);

        double targetTime = action.time / speedMultiplier;
        while (true)
        {
            QueryPerformanceCounter(&currentTime);
            double elapsed = static_cast<double>(currentTime.QuadPart - startTime.QuadPart) / frequency.QuadPart;
            double timeLeft = targetTime - elapsed;
            if (timeLeft <= 0)
            {
                break;
            }
            else if (timeLeft > 0.001){
                precise_sleep(timeLeft - 0.0005);
            }
        }

        WORD hexCode = SCAN_CODES[action.key];
        if (action.type == "down")
        {
            sendKey(hexCode, true);
            activeKeys.insert(hexCode);
        }
        else
        {
            sendKey(hexCode, false);
            activeKeys.erase(hexCode);
        }
    }

    // 无论如何，退出前松开所有按键，防止游戏卡键
    for (auto code : activeKeys)
        sendKey(code, false);

    timeEndPeriod(1);
}