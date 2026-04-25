#include "core/player.h"
#include "core/types.h"
#include <windows.h>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <atomic>

std::string ConvertUTF8ToANSI(const std::string &utf8Str);

bool IsTargetWindowActive(const std::string& keyword_utf8)
{
    if (keyword_utf8.empty()) return true;

    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return false;

    char currentTitle[256];
    GetWindowTextA(hwnd, currentTitle, sizeof(currentTitle));
    std::string titleStr(currentTitle);

    //用转换后的 ANSI 字符串去匹配系统的 ANSI 标题
    std::string keyword_ansi = ConvertUTF8ToANSI(keyword_utf8);

    return (titleStr.find(keyword_ansi) != std::string::npos);
}

std::string ConvertUTF8ToANSI(const std::string& utf8Str) {
    if (utf8Str.empty()) return "";
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
    std::wstring wideStr(wideSize, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, wideStr.data(), wideSize);
    int ansiSize = WideCharToMultiByte(CP_ACP, 0, wideStr.c_str(), -1, NULL, 0, NULL, NULL);
    std::string ansiStr(ansiSize, 0);
    WideCharToMultiByte(CP_ACP, 0, wideStr.c_str(), -1, ansiStr.data(), ansiSize, NULL, NULL);
    if (!ansiStr.empty() && ansiStr.back() == '\0') ansiStr.pop_back();
    return ansiStr;
}

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
          std::atomic<bool>& isPaused,
          std::atomic<float>& currentProgress, std::string targetTitle)
{
    if (actionQueue.empty())
        return;
    int totalSleepMs = static_cast<int>(prepareTime * 1000);
    int sleptMs = 0;
    while (sleptMs < totalSleepMs) {
        // 在准备倒计时期间，如果玩家按了停止，瞬间退出线程！
        if (!isPlaying.load() || (GetAsyncKeyState(VK_F10) & 0x8000)) {
            isPlaying = false;
            return; 
        }
        Sleep(10); // 每次只睡 10 毫秒
        sleptMs += 10;
    }
    timeBeginPeriod(1);
    if (speedMultiplier <= 0.0) speedMultiplier = 1.0;
    TimePeriodGuard guard(1);
    LARGE_INTEGER frequency, startTime, currentTime;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&startTime);
    std::unordered_set<WORD> activeKeys;
    
    double totalDuration = actionQueue.back().time;
    
    if (totalDuration <= 0.0) totalDuration = 1.0;
    for (size_t i = 0; i < actionQueue.size(); ++i)
    {
        bool isF10Pressed = GetAsyncKeyState(VK_F10) & 0x8000;

        if (!isPlaying.load() || isF10Pressed)
        {
            isPlaying.store(false);
            break; 
        }

        const auto& action = actionQueue[i];

        
        currentProgress = static_cast<float>(action.time / totalDuration);

        double targetTime = action.time / speedMultiplier;
        while (true)
        {
            if (!isPlaying.load())
            {
                break;
            }
            // 这里是自旋区，为了子线程快速反应前端传入的isPaused变量
            if (isPaused.load())
            {
                if (!activeKeys.empty())// 收到暂停指令之后清除按键缓存防止游戏内按键停滞
                {
                    for (auto code : activeKeys)
                    {
                        sendKey(code, false);
                    }
                    activeKeys.clear();
                }
                LARGE_INTEGER pauseStart, pauseEnd;
                QueryPerformanceCounter(&pauseStart);
                while (isPaused.load() && isPlaying.load())
                {
                    Sleep(10);
                }
                QueryPerformanceCounter(&pauseEnd);
                startTime.QuadPart += (pauseEnd.QuadPart - pauseStart.QuadPart);// 修正因为暂停而导致的按键开始时间的偏移
            }

            QueryPerformanceCounter(&currentTime);
            double elapsed = static_cast<double>(currentTime.QuadPart - startTime.QuadPart) / frequency.QuadPart;
            double timeLeft = targetTime - elapsed;
            if (timeLeft <= 0)
            {
                break;
            }
            else if (timeLeft > 0.015)
                Sleep(1);
            else if (timeLeft > 0.001){
                precise_sleep(timeLeft - 0.0005);
            }
        }
        if (!isPlaying.load())
        {
            break;
        }

        bool isActive = IsTargetWindowActive(targetTitle);

        WORD hexCode = SCAN_CODES[action.key];
        if (isActive) 
        {
            sendKey(hexCode, action.type == "down");
            if (action.type == "down") activeKeys.insert(hexCode);
            else activeKeys.erase(hexCode);
        } else 
        {
            // 如果中途切走了窗口，立即松开所有已按下的键，防止游戏内卡死
            if (!activeKeys.empty()) 
            {
                for (auto code : activeKeys) sendKey(code, false);
                activeKeys.clear();
            }
        }
    }
     for (auto code : activeKeys)
        sendKey(code, false);

    timeEndPeriod(1);
}

    // 无论如何，退出前松开所有按键，防止游戏卡键