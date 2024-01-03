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

    // set player positions
    setPlayerPosition(pLayer->m_player1, CCPoint { frame.m_player1.m_posX, frame.m_player1.m_posY });
    setPlayerPosition(pLayer->m_player2, CCPoint { frame.m_player2.m_posX, frame.m_player2.m_posY });

    // queue buttons
    if(frame.m_commands.size()) {
        for(auto& cmd : frame.m_commands) {
            pLayer->queueButton(cmd.m_button, cmd.m_holding, cmd.m_rightSide);
        }
    }

    // increment
    m_currentFrame++;
}