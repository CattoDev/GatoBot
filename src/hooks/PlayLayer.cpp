#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "core/Bot.hpp"

// incredible
#include "CheckpointObject.cpp"

using namespace geode::prelude;

class $modify(PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();

        GatoBot::get()->onLevelReset();
    }

    void loadFromCheckpoint(CheckpointObject* obj) {
        PlayLayer::loadFromCheckpoint(obj);

        if(!obj) return;

        auto checkpoint = as<GBCheckpoint*>(obj);
        
        if(checkpoint->m_fields->frame) {
            GatoBot::get()->checkpointLoaded(checkpoint->m_fields->frame);
        }
    }
};