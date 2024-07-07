#include "Bot.hpp"

using namespace geode::prelude;

Result<> GatoBot::setupRenderer() {
    // TEMP
    const int displayFPS = 60;

    /*RenderParams params {
        "libx264",
        // "C:/Programming/gdmods/GatoBot/build/sex.mp4",
        // "C:/Games/Geometry Dash GEODE/Resources/Dash.mp3",
        "/storage/emulated/0/gbtestvideo.mp4",
        "",
        2400,
        1080,
        displayFPS
    };*/

    m_renderParams.m_frameFactor = m_loadedMacro.getFPS() / displayFPS;

    m_encoder = new Encoder(m_renderParams);

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

    // capture frame
    if(m_currentFrame % m_renderParams.m_frameFactor == 0) {
        m_encoder->captureFrame();
    }

    // check for errors
    if(m_encoder->getLastResult().isErr()) {
        log::error("Rendering error: {}", m_encoder->getLastResult().unwrapErr());

        (void)this->changeStatus(BotStatus::Idle);
    }
}