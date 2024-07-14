#include "Bot.hpp"

using namespace geode::prelude;

Result<> GatoBot::setupRenderer() {
    // TODO: error checking
    m_renderParams.m_frameFactor = m_loadedMacro.getFPS() / m_renderParams.m_fps;

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