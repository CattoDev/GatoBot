#pragma once

#include "../Types.hpp"
#include <filesystem>

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
    LevelFrame& getLastFrame();
    void clearFramesFrom(int frame);
    int getFrameCount();
    float getDeltaTime();
    int getFPS();
    bool isEmpty();

    static PackedAction packAction(const PlayerButtonCommand& action);
    static PlayerButtonCommand unpackAction(const PackedAction& action);
    void recordingFinished();

    void saveFile(std::filesystem::path& filePath);
    void loadFile(std::filesystem::path& filePath);
};