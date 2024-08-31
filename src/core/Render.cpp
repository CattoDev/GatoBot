#include "Bot.hpp"

using namespace geode::prelude;

Result<> GatoBot::setupRenderer() {
    Result<> result;

    // video FPS can NOT be higher than TPS
    if(m_renderParams.m_fps > m_loadedMacro.getTPS()) {
        result = geode::Err("Video FPS must be below or equal to the macro TPS!");

        return result;
    }

    m_renderParams.m_timeSinceFrameRender = 1.f; // capture 1st frame
    m_renderParams.m_spf = 1.f / static_cast<float>(m_renderParams.m_fps);
    m_renderParams.m_spt = 1.f / static_cast<float>(m_loadedMacro.getTPS());
    m_renderParams.m_totalFrames = static_cast<int>(std::roundf(static_cast<float>(m_loadedMacro.getStepCount()) * m_renderParams.m_spt * static_cast<float>(m_renderParams.m_fps)));

    // prepare audio
    this->copyVolume();

    auto fmod = FMODAudioEngine::sharedEngine();
    fmod->m_system->getOutput(&m_renderParams.m_FMODOutputType);
    fmod->m_system->setOutput(FMOD_OUTPUTTYPE_NOSOUND);

    // setup Encoder
    m_encoder = std::make_unique<Encoder>(&m_renderParams);

    result = m_encoder->getLastResult();

    // failed to create Encoder
    if(result.isErr()) {
        geode::log::error("{}", m_encoder->getLastResult().unwrapErr());

        // free Encoder
        m_encoder = nullptr;
    }

    log::debug("GatoBot::setupRenderer finished");

    return result;
}

void GatoBot::updateRendering() {
    // failsafe
    this->setVolume(m_renderParams.m_audioVolume);

    // update replay
    this->updateReplaying();

    // fuck you lasagnatester

    // render frame
    if(this->canPerform()) {
        const float spf = m_renderParams.m_spf;

        if(m_renderParams.m_timeSinceFrameRender >= spf) {
            while(m_renderParams.m_timeSinceFrameRender >= spf) m_renderParams.m_timeSinceFrameRender -= spf;
            
            m_encoder->captureFrame();
        }

        m_renderParams.m_timeSinceFrameRender += m_renderParams.m_spt;
    }

    // check for errors
    if(m_encoder->getLastResult().isErr()) {
        log::error("Rendering error: {}", m_encoder->getLastResult().unwrapErr());

        (void)this->changeStatus(BotStatus::Idle);
    }
}