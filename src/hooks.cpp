#include "hooks.hpp"

#include "GatoBot.hpp"
#include "GatoBotMenu.hpp"

/*
    MACROS
*/
#undef HOOKDEF
#ifdef GB_GEODE
#define HOOKDEF(returntype, funcname, selftype, ...) \
returntype GBHooks::funcname##H(selftype self, __VA_ARGS__)
#else
#define HOOKDEF(returntype, funcname, selftype, ...) \
returntype __fastcall GBHooks::funcname##H(selftype self, uintptr_t, ##__VA_ARGS__)
#endif

// replace MH_CreateHook function if using geode lol
#ifdef GB_GEODE
#define GB_CreateHook(addr, _hook, orig) \
    (void)Mod::get()->addHook(reinterpret_cast<void*>(addr), &GBHooks::_hook, GEODE_CONCAT("GatoBot -> ", GEODE_STR(_hook)), tulip::hook::TulipConvention::Thiscall); \
    orig = reinterpret_cast<decltype(orig)>(addr)
#else
#define GB_CreateHook(addr, hook, orig) \
    MH_CreateHook(reinterpret_cast<void*>(addr), reinterpret_cast<void*>(&hook), reinterpret_cast<void**>(&orig))
#endif

/*
    HOOKS
*/
HOOKDEF(bool, PlayLayer_init, gd::PlayLayer*, gd::GJGameLevel* level) {
    auto bot = GatoBot::sharedState();

    bot->statusLabel = nullptr;
    bot->exitLabel = nullptr;

    bot->resetBasicVariables(true);

    if(!PlayLayer_initO(self, level))
        return false;

    auto dir = CCDirector::sharedDirector();
    auto winSize = dir->getWinSize();
    bot->practiceCheckpoints.clear();
    bot->player1holding = false;
    bot->player2holding = false;
    bot->scheduledPause = false;
    bot->currentFrame = 0;
    bot->practiceCheckpoints.clear();
    bot->timeFromLastEsc = clock();
    
    bot->queuedBtnP1 = None;
    bot->queuedBtnP2 = None;
    bot->lastBtnP1 = None;
    bot->lastBtnP2 = None;

    // status label
    bot->statusLabel = CCLabelBMFont::create(" ", "bigFont.fnt");
    bot->statusLabel->setAnchorPoint(CCPointZero);
    bot->statusLabel->setPosition(10, 10);
    bot->statusLabel->setAlignment(CCTextAlignment::kCCTextAlignmentLeft);
    bot->statusLabel->setScale(.5);

    self->m_uiLayer->addChild(bot->statusLabel, 100);

    // exit label
    bot->exitLabel = CCLabelBMFont::create("Press Esc again to pause", "bigFont.fnt");
    bot->exitLabel->setPosition(winSize.width / 2, 75);
    bot->exitLabel->setScale(.75);
    bot->exitLabel->setOpacity(0);

    self->m_uiLayer->addChild(bot->exitLabel, 100);

    return true;
}

HOOKDEF(void, PauseLayer_customSetup, gd::PauseLayer*) {
    auto bot = GatoBot::sharedState();

    if(!bot->loadedHooks)
        GBHooks::mem_init();

    bot->currentPauseLayer = self;
    bot->gamePaused = true;

    PauseLayer_customSetupO(self);

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    auto menu = CCMenu::create();

    auto gatoBtnSpr = gd::ButtonSprite::create("GB", 0, false, "goldFont.fnt", "GJ_button_06.png", 40, 1);
    auto gatoBtn = gd::CCMenuItemSpriteExtra::create(gatoBtnSpr, self, menu_selector(GatoBotMenu::onOpen));
    menu->addChild(gatoBtn);

    gatoBtn->setPosition(winSize.width / 2 - 50, winSize.height / 2 - 40);

    self->addChild(menu, 100);
}

HOOKDEF(void, GJBaseGameLayer_pushButton, gd::GJBaseGameLayer*, int button, bool rightSide) {
    if(GatoBot::sharedState()->handleClick(self, rightSide, Pressed))
        GJBaseGameLayer_pushButtonO(self, button, rightSide);
}

HOOKDEF(void, GJBaseGameLayer_releaseButton, gd::GJBaseGameLayer*, int button, bool rightSide) {
    if(GatoBot::sharedState()->handleClick(self, rightSide, Released))
        GJBaseGameLayer_releaseButtonO(self, button, rightSide);
}

HOOKDEF(void, PlayLayer_removeLastCheckpoint, gd::PlayLayer*) {
    PlayLayer_removeLastCheckpointO(self);

    auto bot = GatoBot::sharedState();

    if(bot->practiceCheckpoints.size() > 0)
        bot->practiceCheckpoints.pop_back();
}

HOOKDEF(void, PlayLayer_resetLevel, gd::PlayLayer*) {
    auto bot = GatoBot::sharedState();

    // disable practice mode
    if(bot->isPlaybackStatus() && self->m_isPracticeMode) {
        self->togglePracticeMode(false);
        bot->practiceCheckpoints.clear();
    }

    PlayLayer_resetLevelO(self);

    if(bot->status == Recording) {
        if(bot->levelFrames.size() > 0) {
            // practice mode shenanigans
            if(self->m_isPracticeMode && bot->practiceCheckpoints.size() > 0) {
                auto checkpoint = bot->practiceCheckpoints.back();

                // set frame
                bot->currentFrame = checkpoint.frame.frame;

                // respawn player with data
                checkpoint.frame.player1.applyToPlayer(self->m_pPlayer1);
                checkpoint.frame.player2.applyToPlayer(self->m_pPlayer2);
            }
            else bot->currentFrame = 0;
            
            // remove all clicks and frames after checkpoint
            if(self->m_isPracticeMode && bot->currentFrame > 0) {
                bot->levelFrames.resize(bot->currentFrame);
            }
            else {
                bot->levelFrames.clear();
            }
            
            // jump?
            if(bot->levelFrames.back().player1.isHolding != MBO(bool, self->m_pPlayer1, 0x611)) {
                if(MBO(bool, self->m_pPlayer1, 0x611)) {
                    bot->levelFrames.back().player1.action = Pressed;
                    bot->levelFrames.back().player1.isHolding = true;
                }
                else {
                    bot->levelFrames.back().player1.action = Released;
                    bot->levelFrames.back().player1.isHolding = false;
                }
            }

            if(bot->levelFrames.back().player2.isHolding != MBO(bool, self->m_pPlayer2, 0x611) && (MBO(bool, self, 0x2A9) && !MBO(bool, self->m_pPlayer2, 0x611) && MBO(bool, self->m_pLevelSettings, 0xFA))) {
                if(MBO(bool, self->m_pPlayer2, 0x611)) {
                    bot->levelFrames.back().player2.action = Pressed;
                    bot->levelFrames.back().player2.isHolding = true;
                }
                else {
                    bot->levelFrames.back().player2.action = Released;
                    bot->levelFrames.back().player2.isHolding = false;
                }
            }

            bot->queuedBtnP1 = None;
            bot->queuedBtnP2 = None;

            bot->lastBtnP1 = bot->levelFrames.back().player1.action;
            bot->lastBtnP2 = bot->levelFrames.back().player2.action;
        }
    }

    if(!self->m_isPracticeMode && !bot->isDelayedRendering()) {
        bot->currentFrame = 0;
        bot->timeFromStart = 0;

        bot->clickSounds.clear();

        // update music offset
        bot->currentMusicOffset = bot->getTimeForXPos(self) + MBO(float, self->m_pLevelSettings, 0xFC); // timeForXPos + songOffset
    }
}

HOOKDEF(void, PlayLayer_destroyPlayer, gd::PlayLayer*, gd::PlayerObject* player, gd::GameObject* obj) {
    PlayLayer_destroyPlayerO(self, player, obj);

    auto bot = GatoBot::sharedState();

    if((bot->isPlaybackStatus()) && MBO(bool, self, 0x39C))
        bot->resetBasicVariables(true);
}

HOOKDEF(void, PlayLayer_levelComplete, gd::PlayLayer*) {
    GatoBot::sharedState()->resetBasicVariables(false);
    
    PlayLayer_levelCompleteO(self);
}

HOOKDEF(void, PlayLayer_onQuit, gd::PlayLayer*) {
    GatoBot::sharedState()->resetBasicVariables(true);
    
    PlayLayer_onQuitO(self);
}

HOOKDEF(void, PlayLayer_pauseGame, gd::PlayLayer*, bool idk) {
    auto bot = GatoBot::sharedState();

    if(!bot->scheduledPause && bot->status == Rendering) {
        clock_t curEscTime = clock();

        if(curEscTime - bot->timeFromLastEsc < 1000) { // 1 sec timeout
            PlayLayer_pauseGameO(self, idk);
        }
        
        bot->timeFromLastEsc = curEscTime;
    }
    else PlayLayer_pauseGameO(self, idk);
}

HOOKDEF(void, CCScheduler_update, CCScheduler*, float dt) {
    auto bot = GatoBot::sharedState();

    auto pLayer = gd::PlayLayer::get();
    bot->inPlayLayer = pLayer != nullptr;
    if(bot->inPlayLayer)
        bot->gamePaused = MBO(bool, pLayer, 0x52F);

    if(bot->updatePlayLayer(dt))
        CCScheduler_updateO(self, dt);

    bot->checkErrors();
}

HOOKDEF(void, CCDirector_drawScene, CCDirector*) {
    auto bot = GatoBot::sharedState();

    if(bot->status == Rendering && bot->settings.getOption<bool>("fastRender") && !bot->gamePaused) {
        if(!self->isPaused())
            self->getScheduler()->update(0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        kmGLPushMatrix();

        // draw labels
        if(bot->statusLabel != nullptr) bot->statusLabel->visit();
        if(bot->exitLabel != nullptr) bot->exitLabel->visit();

        kmGLPopMatrix();

        // swap buffers
        auto glView = self->getOpenGLView();

        if(glView != nullptr)
            glView->swapBuffers();
    }
    else CCDirector_drawSceneO(self);
}

/*HOOKDEF(void, CCDisplayLinkDirector_mainLoop, CCDisplayLinkDirector*) {
    auto bot = GatoBot::sharedState();

    if(bot->status == Rendering) {
        bot->renderingLoop([&] { CCDisplayLinkDirector_mainLoopO(self); });
    }

    CCDisplayLinkDirector_mainLoopO(self);
}*/

// setup
void GBHooks::mem_init() {
    auto bot = GatoBot::sharedState();

    // GJBaseGameLayer::pushButton
    GB_CreateHook(
        gd::base + 0x111500,
        GJBaseGameLayer_pushButtonH,
        GJBaseGameLayer_pushButtonO
    );

    // GJBaseGameLayer::releaseButton
    GB_CreateHook(
        gd::base + 0x111660,
        GJBaseGameLayer_releaseButtonH,
        GJBaseGameLayer_releaseButtonO
    );

    // PlayLayer::removeLastCheckpoint
    GB_CreateHook(
        gd::base + 0x20b830,
        PlayLayer_removeLastCheckpointH,
        PlayLayer_removeLastCheckpointO
    );

    // PlayLayer::resetLevel
    GB_CreateHook(
        gd::base + 0x20bf00,
        PlayLayer_resetLevelH,
        PlayLayer_resetLevelO
    );

    // PlayLayer::destroyPlayer
    GB_CreateHook(
        gd::base + 0x20a1a0,
        PlayLayer_destroyPlayerH,
        PlayLayer_destroyPlayerO
    );

    // PlayLayer::onComplete
    GB_CreateHook(
        gd::base + 0x1fd3d0,
        PlayLayer_levelCompleteH,
        PlayLayer_levelCompleteO
    );

    // PlayLayer::onQuit
    GB_CreateHook(
        gd::base + 0x20d810,
        PlayLayer_onQuitH,
        PlayLayer_onQuitO
    );

    // PlayLayer::pauseGame
    GB_CreateHook(
        gd::base + 0x20d3c0,
        PlayLayer_pauseGameH,
        PlayLayer_pauseGameO
    );

    // CCScheduler::update
    GB_CreateHook(
        bot->updateHookAddr,
        CCScheduler_updateH,
        CCScheduler_updateO
    );

    // CCDirector::drawScene
    GB_CreateHook(
        GetProcAddress(bot->cocosBaseAddr, "?drawScene@CCDirector@cocos2d@@QAEXXZ"),
        CCDirector_drawSceneH,
        CCDirector_drawSceneO
    );

    // CCDisplayLinkDirector::mainLoop
    /*GB_CreateHook(
        reinterpret_cast<size_t>(bot->cocosBaseAddr) + 0xffb10,
        CCDisplayLinkDirector_mainLoopH,
        CCDisplayLinkDirector_mainLoopO
    );*/

    #ifndef GB_GEODE
    MH_EnableHook(MH_ALL_HOOKS);
    #endif

    bot->loadedHooks = true;
    bot->botStatusChanged();
}

void GatoBot::setupBasicHooks() {
    using namespace GBHooks;

    // PlayLayer::init
    GB_CreateHook(
        gd::base + 0x1fb780,
        PlayLayer_initH,
        PlayLayer_initO
    ); 

    // PauseLayer::customSetup
    GB_CreateHook(
        gd::base + 0x1e4620,
        PauseLayer_customSetupH,
        PauseLayer_customSetupO
    );

    #ifdef GB_GEODE
    GBHooks::mem_init();
    #else
    MH_EnableHook(MH_ALL_HOOKS);
    #endif
}