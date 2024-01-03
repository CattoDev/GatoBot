#include "Bot.hpp"

using namespace geode::prelude;

void GatoBot::updateRecording() {
    if(!this->canPerform())
        return;

    auto pLayer = this->getPlayLayer();

    //auto p1 = TEMP_MBO(CCSpritePlus*, pLayer, 0x870); // player1 (GJBaseGameLayer::createPlayer)
    //auto p2 = TEMP_MBO(CCSpritePlus*, pLayer, 0x874); // player2 (GJBaseGameLayer::createPlayer)

    auto p1pos = TEMP_MBO(CCPoint, pLayer->m_player1, 0x81C);
    auto p2pos = TEMP_MBO(CCPoint, pLayer->m_player2, 0x81C);

    PlayerData player1 {
        p1pos.x,
        p1pos.y
    };

    PlayerData player2 {
        p2pos.x,
        p2pos.y
    };

    // get queued buttons
    auto buttons = TEMP_MBO(std::vector<PlayerButtonCommand>, pLayer, 0x2B48); // GJBaseGameLayer::processQueuedButtons

    // add frame
    LevelFrame frame { m_currentFrame, player1, player2, buttons };
    m_loadedMacro.addFrame(frame);

    // increment
    m_currentFrame++;
}