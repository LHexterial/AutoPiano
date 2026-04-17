#include "core/midi_parser.h"
#include "MidiFile.h"
#include <iostream>
#include <algorithm>

using namespace smf;

const char key_map_36[] = {
    ',', 'l', '.', ';', '/', 'i', '9', 'o', '0', 'p', '-', '[', // 低音区
    'z', 's', 'x', 'd', 'c', 'v', 'g', 'b', 'h', 'n', 'j', 'm', // 中音区
    'q', '2', 'w', '3', 'e', 'r', '5', 't', '6', 'y', '7', 'u'  // 高音区
};

std::vector<Action> decodeMidi(const std::string& filePath, double startTime) {
    MidiFile midifile;
    if (!midifile.read(filePath))
    {
        std::cerr << "错误的解析，文件读取失败\n";
        return {};
    }
    // 调用midifile函数
    midifile.doTimeAnalysis();
    midifile.linkNotePairs();
    std::vector<Action> action_queue;

    //遍历音轨
    for (int track = 0; track < midifile.getTrackCount(); track++)
    {
        // 遍历事件
        for (int event = 0; event < midifile[track].size(); event++)
        {
            if (midifile[track][event].isNoteOn())// 按下按键的事件
            {
                if (midifile[track][event].getChannel() == 9) continue;
                double start_t = midifile[track][event].seconds;
                double duration = midifile[track][event].getDurationInSeconds();
                double end_t = start_t + duration;
                int pitch = midifile[track][event].getKeyNumber();

                if (start_t < startTime) continue;// 小于开始时间的音符直接抹除

                start_t -= startTime;
                end_t -= startTime;

                // 解析音轨，进行八度折叠，方便映射到对应的按键
                while (pitch < 48)
                    pitch += 12;
                while (pitch > 83)
                    pitch -= 12;

                char key_char = key_map_36[pitch - 48];
                action_queue.push_back({start_t, "down", key_char});
                action_queue.push_back({end_t, "up", key_char});
            }
        }
    }
    std::sort(action_queue.begin(), action_queue.end(), [&](const Action &a, const Action &b)
              {
        if (std::abs(a.time - b.time) < 1e-7)
        {
            return a.type == "up" && b.type == "down";
        }
        return a.time < b.time; });
    return action_queue;
}