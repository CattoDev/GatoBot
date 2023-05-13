#include "GatoBot.hpp"

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

        instance->targetFPS = instance->getCurrentFPS();

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
            settings.targetWidth = static_cast<int>(frameSize.width);
            settings.targetHeight = static_cast<int>(frameSize.height);
            settings.targetGameFPS = instance->targetFPS;
            settings.targetFPS = 60;

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

float GatoBot::getSongPitch() {
    float ret = 1;

    auto fmod = gd::FMODAudioEngine::sharedEngine();
    if(fmod->isBackgroundMusicPlaying()) {
        auto channel = fmod->m_pGlobalChannel;

        FMOD_Channel_getPitch(channel, &ret);
    }

    return ret;
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

void GatoBot::resetBasicVariables(bool force) {
    if(status == Rendering) {
        if(!force) toggleRenderDelayed();
        else toggleRender();
    }

    if(status == Recording) toggleRecord();
    if(status == Replaying) toggleReplay();
}

void GatoBot::setupBot() {
    auto self = GatoBot::sharedState();

    HMODULE cocosBase = GetModuleHandleA("libcocos2d.dll");
    HMODULE fmodBase = GetModuleHandleA("fmod.dll");

    self->cocosBaseAddr = cocosBase;

    // hook addresses
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

    GatoBot::FMOD_Channel_getPitch = reinterpret_cast<decltype(GatoBot::FMOD_Channel_getPitch)>(
        GetProcAddress(fmodBase, "FMOD_Channel_GetPitch")
    );

    GatoBot::FMOD_Channel_setPitch = reinterpret_cast<decltype(GatoBot::FMOD_Channel_setPitch)>(
        GetProcAddress(fmodBase, "?setPitch@ChannelControl@FMOD@@QAG?AW4FMOD_RESULT@@M@Z")
    );

    GatoBot::PauseLayer_onRetry = reinterpret_cast<decltype(GatoBot::PauseLayer_onRetry)>(gd::base + 0x1E6040);
}

void GatoBot::toggleHook(ToggleHookType hType, bool toggle) {
    switch(hType) {
        case SchedulerUpdate:
            if(toggle) MH_EnableHook(updateHookAddr);
            break;
    }
}