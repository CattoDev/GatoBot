#include "Bot.hpp"

using namespace geode::prelude;

void GatoBot::updateRecording() {
    if(!this->canPerform())
        return;

    //geode::log::debug("Saving step {}", m_currentStep);

    auto pLayer = this->getPlayLayer();

    // add step
    auto step = this->createStepState();

    // add queued commands
    if(m_queuedPlayerCommands.size() > 0) {
        for(auto& cmd : m_queuedPlayerCommands) {
            bool add = true;
            for(auto& curCmd : step.m_commands) {
                if(curCmd.m_button == cmd.m_button && curCmd.m_isPlayer2 == cmd.m_isPlayer2 && curCmd.m_isPush != cmd.m_isPush) {
                    add = false;
                    break;
                }
            }

            if(add) step.m_commands.push_back(cmd);
        }
    }

    m_loadedMacro.addStep(step);

    m_currentStep++;
}

StepState GatoBot::createStepState() {
    StepState state;

    auto pLayer = this->getPlayLayer();

    state.m_step = m_currentStep;

    state.m_player1 = PlayerData {
        pLayer->m_player1->m_position.x,
        pLayer->m_player1->m_position.y,
        pLayer->m_player1->m_yVelocity
    };

    state.m_player2 = PlayerData {
        pLayer->m_player2->m_position.x,
        pLayer->m_player2->m_position.y,
        pLayer->m_player2->m_yVelocity
    };

    // get queued buttons
    gd::vector<PlayerButtonCommand> _buttonsRaw = pLayer->m_queuedButtons;

    // (convert to std::vector)
    for(PlayerButtonCommand& button : _buttonsRaw) {
        state.m_commands.push_back(button);
    }
    
    return std::move(state);
}