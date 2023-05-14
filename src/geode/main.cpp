#include "../hooks.hpp"
#include "../GatoBot.hpp"

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/CCDirector.hpp>

#define as reinterpret_cast

// hooks
class $modify(PlayLayer) {
    bool init(GJGameLevel* level) {
        return GBHooks::PlayLayer_initH(as<gd::PlayLayer*>(this), 0, as<gd::GJGameLevel*>(level));
    }

    void update(float dt) {
        GBHooks::PlayLayer_updateH(as<gd::PlayLayer*>(this), 0, dt);
    }

    void removeLastCheckpoint() {
        GBHooks::PlayLayer_removeLastCheckpointH(as<gd::PlayLayer*>(this), 0);
    }

    void resetLevel() {
        GBHooks::PlayLayer_resetLevelH(as<gd::PlayLayer*>(this), 0);
    }

    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        GBHooks::PlayLayer_destroyPlayerH(as<gd::PlayLayer*>(this), 0, as<gd::PlayerObject*>(player), as<gd::GameObject*>(obj));
    }

    void levelComplete() {
        GBHooks::PlayLayer_levelCompleteH(as<gd::PlayLayer*>(this), 0);
    }

    void onQuit() {
        GBHooks::PlayLayer_onQuitH(as<gd::PlayLayer*>(this), 0);
    }

    // once
    static void onModify(auto& self) {
        GatoBot::sharedState()->setupBot();
    }
};

class $modify(PauseLayer) {
    void customSetup() {
        GBHooks::PauseLayer_customSetupH(as<gd::PauseLayer*>(this), 0);
    }
};

class $modify(GJBaseGameLayer) {
    void pushButton(int button, bool rightSide) {
        GBHooks::GJBaseGameLayer_pushButtonH(as<gd::GJBaseGameLayer*>(this), 0, button, rightSide);
    }

    void releaseButton(int button, bool rightSide) {
        GBHooks::GJBaseGameLayer_releaseButtonH(as<gd::GJBaseGameLayer*>(this), 0, button, rightSide);
    }
};

class $modify(CCScheduler) {
    void update(float dt) {
        GBHooks::CCScheduler_updateH(this, 0, dt);
    }
};

class $modify(CCDirector) {
    void drawScene() {
        GBHooks::CCDirector_drawSceneH(this, 0);
    }
};