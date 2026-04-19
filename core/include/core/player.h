#pragma once
#include "types.h"
#include <vector>
#include <atomic>

// 核心播放函数：接收原子变量以便主线程随时打断和读取进度
void play(const std::vector<Action>& actionQueue, 
          double speedMultiplier, 
          double prepareTime, 
          std::atomic<bool>& isPlaying, 
          std::atomic<float>& currentProgress, std::string targetTitle);