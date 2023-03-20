#include "GatoBot.hpp"

GatoBot* instance = nullptr;

GatoBot* GatoBot::sharedState() {
    if(instance == nullptr) {
        instance = new GatoBot();
        instance->status = BotStatus::Disabled;
        instance->currentFrame = 0;
        instance->frameDelta = 0;
        instance->timeFromStart = 0;
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
        instance->usingGeode = false;

        instance->settings.targetFPS = instance->getCurrentFPS();

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
            instance->settings.targetWidth = static_cast<int>(frameSize.width);
            instance->settings.targetHeight = static_cast<int>(frameSize.height);
            instance->settings.targetGameFPS = instance->settings.targetFPS;

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

// https://github.com/matcool/small-gd-mods/blob/main/src/menu-shaders.cpp#L19
void GatoBot::patchMemory(void* patchLoc, std::vector<uint8_t> bytes) {
    DWORD old_prot;
    VirtualProtect(patchLoc, bytes.size(), PAGE_EXECUTE_READWRITE, &old_prot);
    memcpy(patchLoc, bytes.data(), bytes.size());
    VirtualProtect(patchLoc, bytes.size(), old_prot, &old_prot);
}

#define SHOWSTATUS(cstatus, format, ...) if(status == cstatus) snprintf(buf, 100, format, #cstatus, __VA_ARGS__) 

void GatoBot::updateStatusLabel() {
    if(statusLabel == nullptr) return;

    if(status == Disabled) {
        statusLabel->setString(" ");
        return;
    }

    char buf[100];

    SHOWSTATUS(Recording, "%s frame: %d", currentFrame + 1);
    SHOWSTATUS(Replaying, "%s frame: %d/%d", currentFrame, levelFrames.size());
    SHOWSTATUS(Rendering, "%s frame: %d/%d\nTime from start: %f", currentFrame + 1, levelFrames.size(), timeFromStart);

    statusLabel->setString(buf);
}

void GatoBot::retryLevel() {
    GatoBot::PauseLayer_onRetry(currentPauseLayer, nullptr);
}

void GatoBot::setSongPitch(float pitch) {
    auto fmod = gd::FMODAudioEngine::sharedEngine();
    if(fmod->isBackgroundMusicPlaying()) {
        auto channel = fmod->m_pGlobalChannel;

        __asm {
            push pitch;
            push channel;
        }

        FMOD_Channel_setPitch();
    }
}

void GatoBot::resetBasicVariables() {
    if(status == Rendering) toggleRender();
    if(status == Recording) toggleRecord();
    if(status == Replaying) toggleReplay();
}

void GatoBot::setupBot() {
    auto self = GatoBot::sharedState();

    HMODULE cocosBase = GetModuleHandleA("libcocos2d.dll");
    HMODULE fmodBase = GetModuleHandleA("fmod.dll");

    self->cocosBaseAddr = cocosBase;

    // update hook addr
    self->updateHookAddr = GetProcAddress(cocosBase, "?update@CCScheduler@cocos2d@@UAEXM@Z");

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
    /*MH_CreateHook(
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

    MH_EnableHook(MH_ALL_HOOKS);

    // - disabled by default
    // CCScheduler::update
    MH_CreateHook(
        bot->updateHookAddr,
        reinterpret_cast<void*>(&CCScheduler_updateH),
        reinterpret_cast<void**>(&CCScheduler_updateO)
    );*/
}