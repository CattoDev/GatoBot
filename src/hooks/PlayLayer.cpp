#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "core/Bot.hpp"

// incredible
#include "CheckpointObject.cpp"

using namespace geode::prelude;

class $modify(PlayLayer) {
    void resetLevel() {
        auto bot = GatoBot::get();

        // fix button release on restart
        auto buttons = m_queuedButtons;

        PlayLayer::resetLevel();

        for(auto& cmd : buttons) {
            if(!cmd.m_isPush) {
                this->queueButton(static_cast<int>(cmd.m_button), cmd.m_isPush, cmd.m_isPlayer2);
            }
        }

        bot->onLevelReset();
    }

    void loadFromCheckpoint(CheckpointObject* obj) {
        PlayLayer::loadFromCheckpoint(obj);

        if(!obj) {
            GB_LOG("loadFromCheckpoint called with an empty object?");
            return;
        }

        auto checkpoint = as<GBCheckpoint*>(obj);
        
        if(checkpoint->m_fields->frameState.m_frame) {
            GatoBot::get()->loadFrameState(checkpoint->m_fields->frameState);
        }
    }

    void resume() {
        auto bot = GatoBot::get();

        auto state = bot->createFrameState();

        PlayLayer::resume();

        bot->loadFrameState(state, false);
    }
};