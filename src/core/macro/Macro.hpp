#pragma once

#include "../Types.hpp"

class Macro {
private:
    int m_fps;
    std::vector<LevelFrame> m_allFrames;

public:
    void prepareMacro(int fps);

    void addFrame(LevelFrame& frame);
    LevelFrame& getFrame(int frame);
    void clearFramesFrom(int frame);
    int getFrameCount();
    float getDeltaTime();

    void recordingFinished();

    void saveFile(std::string filePath);
    void loadFile(std::string filePath);
};