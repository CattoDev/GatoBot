// temp
#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

#include <core/Bot.hpp>

using namespace geode::prelude;

class $modify(MenuLayer) {
    bool init() {
        if(!MenuLayer::init()) return false;
        
        GatoBot::get()->updateHooks();

        return true;
    }
};