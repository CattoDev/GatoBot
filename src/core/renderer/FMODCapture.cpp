/*
    https://qa.fmod.com/t/c-core-api-how-to-get-pcm-data-after-dsp-processing/16957/6

    TODO: error checking
*/

// fuck is android doing lmaooo
#ifdef GEODE_ANDROID
    #define strncpy_s strncpy
#endif

#include "FMODCapture.hpp"

using namespace geode::prelude;

FMODCapture* g_captureInstance = nullptr;

FMODCapture::~FMODCapture() {
    g_captureInstance = nullptr;
}

FMOD_RESULT F_CALLBACK CaptureDSPCallback(FMOD_DSP_STATE *dspState, float *inBuffer, float *outBuffer, unsigned int length, int inChannels, int *outChannels) {
    auto cap = FMODCapture::get();

    // wait until data is processed
    std::unique_lock lk(cap->m_threadLock);
    cap->m_lockVar.wait(lk, [cap] { return cap->m_dataProcessed; });

    // copy data
    const int channels = *outChannels;
    const int dataSize = length * channels * sizeof(float);

    memcpy(cap->m_capturedData.data(), inBuffer, dataSize);
    
    // unlock main thread
    cap->m_dataProcessed = false;

    lk.unlock();
    cap->m_lockVar.notify_one();

    return FMOD_OK;
}

bool FMODCapture::setup() {
    FMOD::System* system = FMODAudioEngine::sharedEngine()->m_system;

    // initialize buffer
    m_capturedData.resize(1024, 0);

    // create custom DSP
    FMOD_DSP_DESCRIPTION dspDesc;
    memset(&dspDesc, 0, sizeof(dspDesc));
    strncpy_s(dspDesc.name, "Capture DSP", sizeof(dspDesc.name));
    dspDesc.numinputbuffers = 1;
    dspDesc.numoutputbuffers = 1;
    dspDesc.read = CaptureDSPCallback;
    system->createDSP(&dspDesc, &m_captureDSP);

    // add DSP
    system->getMasterChannelGroup(&m_masterChannelGroup);
    m_masterChannelGroup->addDSP(0, m_captureDSP);

    return true;
}

void FMODCapture::disable() {
    m_masterChannelGroup->removeDSP(m_captureDSP);

    {
        const std::lock_guard<std::mutex> l(m_threadLock);
        m_dataProcessed = true;
        m_shutdown = true;
    }
    m_lockVar.notify_one();

    m_captureDSP->release();
}

std::vector<float>& FMODCapture::capture() {
    {
        // wait for data
        std::unique_lock lk(m_threadLock);
        m_lockVar.wait(lk, [this] { return !m_dataProcessed; });
    }

    return m_capturedData;
}

void FMODCapture::processed() {
    {
        std::unique_lock lk(m_threadLock);
        m_dataProcessed = true;
    }
    m_lockVar.notify_one();
}

FMODCapture* FMODCapture::get() {
    if(!g_captureInstance) {
        g_captureInstance = new FMODCapture();
    }

    return g_captureInstance;
}