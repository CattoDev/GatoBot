#include "Bot.hpp"

using namespace geode::prelude;

Result<> GatoBot::setupRenderer() {
    // TODO: error checking
    m_renderParams.m_frameFactor = m_loadedMacro.getFPS() / m_renderParams.m_fps;

    // prepare audio
    this->copyVolume();

    auto fmod = FMODAudioEngine::sharedEngine();
    fmod->m_system->getOutput(&m_renderParams.m_FMODOutputType);
    fmod->m_system->setOutput(FMOD_OUTPUTTYPE_NOSOUND);

    // setup Encoder
    m_encoder = new Encoder(&m_renderParams);

    auto result = m_encoder->getLastResult();

    // failed to create Encoder
    if(result.isErr()) {
        geode::log::error("{}", m_encoder->getLastResult().unwrapErr());

        // free Encoder
        CC_SAFE_DELETE(m_encoder);
    }

    return result;
}

void GatoBot::updateRendering() {
    // failsafe
    this->setVolume(m_renderParams.m_audioVolume);

    // update replay
    this->updateReplaying();

    // prepare frame for rendering
    if(this->canPerform() && m_currentFrame % m_renderParams.m_frameFactor == 0) {
        m_encoder->captureFrame();
    }

    // check for errors
    if(m_encoder->getLastResult().isErr()) {
        log::error("Rendering error: {}", m_encoder->getLastResult().unwrapErr());

        (void)this->changeStatus(BotStatus::Idle);
    }
}