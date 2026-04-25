#include  "core/AutoPianoEngine.h"
#include "core/player.h"
#include "core/midi_parser.h"
#include <windows.h>
#include <algorithm>

extern std::string ConvertUTF8ToANSI(const std::string& utf8Str);

AutoPianoEngine::AutoPianoEngine(){};
AutoPianoEngine::~AutoPianoEngine(){
    Stop();
    if (playThread.joinable())
        playThread.join();
}

void AutoPianoEngine::UpdateLogic()
{
    // 更新一下按键触发
    bool isF9Down = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
    if (isPlaying.load() && isF9Down && !wasF9Pressed) {
        TogglePause();
    }
    wasF9Pressed = isF9Down;

    bool isF10Down = (GetAsyncKeyState(VK_F10) & 0x8000) != 0;
    if (isPlaying.load() && isF10Down) {
        Stop();
    }

    if (!isPlaying.load() && playThread.joinable()) {
        // 子线程已经执行完毕，主线程进行回收
        playThread.join();

        // 判定是否为自然播放结束（进度 > 98%）
        if (currentProgress.load() >= 0.98f) {
            if (currentTrackIdx + 1 < playlist.size()) {
                currentTrackIdx++;
                StartThread(playlist[currentTrackIdx]);
            } else {
                currentProgress.store(0.0f); // 全部播完
            }
        } else {
            currentProgress.store(0.0f); // 被用户掐断
        }
    }
}

void AutoPianoEngine::StartPlaylist() {
    if (playlist.empty()) return;
    if (isPlaying.load()) Stop();
    
    // 等待上一条线程彻底退出后再启动（保证安全）
    if (playThread.joinable()) playThread.join();
    
    StartThread(playlist[currentTrackIdx]);
}

void AutoPianoEngine::Stop() {
    isPlaying.store(false);
    isPaused.store(false);
    // 注意：这里不直接 join，让 UpdateLogic 在下一帧安全回收
}

void AutoPianoEngine::TogglePause() {
    if (isPlaying.load()) {
        isPaused.store(!isPaused.load());
    }
}

void AutoPianoEngine::AddTrack(const std::string& path) {
    if (!path.empty()) playlist.push_back(path);
}

void AutoPianoEngine::RemoveTrack(size_t index) {
    if (index >= playlist.size()) return;

    // 🌟 核心逻辑：修复删除导致的索引偏移
    if (index < currentTrackIdx) {
        // 如果你删除了正在播放的曲目“上面”的歌，当前播放的索引必须减 1，保持对齐
        currentTrackIdx--;
    } 
    else if (index == currentTrackIdx) {
        // 如果你直接把正在播放的歌给删了
        if (isPlaying.load()) {
            Stop(); // 先强制停下这首歌
        }
        // 删除后，后面的歌会顶上来，所以不需要 --。
        // 但如果删的是最后一首，索引必须回退防止越界
        if (currentTrackIdx > 0 && currentTrackIdx == playlist.size() - 1) {
            currentTrackIdx--;
        }
    }

    // 真正执行删除
    playlist.erase(playlist.begin() + index);
}
void AutoPianoEngine::ClearPlaylist() {
    Stop();
    playlist.clear();
    currentTrackIdx = 0;
}

void AutoPianoEngine::StartThread(const std::string& midiPath) {
    isPlaying.store(true);
    isPaused.store(false);
    currentProgress.store(0.0f);
    playThread = std::thread(&AutoPianoEngine::PlayTask, this, midiPath);
}

void AutoPianoEngine::PlayTask(std::string midiPath)
{
    std::string ansiPath = ConvertUTF8ToANSI(midiPath);
    auto actions = decodeMidi(ansiPath, 0.0);
    if (!actions.empty())
    {
        play(actions, speed, prepTime, isPlaying, isPaused, currentProgress, targetTitle);
    }
    isPlaying.store(false);
}