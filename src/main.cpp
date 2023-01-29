#include "GatoBot.hpp"
#include "GatoBotMenu.hpp"
#include "LoadingCircle.hpp"

bool(__thiscall* PlayLayer_initO)(gd::PlayLayer*, gd::GJGameLevel*);
bool __fastcall PlayLayer_initH(gd::PlayLayer* self, uintptr_t, gd::GJGameLevel* level) {
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

    bot->resetBasicVariables();

    // status label
    bot->statusLabel = CCLabelBMFont::create(" ", "bigFont.fnt");
    bot->statusLabel->setAnchorPoint(CCPointZero);
    bot->statusLabel->setPosition(10, 10);
    bot->statusLabel->setAlignment(CCTextAlignment::kCCTextAlignmentLeft);
    bot->statusLabel->setScale(.7);

    self->m_uiLayer->addChild(bot->statusLabel, 100);

    return true;
}

void(__thiscall* PlayLayer_updateO)(gd::PlayLayer*, float);
void __fastcall PlayLayer_updateH(gd::PlayLayer* self, uintptr_t, float dt) {
    auto bot = GatoBot::sharedState();

    bot->gamePaused = MBO(bool, self, 0x52F);

    if(bot->status != Disabled)
        bot->updateStatusLabel();

    if(bot->status == Recording || bot->status == Replaying) {
        auto dir = CCDirector::sharedDirector();

        // why is mega hack gay
        if(dir->getAnimationInterval() != bot->settings.targetSPF) {
            dir->setAnimationInterval(bot->settings.targetSPF);
        }

        float spf = (float)dir->getAnimationInterval();
        float tScale = dir->getScheduler()->getTimeScale();

        dt = spf * tScale; // ensure smooth recording (although it will look like smooth fix when replaying, but who cares, not like anyone is gonna record the replay)
    }

    // done replaying / rendering
    if(bot->status != Recording && bot->currentFrame + 1 >= bot->levelFrames.size()) {
        if(bot->status == Replaying) bot->toggleReplay();
        if(bot->status == Rendering) bot->toggleRender();
    }

    // update
    PlayLayer_updateO(self, dt);

    // update replay / render
    if((bot->status == Replaying || bot->status == Rendering) && self->m_pPlayer1->getPositionX() > 0 
        && !MBO(bool, self, 0x39C) 
        && MBO(bool, self, 0x2EC)
        && bot->levelFrames.size() > 0 
        && (size_t)bot->currentFrame < bot->levelFrames.size()) // shut up cmake 
    {
        LevelFrameData frame = bot->levelFrames[bot->currentFrame];

        //std::cout << "alignment: " << bot->currentFrame << " :: " << frame.frame << "\n";

        frame.player1.applyToPlayer(self->m_pPlayer1);
        frame.player2.applyToPlayer(self->m_pPlayer2);

        auto nextFrame = bot->levelFrames[bot->currentFrame + 1];

        // jump
        // player 1
        if(nextFrame.player1.action != None) {
            if(nextFrame.player1.action == Pressed) self->pushButton(1, true);
            else self->releaseButton(1, true);
        }

        // player 2
        if(nextFrame.player2.action != None && (MBO(bool, self->m_pLevelSettings, 0xFA) /*isDualMode*/ && MBO(bool, self->m_pLevelSettings, 0xFA) /*isTwoPlayer*/)) {
            if(nextFrame.player2.action == Pressed) self->pushButton(1, false);
            else self->releaseButton(1, false);
        }

        // increment
        bot->currentFrame++;
    }
    else {
        // update recording
        if(bot->status == Recording && self->m_pPlayer1->getPositionX() > 0
            && !MBO(bool, self, 0x39C)
            && MBO(bool, self, 0x2EC))
        { 
            bot->handleFrame(self);
            
            // increment
            bot->currentFrame++;
        }
        else if(self->m_pPlayer1->getPositionX() == 0) {
            bot->currentFrame = 0;
        }
    }
}

void loadHooks();
// pause layer
void(__thiscall* PauseLayer_customSetupO)(gd::PauseLayer*);
void __fastcall PauseLayer_customSetupH(gd::PauseLayer* self, uintptr_t) {
    PauseLayer_customSetupO(self);

    auto bot = GatoBot::sharedState();

    if(!bot->settings.loadedHooks) {
        loadHooks();
        bot->settings.loadedHooks = true;
    }

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

void(__thiscall* GJBaseGameLayer_pushButtonO)(gd::GJBaseGameLayer*, int, bool);
void __fastcall GJBaseGameLayer_pushButtonH(gd::GJBaseGameLayer* self, uintptr_t, int button, bool rightSide) {
    GJBaseGameLayer_pushButtonO(self, button, rightSide);

    auto bot = GatoBot::sharedState();

    if(bot->status == Recording && !MBO(bool, self, 0x39C)) {
        bool twoPlayer = MBO(bool, self->m_pLevelSettings, 0xFA);

        if(MBO(bool, self, 0x2A9) && !rightSide && twoPlayer)
            bot->queuedBtnP2 = Pressed;

        else bot->queuedBtnP1 = Pressed;
    }
}

void(__thiscall* GJBaseGameLayer_releaseButtonO)(gd::GJBaseGameLayer*, int, bool);
void __fastcall GJBaseGameLayer_releaseButtonH(gd::GJBaseGameLayer* self, uintptr_t, int button, bool rightSide) {
    GJBaseGameLayer_releaseButtonO(self, button, rightSide);

    auto bot = GatoBot::sharedState();

    if(bot->status == Recording && !MBO(bool, self, 0x39C)) {
        bool twoPlayer = MBO(bool, self->m_pLevelSettings, 0xFA);

        if(MBO(bool, self, 0x2A9) && !rightSide && twoPlayer)
            bot->queuedBtnP2 = Released;

        else bot->queuedBtnP1 = Released;
    }
}

void(__thiscall* UILayer_onCheckO)(gd::UILayer*, CCObject*);
void __fastcall UILayer_onCheckH(gd::UILayer* self, uintptr_t, CCObject* pSender) {
    UILayer_onCheckO(self, pSender);

    GatoBot::sharedState()->handleCheckpoint(gd::PlayLayer::get());
}

void midhookFuckery() {
    GatoBot::sharedState()->handleCheckpoint(gd::PlayLayer::get());
}

// midfunc moment
void(*PlayerObject_tryPlaceCheckpointO)();
__declspec(naked) void PlayerObject_tryPlaceCheckpointH() {
    __asm {
        call midhookFuckery;
        jmp PlayerObject_tryPlaceCheckpointO;
    }
}  

void(__thiscall* PlayLayer_removeLastCheckpointO)(gd::PlayLayer*);
void __fastcall PlayLayer_removeLastCheckpointH(gd::PlayLayer* self, uintptr_t) {
    PlayLayer_removeLastCheckpointO(self);

    auto bot = GatoBot::sharedState();

    if(bot->practiceCheckpoints.size() > 0)
        bot->practiceCheckpoints.pop_back();
}

void(__thiscall* PlayLayer_resetLevelO)(gd::PlayLayer*);
void __fastcall PlayLayer_resetLevelH(gd::PlayLayer* self, uintptr_t) {
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

            if(bot->levelFrames.back().player2.isHolding != MBO(bool, self->m_pPlayer2, 0x611)) {
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

void(__thiscall* PlayLayer_destroyPlayerO)(gd::PlayLayer*, gd::PlayerObject*, gd::GameObject*);
void __fastcall PlayLayer_destroyPlayerH(gd::PlayLayer* self, uintptr_t, gd::PlayerObject* player, gd::GameObject* obj) {
    PlayLayer_destroyPlayerO(self, player, obj);

    auto bot = GatoBot::sharedState();

    if(MBO(bool, self->m_pPlayer1, 0x662) || MBO(bool, self, 0x39C)) return;

    // guy wtf is this
    auto magicNoclipSpike = MBO(gd::GameObject*, self, 0x530);

    if(magicNoclipSpike == nullptr) {
        if(bot->status == Rendering) bot->toggleRender();
    }
    else {
        if(obj != nullptr) {
            if(magicNoclipSpike->m_uID != obj->m_uID) {
                if(bot->status == Rendering) bot->toggleRender();
            }
        }
    }
}

void(__thiscall* CCScheduler_updateO)(CCScheduler*, float);
void __fastcall CCScheduler_updateH(CCScheduler* self, uintptr_t, float dt) {
    auto bot = GatoBot::sharedState();

    auto pLayer = gd::PlayLayer::get();

    // this is a really shit way to display an error
    if(bot->lastInfoCode != 0) {
        // pause game
        reinterpret_cast<void(__thiscall*)(gd::PlayLayer*, bool)>(gd::base + 0x20d3c0)(pLayer, false);
        
        if(bot->lastInfoCode == 1) {
            auto alert = gd::FLAlertLayer::create(nullptr, "FFmpeg Error", "OK", nullptr, 400, CCString::createWithFormat("FFmpeg errored. Check the console for logs.")->m_sString);
            alert->m_pTargetLayer = bot->currentPauseLayer;
            alert->show();
        }
        if(bot->lastInfoCode == 2) {
            CCDirector::sharedDirector()->getRunningScene()->addChild(LoadingCircle::create(), 9999);
        }

        bot->lastInfoCode = 0;
    }

    if(bot->status == Rendering && !bot->gamePaused) {
        if(!bot->currentFrameHasData) {
            float deltaTime = 1.f / static_cast<float>(bot->settings.targetGameFPS); // constant delta time

            if(bot->currentFrame % bot->settings.divideFramesBy == 0 && pLayer->m_pPlayer1->getPositionX() > 0) {
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
                bot->updateRender(); // add frame to render

                bot->timeFromStart += 1.f / static_cast<float>(bot->settings.targetFPS);
            }
            else {
                CCScheduler_updateO(self, deltaTime);
            }
        }
    }
    else {
        CCScheduler_updateO(self, dt);
    }
}

void(__thiscall* PlayLayer_levelCompleteO)(gd::PlayLayer*);
void __fastcall PlayLayer_levelCompleteH(gd::PlayLayer* self, uintptr_t) {
    PlayLayer_levelCompleteO(self);

    GatoBot::sharedState()->resetBasicVariables();
}

void(__thiscall* PlayLayer_onQuitO)(gd::PlayLayer*);
void __fastcall PlayLayer_onQuitH(gd::PlayLayer* self, uintptr_t) {
    PlayLayer_onQuitO(self);

    GatoBot::sharedState()->resetBasicVariables();
}

void loadHooks() {
    HMODULE cocosBase = GetModuleHandleA("libcocos2d.dll");
    HMODULE fmodBase = GetModuleHandleA("fmod.dll");

    // init GatoBot and set cocos addr
    GatoBot::sharedState()->cocosBaseAddr = cocosBase;

    // declare inline funcs
    GatoBot::ZipUtils_compressString = reinterpret_cast<decltype(GatoBot::ZipUtils_compressString)>(
        GetProcAddress(cocosBase, "?compressString@ZipUtils@cocos2d@@SA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V34@_NH@Z")
    );

    GatoBot::ZipUtils_decompressString = reinterpret_cast<decltype(GatoBot::ZipUtils_decompressString)>(
        GetProcAddress(cocosBase, "?decompressString@ZipUtils@cocos2d@@SA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V34@_NH@Z")
    );

    GatoBot::FMOD_Channel_setPosition = reinterpret_cast<decltype(GatoBot::FMOD_Channel_setPosition)>(
        GetProcAddress(fmodBase, "FMOD_Channel_SetPosition")
    );

    GatoBot::FMOD_Channel_setPitch = reinterpret_cast<decltype(GatoBot::FMOD_Channel_setPitch)>(
        GetProcAddress(fmodBase, "?setPitch@ChannelControl@FMOD@@QAG?AW4FMOD_RESULT@@M@Z")
    );

    GatoBot::PauseLayer_onRetry = reinterpret_cast<decltype(GatoBot::PauseLayer_onRetry)>(gd::base + 0x1E6040);

    // PlayLayer::update
    MH_CreateHook(
        reinterpret_cast<void*>(gd::base + 0x2029c0),
        reinterpret_cast<void*>(&PlayLayer_updateH),
        reinterpret_cast<void**>(&PlayLayer_updateO)
    );

    // GJBaseGameLayer::pushButton
    MH_CreateHook(
        reinterpret_cast<void*>(gd::base + 0x111500),
        reinterpret_cast<void*>(&GJBaseGameLayer_pushButtonH),
        reinterpret_cast<void**>(&GJBaseGameLayer_pushButtonO)
    );

    // GJBaseGameLayer::releaseButton
    MH_CreateHook(
        reinterpret_cast<void*>(gd::base + 0x111660),
        reinterpret_cast<void*>(&GJBaseGameLayer_releaseButtonH),
        reinterpret_cast<void**>(&GJBaseGameLayer_releaseButtonO)
    );

    // UILayer::onCheck
    MH_CreateHook(
        reinterpret_cast<void*>(gd::base + 0x25fb60),
        reinterpret_cast<void*>(&UILayer_onCheckH),
        reinterpret_cast<void**>(&UILayer_onCheckO)
    );

    // PlayLayer::tryPlaceCheckpoint (MIDHOOK)
    MH_CreateHook(
        reinterpret_cast<void*>(gd::base + 0x20b487),
        reinterpret_cast<void*>(&PlayerObject_tryPlaceCheckpointH),
        reinterpret_cast<void**>(&PlayerObject_tryPlaceCheckpointO)
    );

    // PlayLayer::removeLastCheckpoint
    MH_CreateHook(
        reinterpret_cast<void*>(gd::base + 0x20b830),
        reinterpret_cast<void*>(&PlayLayer_removeLastCheckpointH),
        reinterpret_cast<void**>(&PlayLayer_removeLastCheckpointO)
    );

    // PlayLayer::resetLevel
    MH_CreateHook(
        reinterpret_cast<void*>(gd::base + 0x20bf00),
        reinterpret_cast<void*>(&PlayLayer_resetLevelH),
        reinterpret_cast<void**>(&PlayLayer_resetLevelO)
    );

    // PlayLayer::onComplete
    MH_CreateHook(
        reinterpret_cast<void*>(gd::base + 0x1fd3d0),
        reinterpret_cast<void*>(&PlayLayer_levelCompleteH),
        reinterpret_cast<void**>(&PlayLayer_levelCompleteO)
    );

    // PlayLayer::onQuit
    MH_CreateHook(
        reinterpret_cast<void*>(gd::base + 0x20d810),
        reinterpret_cast<void*>(&PlayLayer_onQuitH),
        reinterpret_cast<void**>(&PlayLayer_onQuitO)
    );

    // PlayLayer::destroyPlayer
    MH_CreateHook(
        reinterpret_cast<void*>(gd::base + 0x20a1a0),
        reinterpret_cast<void*>(&PlayLayer_destroyPlayerH),
        reinterpret_cast<void**>(&PlayLayer_destroyPlayerO)
    );

    // CCScheduler::update
    MH_CreateHook(
        GetProcAddress(cocosBase, "?update@CCScheduler@cocos2d@@UAEXM@Z"),
        reinterpret_cast<void*>(&CCScheduler_updateH),
        reinterpret_cast<void**>(&CCScheduler_updateO)
    );

    MH_EnableHook(MH_ALL_HOOKS);
}

// load
DWORD WINAPI ModThread(void* hModule) {
    // debug console
    /*if(AllocConsole()) {
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    }*/

    // MinHook
    MH_Initialize();

    // PlayLayer::init
    MH_CreateHook(
        reinterpret_cast<void*>(gd::base + 0x1fb780),
        reinterpret_cast<void*>(&PlayLayer_initH),
        reinterpret_cast<void**>(&PlayLayer_initO)
    );

    // PauseLayer::customSetup
    MH_CreateHook(
        reinterpret_cast<void*>(gd::base + 0x1e4620),
        reinterpret_cast<void*>(&PauseLayer_customSetupH),
        reinterpret_cast<void**>(&PauseLayer_customSetupO)
    );

    MH_EnableHook(MH_ALL_HOOKS);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE handle, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        auto h = CreateThread(0, 0, ModThread, handle, 0, 0);
        if (h)
            CloseHandle(h);
        else
            return FALSE;
    }
    return TRUE;
}