#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <core/Bot.hpp>

// incredible
#include "CheckpointObject.cpp"

using namespace geode::prelude;

class $modify(PlayLayer) {
    void resetLevel() {
        auto bot = GatoBot::get();

        bot->clearQueuedCommands();

        // fix button release on restart
        gd::vector<PlayerButtonCommand> buttons = m_queuedButtons;

        PlayLayer::resetLevel();

        for(auto& cmd : buttons) {
            if(!cmd.m_isPush) {
                this->queueButton(static_cast<int>(cmd.m_button), cmd.m_isPush, cmd.m_isPlayer2);
            }
        }
 
        bot->onLevelReset();
    }

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        auto bot = GatoBot::get();
        bot->levelEntered(this);

        if(!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        if(bot->getStatus() != BotStatus::Rendering) return true;

        // Rendering stuff
        if(auto enc = bot->getEncoder()) {
            enc->setupInfoLabels();
        }

        return true;
    }

    void loadFromCheckpoint(CheckpointObject* obj) {
        PlayLayer::loadFromCheckpoint(obj);

        if(!obj) {
            log::debug("loadFromCheckpoint called with an empty object?");
            return;
        }

        auto checkpoint = as<GBCheckpoint*>(obj);
        
        if(checkpoint->m_fields->stepState.m_step > 0) {
            auto bot = GatoBot::get();

            bot->loadStepState(checkpoint->m_fields->stepState);
        
            // fix an issue where the button would still be held
            // after respawning from a checkpoint after releasing
            // said button
            bot->queuePlayerCommand(PlayerButtonCommand { PlayerButton::Jump, false, false });
            bot->queuePlayerCommand(PlayerButtonCommand { PlayerButton::Jump, false, true });

            // platformer specific
            if(m_uiLayer->m_inPlatformer) {
                bot->queuePlayerCommand(PlayerButtonCommand { PlayerButton::Left, false, false });
                bot->queuePlayerCommand(PlayerButtonCommand { PlayerButton::Left, false, true });
                bot->queuePlayerCommand(PlayerButtonCommand { PlayerButton::Right, false, false });
                bot->queuePlayerCommand(PlayerButtonCommand { PlayerButton::Right, false, true });
            }
        }
    }

    void startGame() {
        PlayLayer::startGame();
        GatoBot::get()->levelStarted();
    }

    void onQuit() {
        (void)GatoBot::get()->changeStatus(BotStatus::Idle);
        PlayLayer::onQuit();
    }
};