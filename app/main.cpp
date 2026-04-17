#include "core/midi_parser.h"
#include "core/player.h"
#include <iostream>
#include <string>
#include <windows.h>
#include <winuser.h>
#include <shellapi.h>

int main(int argc, char* argv[]) {
    ChangeWindowMessageFilter(0x0049, 1);
    ChangeWindowMessageFilter(WM_DROPFILES, 1);
    system("chcp 65001 > nul");
    std::cout << "AutoPiano " << PROJECT_VERSION << std::endl;
    std::cout << "[tips] 你可以把 [.mid] 文件拖拽到命令行进行读取" << std::endl;
    std::string midiFile;
    if (argc > 1) {
        midiFile = argv[1]; 
    }
    if (midiFile.empty()) return 1;

    // 2. 🌟 绝对路径魔法：获取 exe 所在目录，防止拖拽导致路径错乱
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exeDir = std::string(exePath);
    exeDir = exeDir.substr(0, exeDir.find_last_of("\\/"));
    std::string iniPath = exeDir + "\\config.ini";
    char speedStr[32], prepStr[32];
    GetPrivateProfileStringA("Settings", "speed", "0.90", speedStr, 32, iniPath.c_str());
    GetPrivateProfileStringA("Settings", "prepare_time", "5.0", prepStr, 32, iniPath.c_str());
    
    double speed = std::stod(speedStr);
    double prepare = std::stod(prepStr);

    std::cout << "-----------------------------------" << std::endl;
    std::cout << "加载曲谱: " << midiFile << std::endl;
    std::cout << "运行倍速: " << speed << "x" << std::endl;
    std::cout << "准备时间: " << prepare << "s" << std::endl;
    std::cout << "配置文件: " << iniPath << std::endl;
    std::cout << "-----------------------------------" << std::endl;
    

    auto actionQueue = decodeMidi(midiFile, 0.0);
    if (actionQueue.empty())
    {
        std::cout << "\n[Error] 文件读取失败，未解析到相关动作队列" << std::endl;
        system("pause");
        return 1;
    }
    play(actionQueue, speed, prepare);
    std::cout << "\n任务已完成" << std::endl;
    Sleep(2000);
    return 0;
}