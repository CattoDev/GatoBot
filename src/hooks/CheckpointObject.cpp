#include <Geode/Geode.hpp>
#include <Geode/modify/CheckpointObject.hpp>

#include "core/Bot.hpp"

using namespace geode::prelude;

class $modify(GBCheckpoint, CheckpointObject) {
    int frame;

    static CheckpointObject* create() {
        auto checkpoint = as<GBCheckpoint*>(CheckpointObject::create());
        checkpoint->m_fields->frame = GatoBot::get()->getCurrentFrameNum();

        GB_LOG("CheckpointObject::create frame {}", checkpoint->m_fields->frame);

        return checkpoint;
    }
};