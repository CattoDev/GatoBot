#include "Bot.hpp"

using namespace geode::prelude;

void setPlayerPosition(PlayerObject* player, CCPoint const& pos) {
    player->m_position = pos;
    player->setPosition(pos);
}

void GatoBot::updateReplaying() {
    if(!this->canPerform())
        return;

    auto pLayer = this->getPlayLayer();

    // get frame
    const auto frame = m_loadedMacro.getFrame(m_currentFrame);

    // just debug
    if(frame.m_frame != m_currentFrame) {
        GB_LOGERR("FRAME MISMATCH {} != {}", frame.m_frame, m_currentFrame);
    }

    // set player positions
    setPlayerPosition(pLayer->m_player1, CCPoint { frame.m_player1.m_posX, frame.m_player1.m_posY });
    setPlayerPosition(pLayer->m_player2, CCPoint { frame.m_player2.m_posX, frame.m_player2.m_posY });

    // set velocity
    pLayer->m_player1->m_yVelocity = frame.m_player1.m_yVel;
    pLayer->m_player2->m_yVelocity = frame.m_player2.m_yVel;

    // queue buttons
    if(frame.m_commands.size()) {
        for(auto& cmd : frame.m_commands) {
            pLayer->queueButton(static_cast<int>(cmd.m_button), cmd.m_isPush, cmd.m_isPlayer2);
        }
    }

    // increment
    m_currentFrame++;
}