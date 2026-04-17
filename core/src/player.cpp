#include "core/player.h"
#include "core/types.h"
#include <windows.h>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

typedef NTSTATUS(WINAPI *NtDelayExecution_t)(BOOLEAN Alertable, PLARGE_INTEGER DelayInterval);// 抓取windows内核函数

NtDelayExecution_t pNtDelayExecution = nullptr;

void init_ntdll() {
    if (pNtDelayExecution == nullptr)
    {
        HMODULE hNtDll = GetModuleHandleA("ntdll.dll");

        if (hNtDll)
        {
            pNtDelayExecution = (NtDelayExecution_t)GetProcAddress(hNtDll, "NtDelayExecution");
        }
    }
}

void precise_sleep(double seconds) // 封装精准休憩函数
{
    if (seconds <= 0 || pNtDelayExecution == nullptr)
        return;
    LARGE_INTEGER delay;
    delay.QuadPart = static_cast<LONGLONG>(-seconds * 10000000.0);
    pNtDelayExecution(FALSE, &delay);
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

void show_pre_mesg(double time)
{
    std::cout << "脚本将于: " << time << "秒启动";
}

void play(const std::vector<Action> &actionQueue, double speedMultiplier, double prepareTime)
{
    init_ntdll();
    show_pre_mesg(prepareTime);
    Sleep(static_cast<DWORD>(prepareTime * 1000));
    timeBeginPeriod(1);
    LARGE_INTEGER frequency, startTime, currentTime;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&startTime);
    std::unordered_set<WORD> activeKeys;
    double totalDuration = actionQueue.back().time;
    int lastPercent = -1;

    for (const auto& action : actionQueue)
    {
        if (GetAsyncKeyState(VK_F10) & 0x8000)
        {
            break;
        }
        int currentPercent = static_cast<int>((action.time / totalDuration) * 100);
        if (currentPercent != lastPercent)
        {
            int barWidth = 30;
            int pos = barWidth *currentPercent / 100;
            std::cout << "\r" << "[";
            for (int i = 0; i < barWidth; ++i)
            {
                if (i < pos)
                    std::cout << "=";
                else if (i == pos)
                    std::cout << ">";
                else
                    std::cout << " ";
            }
            std::cout << "] " << currentPercent << "%" << std::flush;
            lastPercent = currentPercent;
        }
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
    for (auto code : activeKeys)
        sendKey(code, false);

    timeEndPeriod(1);
}

