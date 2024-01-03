#include "Macro.hpp"

#include "Debug.hpp"

void Macro::prepareMacro(int fps) {
    m_fps = fps;

    // allocate frames
    m_allFrames.reserve(100000);
}

void Macro::addFrame(LevelFrame& frame) {
    m_allFrames.push_back(std::move(frame));
}

LevelFrame& Macro::getFrame(int frame) {
    return m_allFrames[frame];
}

void Macro::clearFramesAfter(int frame) {
    GB_LOG("Macro::clearFramesAfter {} ({})", frame, m_allFrames.size());

    frame = std::min(frame, this->getFrameCount());
    m_allFrames.erase(m_allFrames.begin() + frame, m_allFrames.end());
}

int Macro::getFrameCount() {
    return static_cast<int>(m_allFrames.size());
}

float Macro::getDeltaTime() {
    return 1.f / static_cast<float>(m_fps);
}