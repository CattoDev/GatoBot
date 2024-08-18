#include <Geode/Geode.hpp>
#include <Geode/modify/CheckpointObject.hpp>

#include <core/Bot.hpp>

using namespace geode::prelude;

class $modify(GBCheckpoint, CheckpointObject) {
    struct Fields {
        StepState stepState;
    };

    bool init() {
        if(!CheckpointObject::init()) return false;

        m_fields->stepState = GatoBot::get()->createStepState();

        log::debug("CheckpointObject::init step {}", m_fields->stepState.m_step);

        return true;
    }
};