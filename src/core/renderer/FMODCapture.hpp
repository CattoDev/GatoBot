#pragma once

#include <Geode/Geode.hpp>

class FMODCapture {
private:
    FMOD::DSP* m_captureDSP;
    FMOD::ChannelGroup* m_masterChannelGroup; 

public:
    std::vector<float> m_capturedData;
    bool m_started = false;
    bool m_dataProcessed = true;
    bool m_shutdown = false;
    std::mutex m_threadLock;
    std::condition_variable m_lockVar;

public:
    ~FMODCapture();

    void setup();
    void begin();
    void disable();
    std::vector<float>& capture();
    void processed();

    static FMODCapture* get();
};