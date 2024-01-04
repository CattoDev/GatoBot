#include "Bot.hpp"

using namespace geode::prelude;

void GatoBot::updateRecording() {
    if(!this->canPerform())
        return;

    auto pLayer = this->getPlayLayer();

    // get frame values for delta time
    auto unk1 = TEMP_MBO(double, pLayer, 0x2ac0);
    auto unk2 = TEMP_MBO(int, pLayer, 0x2afc);
    auto unk3 = TEMP_MBO(float, pLayer, 0x2d0);

    // get positions
    auto p1pos = TEMP_MBO(CCPoint, pLayer->m_player1, 0x81C);
    auto p2pos = TEMP_MBO(CCPoint, pLayer->m_player2, 0x81C);

    PlayerData player1 {
        p1pos.x,
        p1pos.y,
        TEMP_MBO(double, pLayer->m_player1, 0x798)
    };

    PlayerData player2 {
        p2pos.x,
        p2pos.y,
        TEMP_MBO(double, pLayer->m_player2, 0x798)
    };

    // get queued buttons
    auto buttons = TEMP_MBO(std::vector<PlayerButtonCommand>, pLayer, 0x2B48); // GJBaseGameLayer::processQueuedButtons

    // add frame
    LevelFrame frame { m_currentFrame, unk1, unk2, unk3, player1, player2, buttons };
    m_loadedMacro.addFrame(frame);

    // increment
    m_currentFrame++;
}