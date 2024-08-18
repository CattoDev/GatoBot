#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

#include <core/Bot.hpp>

using namespace geode::prelude;

class $modify(GJBaseGameLayer) {
    void processCommands(float pd) {
        GatoBot::get()->updateBot();
        GJBaseGameLayer::processCommands(pd);
    }
};