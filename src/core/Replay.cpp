#include "Bot.hpp"

using namespace geode::prelude;

void GatoBot::updateReplaying() {
    if(!this->canPerform())
        return;

    //geode::log::debug("Playing step {}", m_currentStep);

    auto pLayer = this->getPlayLayer();

    // get frame
    StepState& step = m_loadedMacro.getStep(m_currentStep);

    // just debugging stuff
    {
        if((step.m_player1.m_posX != 0 && step.m_player1.m_posY != 0) && (pLayer->m_player1->m_position.x != step.m_player1.m_posX || pLayer->m_player1->m_position.y != step.m_player1.m_posY)) {
            log::error("PLAYER POSITION INCONSISTENT ({}, {}) != ({}, {})", pLayer->m_player1->m_position.x, pLayer->m_player1->m_position.y, step.m_player1.m_posX, step.m_player1.m_posY);
        }
    }

    // queue buttons
    if(step.m_commands.size()) {
        for(const PlayerButtonCommand& cmd : step.m_commands) {
            pLayer->queueButton(static_cast<int>(cmd.m_button), cmd.m_isPush, cmd.m_isPlayer2);
        }
    }

    // increment
    m_currentStep++;
}