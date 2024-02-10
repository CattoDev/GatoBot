#include <Geode/Geode.hpp>
#include <Geode/modify/CheckpointObject.hpp>

#include "core/Bot.hpp"

using namespace geode::prelude;

class $modify(GBCheckpoint, CheckpointObject) {
    FrameState frameState;

    static CheckpointObject* create() {
        auto checkpoint = as<GBCheckpoint*>(CheckpointObject::create());
        checkpoint->m_fields->frameState = GatoBot::get()->createFrameState();

        GB_LOG("CheckpointObject::create frame {}", checkpoint->m_fields->frameState.m_frame);

        return checkpoint;
    }
};