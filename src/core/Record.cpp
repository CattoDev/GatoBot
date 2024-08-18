#include "Bot.hpp"

using namespace geode::prelude;

void GatoBot::updateRecording() {
    if(!this->canPerform())
        return;

    auto pLayer = this->getPlayLayer();

    // get queued buttons
    gd::vector<PlayerButtonCommand> _buttonsRaw = pLayer->m_queuedButtons;

    // (convert to std::vector)
    std::vector<PlayerButtonCommand> buttons;
    for(PlayerButtonCommand& button : _buttonsRaw) {
        buttons.push_back(button);
    }

    // add frame
    LevelFrame frame { m_currentFrame, buttons, this->createFrameState() };
    m_loadedMacro.addFrame(frame);

    // increment
    m_currentFrame++;
}