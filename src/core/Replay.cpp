#include "Bot.hpp"

using namespace geode::prelude;

void GatoBot::updateReplaying() {
    if(!this->canPerform())
        return;

    auto pLayer = this->getPlayLayer();

    // get frame
    const auto frame = m_loadedMacro.getFrame(m_currentFrame);

    // just debug
    if(frame.m_frame != m_currentFrame) {
        log::error("FRAME MISMATCH {} != {}", frame.m_frame, m_currentFrame);
    }

    //GB_LOG("{} == {}", pLayer->m_player1->m_position.x, frame.m_player1.m_posX);
    //if(pLayer->m_player1->m_position.x != frame.m_frameState.m_player1.m_posX || pLayer->m_player1->m_position.y != frame.m_frameState.m_player1.m_posY) {
    //    log::error("PLAYER POSITION INCONSISTENT ({}, {}) != ({}, {})", pLayer->m_player1->m_position.x, pLayer->m_player1->m_position.y, frame.m_frameState.m_player1.m_posX, frame.m_frameState.m_player1.m_posY);
    //}

    // queue buttons
    if(frame.m_commands.size()) {
        for(const PlayerButtonCommand& cmd : frame.m_commands) {
            pLayer->queueButton(static_cast<int>(cmd.m_button), cmd.m_isPush, cmd.m_isPlayer2);
        }
    }

    // increment
    m_currentFrame++;
}