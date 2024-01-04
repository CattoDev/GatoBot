#include "Bot.hpp"

using namespace geode::prelude;

void setPlayerPosition(PlayerObject* player, CCPoint const& pos) {
    //GB_LOG("{} vs {}", pos.x, player->getPositionX());
    
    TEMP_MBO(CCPoint, player, 0x81C) = pos;
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

    // set delta time values
    TEMP_MBO(double, pLayer, 0x2ac0) = frame.m_unk1;
    TEMP_MBO(int, pLayer, 0x2afc) = frame.m_unk2;
    TEMP_MBO(float, pLayer, 0x2d0) = frame.m_unk3;

    // set player positions
    setPlayerPosition(pLayer->m_player1, CCPoint { frame.m_player1.m_posX, frame.m_player1.m_posY });
    setPlayerPosition(pLayer->m_player2, CCPoint { frame.m_player2.m_posX, frame.m_player2.m_posY });

    // set velocity
    TEMP_MBO(double, pLayer->m_player1, 0x798) = frame.m_player1.m_yVel;
    TEMP_MBO(double, pLayer->m_player2, 0x798) = frame.m_player2.m_yVel;

    // queue buttons
    if(frame.m_commands.size()) {
        for(auto& cmd : frame.m_commands) {
            GB_LOG("{}: {} {} {}", frame.m_frame, cmd.m_button, cmd.m_holding, cmd.m_rightSide);

            pLayer->queueButton(cmd.m_button, cmd.m_holding, cmd.m_rightSide);
        }
    }

    // increment
    m_currentFrame++;
}