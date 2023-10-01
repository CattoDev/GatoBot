#include "GatoBot.hpp"
#include "hooks.hpp"
#include "LoadingCircle.hpp"

#include <fmod/fmod.hpp>

GatoBot* instance = nullptr;

GatoBot* GatoBot::sharedState() {
    if(instance == nullptr) {
        instance = new GatoBot();
        instance->status = BotStatus::Disabled;
        instance->currentFrame = 0;
        instance->frameDelta = 0;
        instance->timeFromStart = 0;
        instance->endDelayStart = 0;
        instance->player1holding = false;
        instance->player2holding = false;
        instance->currentPauseLayer = nullptr;
        instance->botMenu = nullptr;
        instance->gamePaused = false;
        instance->statusLabel = nullptr;
        instance->lastInfoCode = 0;
        instance->lastSPF = CCDirector::sharedDirector()->getAnimationInterval();
        instance->currentFrameHasData = false;
        instance->presetSettings = false;

        instance->levelFrames.reserve(99999);
    }

    // just in case
    instance->preset();

    return instance;
}

void GatoBot::preset() {
    if(!presetSettings) {
        if(CCEGLView::sharedOpenGLView() != nullptr) { 
            auto frameSize = CCEGLView::sharedOpenGLView()->getFrameSize();
            settings.setOption("targetWidth", static_cast<int>(frameSize.width));
            settings.setOption("targetHeight", static_cast<int>(frameSize.height));
            settings.setOption("targetGameFPS", instance->getCurrentFPS());
            settings.setOption("targetFPS", 60);

            presetSettings = true;
        }
    } 
}

bool GatoBot::FFmpegInstalled() {
    WCHAR buffer[MAX_PATH];
    GetModuleFileNameW(GetModuleHandleA(nullptr), buffer, MAX_PATH);
    const auto path = std::filesystem::path(buffer).parent_path() / "ffmpeg.exe";

    return std::filesystem::exists(path);
}

int GatoBot::getCurrentFPS() {
    // can't wait for stuff to break
    auto dir = CCDirector::sharedDirector();
    return std::round(1.f / dir->getAnimationInterval() / dir->getScheduler()->getTimeScale());
}

void GatoBot::toggleGameFPSCap(bool toggled) {
    uintptr_t addr = (uintptr_t)cocosBaseAddr + 0xC1710;
    auto patchLoc = reinterpret_cast<void*>(addr);

    std::vector<uint8_t> bytes = {0x75, 0x24};
    if(!toggled) bytes = {0xEB, 0x24};

    patchMemory(patchLoc, bytes);
}

float GatoBot::getTimeForXPos(gd::PlayLayer* pLayer) {
    float ret;
    float xPos = pLayer->m_pPlayer1->getPositionX();
    __asm movss xmm1, xPos;
    reinterpret_cast<void(__thiscall*)(gd::PlayLayer*, bool)>(gd::base + 0x208800)(pLayer, pLayer->m_isTestMode); // PlayLayer::timeForXPos2
    __asm movss ret, xmm0; // return value

    return ret;
}

void GatoBot::patchMemory(void* patchLoc, std::vector<uint8_t> bytes) {
    #ifdef GB_GEODE
    Mod::get()->patch(patchLoc, bytes);
    #else
    
    // https://github.com/matcool/small-gd-mods/blob/main/src/menu-shaders.cpp#L19
    DWORD old_prot;
    VirtualProtect(patchLoc, bytes.size(), PAGE_EXECUTE_READWRITE, &old_prot);
    memcpy(patchLoc, bytes.data(), bytes.size());
    VirtualProtect(patchLoc, bytes.size(), old_prot, &old_prot);
    #endif
}

#define SHOWSTATUS(cstatus, format, ...) if(status == cstatus) snprintf(buf, 100, format, #cstatus, __VA_ARGS__) 

void GatoBot::updateStatusLabel() {
    if(statusLabel == nullptr) return;

    if(!inPlayLayer) return;

    if(status == Disabled) {
        statusLabel->setString(" ");
        return;
    }

    char buf[100];

    float timeDiff = (float)(clock() - totalRenderTimeStart) / CLOCKS_PER_SEC;

    SHOWSTATUS(Recording, "%s frame: %d", currentFrame + 1);
    SHOWSTATUS(Replaying, "%s frame: %d/%d", currentFrame + 1, levelFrames.size());
    
    if(status == Rendering) {
        int nextFrame = currentFrame + 1;
        if(nextFrame < levelFrames.size())
            snprintf(buf, 100, "Rendering frame: %d/%d\nSeconds rendered: %f\nTotal rendering time: %f", nextFrame, levelFrames.size(), timeFromStart, timeDiff);

        else
            snprintf(buf, 100, "Rendering frame: %d/%d + %d\nSeconds rendered: %f\nTotal rendering time: %f", nextFrame, levelFrames.size(), nextFrame - levelFrames.size(), timeFromStart, timeDiff);
    }

    statusLabel->setString(buf);
}

void GatoBot::retryLevel() {
    if(currentPauseLayer != nullptr)
        GatoBot::PauseLayer_onRetry(currentPauseLayer, nullptr);
}

void GatoBot::pauseLevel() {
    scheduledPause = true;

    if(auto pLayer = gd::PlayLayer::get())
        reinterpret_cast<void(__thiscall*)(gd::PlayLayer*, bool)>(gd::base + 0x20d3c0)(pLayer, false);

    scheduledPause = false;
}

float GatoBot::getSongPitch() {
    auto fmod = gd::FMODAudioEngine::sharedEngine();
    float pitch = 1;

    if(fmod->isBackgroundMusicPlaying())
        fmod->m_pGlobalChannel->getPitch(&pitch);

    return pitch;
}

void GatoBot::setSongPitch(float pitch) {
    auto fmod = gd::FMODAudioEngine::sharedEngine();
    if(fmod->isBackgroundMusicPlaying()) {
        fmod->m_pGlobalChannel->setPitch(pitch);
    }
}

void GatoBot::changeStatus(BotStatus newStatus, StatusChangeData cData) {
    auto dir = CCDirector::sharedDirector();

    // disable
    if(newStatus == Disabled) {
        if(status == Rendering) {
            toggleRendering(false);
            return;
        }

        dir->setAnimationInterval(lastSPF);
        
        dir->getScheduler()->setTimeScale(1);
        setSongPitch(1);
    }

    // prepare for recording / replaying
    if(newStatus == Recording || newStatus == Replaying) {
        if(newStatus == Recording)
            levelFrames.clear();

        if(cData.FPS > 0 && cData.speed > 0) {
            float newSPF = 1.f / (cData.FPS * cData.speed);
            lastSPF = dir->getAnimationInterval();

            settings.setOption("targetSPF", newSPF);
            settings.setOption("targetSpeed", cData.speed);
            settings.setOption("targetFPS", cData.FPS);

            dir->setAnimationInterval(newSPF);
            dir->getScheduler()->setTimeScale(cData.speed);
        }
    }

    // prepare for rendering
    if(newStatus == Rendering)
        toggleRendering(true);

    else {
        status = newStatus;
        botStatusChanged();
    }

    updateStatusLabel();
}

void GatoBot::resetBasicVariables(bool force) {
    if(!force && settings.getOption<float>("renderDelay") > 0) {
        // delayed rendering
        if(endDelayStart == 0)
            endDelayStart = timeFromStart;
    }
    else endDelayStart = 0;

    changeStatus(Disabled);
}

void GatoBot::toggleAnticheat(bool toggled) {
    std::vector<uint8_t> bytes = {0xE8, 0x3B, 0xAD, 0x00, 0x00};
    if(!toggled) bytes = {0x90, 0x90, 0x90, 0x90, 0x90};

    patchMemory(reinterpret_cast<void*>(gd::base + 0x202ad0), bytes);
}

void GatoBot::checkErrors() {
    if(lastInfoCode != 0) {
        pauseLevel();
            
        if(lastInfoCode == 1) {
            auto alert = gd::FLAlertLayer::create(nullptr, "FFmpeg Error", "OK", nullptr, 400, CCString::createWithFormat("<cr>FFmpeg errored. Check the console for logs.</c>")->m_sString);
            alert->m_pTargetLayer = currentPauseLayer;
            alert->show();
        }
        if(lastInfoCode == 2) {
            CCDirector::sharedDirector()->getRunningScene()->addChild(GBLoadingCircle::create(), 9999);
        }
        if(lastInfoCode == 3) {
            GBLoadingCircle::remove();

            lastInfoCode = 0; // prevent recursion
            botStatusChanged();
        }
        
        lastInfoCode = 0;
    }
}

void GatoBot::updateExitText() {
    if(exitLabel != nullptr && inPlayLayer) {
        auto curTime = clock();

        const int timeOut = 1000;
        const int timeOutHalf = timeOut / 2;

        float diff = curTime - timeFromLastEsc;
        if(diff <= timeOut) {
            if(diff <= timeOut / 2)
                exitLabel->setOpacity(255);

            else {
                exitLabel->setOpacity(255 - (((diff - timeOutHalf) / timeOutHalf) * 255));
            }
        }
        else exitLabel->setOpacity(0);
    }
}

bool GatoBot::updatePlayLayer(float& dt) {
    bool canAdvance = true;

    if(status != Disabled && !gamePaused) {
        updateStatusLabel();

        updateCommon(dt);

        if(status == Recording)
            updateRecording();

        if(status == Replaying)
            updateReplaying();

        if(status == Rendering)
            canAdvance = updateRendering(dt);
    }

    return canAdvance;
}

void GatoBot::updateCommon(float& dt) {
    auto pLayer = gd::PlayLayer::get();
    auto dir = CCDirector::sharedDirector();
    const float tScale = dir->getScheduler()->getTimeScale();

    // bot done
    if(currentFrame >= levelFrames.size() && status != Recording && !isDelayedRendering()) {
        resetBasicVariables(false);
        return;
    }
    
    // speed changed
    if(tScale != settings.getOption<float>("targetSpeed")) {
        if(status != Rendering) {
            settings.setOption("targetSPF", 1.f / (settings.getOption<int>("targetFPS") * tScale));
            settings.setOption("targetSpeed", tScale);
        }
        else dir->getScheduler()->setTimeScale(settings.getOption<float>("targetSpeed"));
    }

    // audio speed
    if(getSongPitch() != tScale)
         setSongPitch(tScale);

    // megahack can be fucky so we update checkpoints like this lol
    if(inPlayLayer && pLayer->m_checkpoints->count() > practiceCheckpoints.size()) {
        handleCheckpoint(pLayer);
    }

    // lock delta
    dt = settings.getOption<float>("targetSPF");
}

void GatoBot::setupBot() {
    HMODULE cocosBase = GetModuleHandleA("libcocos2d.dll");
    HMODULE fmodBase = GetModuleHandleA("fmod.dll");

    cocosBaseAddr = cocosBase;

    // hook addresses
    updateHookAddr = GetProcAddress(cocosBase, "?update@CCScheduler@cocos2d@@UAEXM@Z");

    // declare inline funcs
    GatoBot::ZipUtils_compressString = reinterpret_cast<decltype(GatoBot::ZipUtils_compressString)>(
        GetProcAddress(cocosBase, "?compressString@ZipUtils@cocos2d@@SA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V34@_NH@Z")
    );

    GatoBot::ZipUtils_decompressString = reinterpret_cast<decltype(GatoBot::ZipUtils_decompressString)>(
        GetProcAddress(cocosBase, "?decompressString@ZipUtils@cocos2d@@SA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V34@_NH@Z")
    );

    GatoBot::PauseLayer_onRetry = reinterpret_cast<decltype(GatoBot::PauseLayer_onRetry)>(gd::base + 0x1E6040);

    // hooks
    setupBasicHooks();

    settings.load();

    botStatusChanged();
}

void GatoBot::toggleHook(ToggleHookType hType, bool toggle) {
    #ifdef GB_GEODE
    auto mod = Mod::get();

    for(auto& h : mod->getHooks()) {
        if(
           hType == SchedulerUpdate && !h->getDisplayName().compare("GatoBot -> CCScheduler_updateH")
        || hType == DrawScene && !h->getDisplayName().compare("GatoBot -> CCDirector_drawSceneH")
        ) {
            if(toggle) mod->enableHook(h);
            else mod->disableHook(h);
        }
    }

    #else

    // shit code that I won't even bother rewriting
    switch(hType) {
        case SchedulerUpdate:
            if(toggle) MH_EnableHook(updateHookAddr);
            else MH_DisableHook(updateHookAddr);
            break;

        case DrawScene:
            if(toggle) MH_EnableHook(GetProcAddress(cocosBaseAddr, "?drawScene@CCDirector@cocos2d@@QAEXXZ"));
            else MH_DisableHook(GetProcAddress(cocosBaseAddr, "?drawScene@CCDirector@cocos2d@@QAEXXZ"));
            break;
    };

    #endif
}

void GatoBot::botStatusChanged() {
    toggleHook(SchedulerUpdate, status != Disabled);
    toggleHook(DrawScene, status == Rendering);

    toggleAnticheat(status == Disabled);

    updateStatusLabel();
    checkErrors();
}