#include "hooks.hpp"

#include "GatoBot.hpp"
#include "GatoBotMenu.hpp"
#include "LoadingCircle.hpp"

/*
    MACROS
*/
#undef HOOKDEF
#define HOOKDEF(returntype, funcname, selftype, ...) \
returntype __fastcall GBHooks::funcname##H(selftype self, uintptr_t, ##__VA_ARGS__)

// replace MH_CreateHook function if using geode lol
#ifdef GB_GEODE
#define GB_CreateHook(addr, hook, orig) \
    orig = reinterpret_cast<decltype(orig)>(addr)
#else
#define GB_CreateHook(addr, hook, orig) \
    MH_CreateHook(reinterpret_cast<void*>(addr), reinterpret_cast<void*>(&hook), reinterpret_cast<void**>(&orig))
#endif

/*
    HOOKS
*/
HOOKDEF(bool, PlayLayer_init, gd::PlayLayer*, gd::GJGameLevel* level) {
    if(!PlayLayer_initO(self, level)) return false;

    auto bot = GatoBot::sharedState();
    auto dir = CCDirector::sharedDirector();
    auto winSize = dir->getOpenGLView()->getFrameSize();
    bot->practiceCheckpoints.clear();
    bot->player1holding = false;
    bot->player2holding = false;
    bot->currentFrame = 0;
    bot->practiceCheckpoints.clear();
    
    bot->queuedBtnP1 = None;
    bot->queuedBtnP2 = None;
    bot->lastBtnP1 = None;
    bot->lastBtnP2 = None;

    bot->resetBasicVariables(true);

    // status label
    bot->statusLabel = CCLabelBMFont::create(" ", "bigFont.fnt");
    bot->statusLabel->setAnchorPoint(CCPointZero);
    bot->statusLabel->setPosition(10, 10);
    bot->statusLabel->setAlignment(CCTextAlignment::kCCTextAlignmentLeft);
    bot->statusLabel->setScale(.5);

    self->m_uiLayer->addChild(bot->statusLabel, 100);

    return true;
}

HOOKDEF(void, PauseLayer_customSetup, gd::PauseLayer*) {
    PauseLayer_customSetupO(self);

    auto bot = GatoBot::sharedState();

    if(!bot->settings.loadedHooks)
        GBHooks::mem_init();

    bot->currentPauseLayer = self;
    bot->gamePaused = true;

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    auto menu = CCMenu::create();

    auto gatoBtnSpr = gd::ButtonSprite::create("GB", 0, false, "goldFont.fnt", "GJ_button_06.png", 40, 1);
    auto gatoBtn = gd::CCMenuItemSpriteExtra::create(gatoBtnSpr, self, menu_selector(GatoBotMenu::onOpen));
    menu->addChild(gatoBtn);

    gatoBtn->setPosition(winSize.width / 2 - 50, winSize.height / 2 - 40);

    self->addChild(menu, 100);
}

HOOKDEF(void, PlayLayer_update, gd::PlayLayer*, float dt) {
    auto bot = GatoBot::sharedState();

    bot->gamePaused = MBO(bool, self, 0x52F);

    if(bot->status != Disabled)
        bot->updateStatusLabel();

    if(bot->status == Recording || bot->status == Replaying || bot->status == Rendering) {
        auto dir = CCDirector::sharedDirector();
        const float tScale = dir->getScheduler()->getTimeScale();

        if(bot->status != Rendering) {
            // speed changed
            if(tScale != bot->settings.targetSpeed) {
                bot->settings.targetSPF = 1.f / (bot->targetFPS * tScale);
                bot->settings.targetSpeed = tScale;
            }

            // fps changed
            if(dir->getAnimationInterval() != bot->settings.targetSPF) {
                dir->setAnimationInterval(bot->settings.targetSPF);
            }

            // fix audio speed
            if(bot->getSongPitch() != tScale) {
                bot->setSongPitch(tScale);
            }

            // update checkpoints
            if(bot->status == Recording) {
                if(self->m_checkpoints->count() > bot->practiceCheckpoints.size()) {
                    bot->handleCheckpoint(self);
                }
            }

            // lock delta
            dt = bot->settings.targetSPF * tScale;
        }
        else {
            // lock delta
            dt = (1.f / bot->settings.targetGameFPS) * tScale;
        }
    }

    // update replay / render
    if((bot->status == Replaying || bot->status == Rendering)
        && !MBO(bool, self, 0x39C) 
        && MBO(bool, self, 0x2EC)
        && bot->levelFrames.size() > 0 
        && (size_t)bot->currentFrame < bot->levelFrames.size()) // shut up cmake 
    {
        LevelFrameData frame = bot->levelFrames[bot->currentFrame];

        if(frame.frame > -1) {
            // jump
            // player 1
            if(frame.player1.action != None) {
                if(frame.player1.action == Pressed) self->pushButton(1, true);
                else self->releaseButton(1, true);
            }

            // player 2
            if(frame.player2.action != None && (MBO(bool, self->m_pLevelSettings, 0xFA) /*isDualMode*/ && MBO(bool, self->m_pLevelSettings, 0xFA) /*isTwoPlayer*/)) {
                if(frame.player2.action == Pressed) self->pushButton(1, false);
                else self->releaseButton(1, false);
            }

            frame.player1.applyToPlayer(self->m_pPlayer1);
            frame.player2.applyToPlayer(self->m_pPlayer2);
        }

        // increment
        bot->currentFrame++;
    }
    else {
        // update recording
        if(bot->status == Recording
            && !MBO(bool, self, 0x39C) // isDead?
            && MBO(bool, self, 0x2EC)
        )
        { 
            bot->handleFrame(self);
        
            // increment
            bot->currentFrame++;
        }
        else 
            if(self->m_pPlayer1->getPositionX() == 0) {
                bot->currentFrame = 0;
        }
    }

    // done replaying / rendering
    if(bot->status != Recording && bot->currentFrame >= bot->levelFrames.size()) {
        if(bot->status == Replaying) bot->toggleReplay();
        if(bot->status == Rendering) bot->toggleRenderDelayed();
    }

    // call original
    PlayLayer_updateO(self, dt);
}

HOOKDEF(void, GJBaseGameLayer_pushButton, gd::GJBaseGameLayer*, int button, bool rightSide) {
    GatoBot::sharedState()->handleClick(self, rightSide, Pressed);

    GJBaseGameLayer_pushButtonO(self, button, rightSide);
}

HOOKDEF(void, GJBaseGameLayer_releaseButton, gd::GJBaseGameLayer*, int button, bool rightSide) {
    GatoBot::sharedState()->handleClick(self, rightSide, Released);

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
    if(bot->status == Replaying || bot->status == Rendering && self->m_isPracticeMode) {
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
            // MBO(bool, self, 0x2A9) && !rightSide && twoPlayer

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

    if(bot->status == Recording || bot->status == Replaying)
        bot->setSongPitch(CCDirector::sharedDirector()->getScheduler()->getTimeScale());

    if(!self->m_isPracticeMode) {
        bot->currentFrame = 0;
        bot->timeFromStart = 0;

        // update music offset
        bot->currentMusicOffset = bot->getTimeForXPos(self) + MBO(float, self->m_pLevelSettings, 0xFC); // timeForXPos + songOffset 
    }
}

HOOKDEF(void, PlayLayer_destroyPlayer, gd::PlayLayer*, gd::PlayerObject* player, gd::GameObject* obj) {
    PlayLayer_destroyPlayerO(self, player, obj);

    auto bot = GatoBot::sharedState();

    if(bot->status == Rendering && MBO(bool, self, 0x39C)) bot->toggleRender();
}

HOOKDEF(void, PlayLayer_levelComplete, gd::PlayLayer*) {
    PlayLayer_levelCompleteO(self);

    GatoBot::sharedState()->resetBasicVariables(false);
}

HOOKDEF(void, PlayLayer_onQuit, gd::PlayLayer*) {
    PlayLayer_onQuitO(self);

    GatoBot::sharedState()->resetBasicVariables(true);
}

HOOKDEF(void, CCScheduler_update, CCScheduler*, float dt) {
    auto bot = GatoBot::sharedState();

    auto pLayer = gd::PlayLayer::get();

    // this is a really shit way to display an error
    if(bot->lastInfoCode != 0) {
        // pause game
        reinterpret_cast<void(__thiscall*)(gd::PlayLayer*, bool)>(gd::base + 0x20d3c0)(pLayer, false);
        
        if(bot->lastInfoCode == 1) {
            auto alert = gd::FLAlertLayer::create(nullptr, "FFmpeg Error", "OK", nullptr, 400, CCString::createWithFormat("<cr>FFmpeg errored. Check the console for logs.</c>")->m_sString);
            alert->m_pTargetLayer = bot->currentPauseLayer;
            alert->show();
        }
        if(bot->lastInfoCode == 2) {
            CCDirector::sharedDirector()->getRunningScene()->addChild(GBLoadingCircle::create(), 9999);
        }

        bot->lastInfoCode = 0;
    }

    if(bot->status == Rendering && !bot->gamePaused) {
        // delay
        if(bot->timeFromStart - bot->endDelayStart >= bot->settings.renderDelay && bot->endDelayStart > 0) {
            bot->toggleRender();

            return;
        }

        if(!bot->currentFrameHasData) {
            auto pLayer = gd::PlayLayer::get();
            float deltaTime = 1.f / static_cast<float>(bot->settings.targetGameFPS); // constant delta time

            if(bot->currentFrame % bot->settings.divideFramesBy == 0
                && !MBO(bool, pLayer, 0x39C) 
                && MBO(bool, pLayer, 0x2EC))
            {
                auto fmod = gd::FMODAudioEngine::sharedEngine();

                if(fmod->isBackgroundMusicPlaying() && !pLayer->m_hasCompletedLevel) {
                    // what the fuck why are the args backwards
                    auto channel = fmod->m_pGlobalChannel;
                    int musicTime = static_cast<int>((bot->getTimeForXPos(pLayer) + MBO(float, pLayer->m_pLevelSettings, 0xFC)) * 1000);
                    __asm {
                        push 0x1;
                        push musicTime;
                        push channel;
                    }
                    GatoBot::FMOD_Channel_setPosition();
                }

                CCScheduler_updateO(self, deltaTime);
                bot->updateRender(); // render frame

                bot->timeFromStart += (1.f / static_cast<float>(bot->settings.targetFPS));
            }
            else {
                CCScheduler_updateO(self, deltaTime);
            }

            if(bot->endDelayStart > 0)
                bot->currentFrame++;
        }
    }
    else {
        CCScheduler_updateO(self, dt);
    }
}

HOOKDEF(void, CCDirector_drawScene, CCDirector*) {
    auto bot = GatoBot::sharedState();

    if(bot->status == Rendering && bot->settings.fastRender && !bot->gamePaused) {
        if(!self->isPaused())
            self->getScheduler()->update(0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        kmGLPushMatrix();

        // draw status label
        bot->statusLabel->visit();

        kmGLPopMatrix();

        // swap buffers
        auto glView = self->getOpenGLView();

        if(glView != nullptr)
            glView->swapBuffers();
    }
    else CCDirector_drawSceneO(self);
}

// setup
void GBHooks::mem_init() {
    auto bot = GatoBot::sharedState();

    // PlayLayer::update
    GB_CreateHook(
        gd::base + 0x2029c0,
        PlayLayer_updateH,
        PlayLayer_updateO
    );

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

    #ifndef GB_GEODE
    MH_EnableHook(MH_ALL_HOOKS);
    #endif

    bot->settings.loadedHooks = true;
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