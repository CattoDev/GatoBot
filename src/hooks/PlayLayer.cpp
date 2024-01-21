#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "core/Bot.hpp"

// incredible
#include "CheckpointObject.cpp"

using namespace geode::prelude;

class $modify(PlayLayer) {
    void resetLevel() {
        // fix button release on restart
        //auto buttons = TEMP_MBO(std::vector<PlayerButtonCommand>, this, 0x2B48);
        auto buttons = m_queuedButtons;

        PlayLayer::resetLevel();

        for(auto& cmd : buttons) {
            if(!cmd.m_isPush) {
                this->queueButton(static_cast<int>(cmd.m_button), cmd.m_isPush, cmd.m_isPlayer2);
            }
        }

        GatoBot::get()->onLevelReset();
    }

    void loadFromCheckpoint(CheckpointObject* obj) {
        PlayLayer::loadFromCheckpoint(obj);

        if(!obj) {
            GB_LOG("loadFromCheckpoint called with an empty object?");
            return;
        }

        auto checkpoint = as<GBCheckpoint*>(obj);
        
        if(checkpoint->m_fields->frame) {
            GatoBot::get()->checkpointLoaded(checkpoint->m_fields->frame);
        }
    }
};