#include "Bot.hpp"

using namespace geode::prelude;

void GatoBot::setupRenderer() {
    //m_encoder = new Encoder(600, 800, "libx264");
    // TEMP
    RenderParams params {
        "libx264",
        "C:/Programming/gdmods/GatoBot/build/sex.mp4",
        "C:/Games/Geometry Dash GEODE/Resources/Dash.mp3",
        1600,
        900,
        75
    };

    m_encoder = new Encoder(params);

    if(m_encoder->getLastResult().isErr()) {
        geode::log::error("{}", m_encoder->getLastResult().unwrapErr());
    }

    //CC_SAFE_DELETE(m_encoder);
}

void GatoBot::updateRendering() {
    // update replay
    this->updateReplaying();

    // capture frame
    m_encoder->captureCurrentFrame();

    // check for errors
    if(m_encoder->getLastResult().isErr()) {
        log::error("Rendering error: {}", m_encoder->getLastResult().unwrapErr());
        this->changeStatus(BotStatus::Idle);
    }
}