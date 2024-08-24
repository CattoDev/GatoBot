// temp
#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

#include <core/Bot.hpp>

using namespace geode::prelude;

$on_mod(Loaded) {
    // initialize GatoBot
    GatoBot::get();
}