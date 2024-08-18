#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <core/Bot.hpp>

// incredible
#include "CheckpointObject.cpp"

using namespace geode::prelude;

class $modify(PlayLayer) {
    void resetLevel() {
        auto bot = GatoBot::get();

        // fix button release on restart
        gd::vector<PlayerButtonCommand> buttons = m_queuedButtons;

        PlayLayer::resetLevel();

        for(auto& cmd : buttons) {
            if(!cmd.m_isPush) {
                this->queueButton(static_cast<int>(cmd.m_button), cmd.m_isPush, cmd.m_isPlayer2);
            }
        }
 
        bot->onLevelReset();
    }

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        auto bot = GatoBot::get();
        bot->levelEntered(this);

        if(!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        if(bot->getStatus() != Rendering) return true;

        // Rendering stuff
        

        return true;
    }

    void loadFromCheckpoint(CheckpointObject* obj) {
        PlayLayer::loadFromCheckpoint(obj);

        if(!obj) {
            log::debug("loadFromCheckpoint called with an empty object?");
            return;
        }

        auto checkpoint = as<GBCheckpoint*>(obj);
        
        if(checkpoint->m_fields->frameState.m_frame) {
            GatoBot::get()->loadFrameState(checkpoint->m_fields->frameState);
        }
    }

    void resume() {
        // fix frame delta inaccuracy
        auto bot = GatoBot::get();

        auto state = bot->createFrameState();

        PlayLayer::resume();

        bot->loadFrameState(state, false);
    }

    void startGame() {
        PlayLayer::startGame();
        GatoBot::get()->levelStarted();
    }
};