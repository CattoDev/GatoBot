#include "Bot.hpp"

using namespace geode::prelude;

void GatoBot::updateRecording() {
    if(!this->canPerform())
        return;

    auto pLayer = this->getPlayLayer();

    PlayerData player1 {
        pLayer->m_player1->m_position.x,
        pLayer->m_player1->m_position.y,
        pLayer->m_player1->m_yVelocity
    };

    PlayerData player2 {
        pLayer->m_player2->m_position.x,
        pLayer->m_player2->m_position.y,
        pLayer->m_player1->m_yVelocity
    };

    // get queued buttons
    //auto buttons = TEMP_MBO(std::vector<PlayerButtonCommand>, pLayer, 0x2B48); // GJBaseGameLayer::processQueuedButtons
    auto buttons = pLayer->m_queuedButtons;

    // add frame
    LevelFrame frame { m_currentFrame, player1, player2, buttons };
    m_loadedMacro.addFrame(frame);

    // increment
    m_currentFrame++;
}