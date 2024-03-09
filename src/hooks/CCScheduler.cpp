#include <Geode/Geode.hpp>
#include <Geode/modify/CCScheduler.hpp>

#include "core/Bot.hpp"

using namespace geode::prelude;

class $modify(CCScheduler) {
    void update(float dt) {
        auto bot = GatoBot::get();

        bot->updatePlayLayer(dt);
        CCScheduler::update(dt);

        //log::debug("delta time {} [{} FPS]", dt, 1.f / dt);
    }
};