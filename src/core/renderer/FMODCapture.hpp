#pragma once

#include <Geode/Geode.hpp>

//#include <condition_variable>

class FMODCapture {
private:
    FMOD::DSP* m_captureDSP;
    FMOD::ChannelGroup* m_masterChannelGroup; 
    //std::mutex m_threadLock;

public:
    std::vector<float> m_capturedData;
    bool m_dataProcessed = true;
    bool m_shutdown = false;
    std::mutex m_threadLock;
    std::condition_variable m_lockVar;

public:
    ~FMODCapture();

    bool setup();
    void disable();
    std::vector<float>& capture();
    void processed();

    static FMODCapture* get();
};