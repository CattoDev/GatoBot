#pragma once

#include "../Types.hpp"

class Macro {
private:
    int m_fps;
    std::vector<LevelFrame> m_allFrames;

public:
    using PackedAction = unsigned char; // 1 byte

public:
    void prepareMacro(int fps);

    void addFrame(LevelFrame& frame);
    LevelFrame& getFrame(int frame);
    void clearFramesFrom(int frame);
    int getFrameCount();
    float getDeltaTime();
    int getFPS();
    bool isEmpty();

    static PackedAction packAction(const PlayerButtonCommand& action);
    static PlayerButtonCommand unpackAction(const PackedAction& action);
    void recordingFinished();

    void saveFile(const std::string& filePath);
    void loadFile(const std::string& filePath);
};