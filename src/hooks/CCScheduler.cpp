#include <Geode/Geode.hpp>
#include <Geode/modify/CCScheduler.hpp>

#include "core/Bot.hpp"

using namespace geode::prelude;

class $modify(CCScheduler) {
    void update(float dt) {
        auto bot = GatoBot::get();

        if(bot->updatePlayLayer(dt)) {
            CCScheduler::update(dt);
        }

        //CCScheduler::update(dt);
    }
};