#include <Geode/Geode.hpp>
#include <Geode/modify/CheckpointObject.hpp>

#include "core/Bot.hpp"

using namespace geode::prelude;

// TODO: FIX (inlined)
/*
class $modify(GBCheckpoint, CheckpointObject) {
    struct Fields {
        FrameState frameState;
    };

    static CheckpointObject* create() {
        auto checkpoint = as<GBCheckpoint*>(CheckpointObject::create());
        checkpoint->m_fields->frameState = GatoBot::get()->createFrameState();

        GB_LOG("CheckpointObject::create frame {}", checkpoint->m_fields->frameState.m_frame);

        return checkpoint;
    }
};*/