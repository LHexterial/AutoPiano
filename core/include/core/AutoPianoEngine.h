#pragma once
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <cstddef>

struct Action;

class AutoPianoEngine
{
private:

    // 核心数据区域，控制线程与ui沟通的状态数据
    std::atomic<bool> isPlaying{false};
    std::atomic<bool> isPaused{false};
    std::atomic<float> currentProgress{0.0f};

    // 播放列表
    std::vector<std::string> playlist;
    size_t currentTrackIdx = 0;

    // 播放设置
    float speed = 1.0f;
    float prepTime = 3.0f;
    std::string targetTitle = "";

    // 线程设置
    std::thread playThread;
    bool wasF9Pressed = false;// 暂停按键
    bool wasF10Pressed = false; // 停止按键
    void StartThread(const std::string &midiPath);// 内部私有方法
    void PlayTask(std::string midiPath);
public:
    AutoPianoEngine();
    ~AutoPianoEngine();

    // UI控制指令
    void UpdateLogic();
    void StartPlaylist();// 开始播放队列
    void Stop();         // 停止播放
    void TogglePause(); // 切换暂停状态

    // 列表管理指令
    void AddTrack(const std::string &path);
    // AutoPianoEngine.h 里面加上这一句
    void SetCurrentTrackIdx(size_t index) { if (index < playlist.size()) currentTrackIdx = index; }
    void RemoveTrack(size_t index);
    void ClearPlaylist();

    // 设置和获取
    void SetTargetWindow(const std::string &title) { targetTitle = title; };
    float *GetSpeedPtr() { return &speed; };
    float *GetPrepPtr() { return &prepTime; };

    // 获取内部状态
    bool IsPlaying() const { return isPlaying.load(); };
    bool IsPaused() const { return isPaused.load(); };
    float GetProgress() const { return currentProgress.load(); };
    const std::vector<std::string> &GetPlaylist() const { return playlist; };
    size_t GetCurrentTrackIdx() const { return currentTrackIdx; };

};